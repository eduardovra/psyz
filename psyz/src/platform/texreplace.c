#include "texreplace.h"
#include <psyz.h>
#include <psyz/video.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../internal.h"

#define XXH_INLINE_ALL
#include "xxhash.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#include "stb_image.h"

// A pack image is named after what it replaces, as DuckStation dumps it:
// texpage-[ST]P4-<texels hash>-<palette
// hash>-<page>-<x>-<y>-<w>x<h>-P<min>-<max> The hashes are XXH3-64 of the
// image's page texels as VRAM holds them, and of the palette's first max - min
// + 1 entries; C16 images have no palette.

enum { MODE_P4, MODE_P8, MODE_C16, MODE_COUNT };

typedef struct {
    uint64_t tex_hash;
    uint64_t pal_hash;
    uint16_t pal_len; // palette entries hashed; 0 for C16
    uint16_t pal_max;
    uint8_t mode;
    bool semi; // "ST": alpha marks the semi-transparent texels
    bool loaded;
    bool failed;
    uint16_t x, y, w, h; // page texels
    char* path;
    TexReplaceImage image;
} PackEntry;

static PackEntry* pack;
static int pack_count;
static bool pal_len_used[MODE_COUNT][257];
static TexReplaceBackend backend;

static uint16_t vram[VRAM_H][VRAM_W];
// one bit per 64x256 VRAM cell the GPU drew into since its last upload
static uint32_t rendered_cells;

// the pack images whose hashes match a texture page used with a palette
typedef struct {
    uint32_t key; // 0 when free
    bool stale;
    int16_t page_x, page_y, page_w;
    int16_t clut_x, clut_y, clut_w;
    int first_hit, hit_count;
} Match;

#define MATCH_SLOTS 4096
#define MAX_HITS 65536
static Match matches[MATCH_SLOTS];
static int match_count;
static int hits[MAX_HITS];
static int hit_count;

static int atlas_x, atlas_y, atlas_row_h;
static bool atlas_full;

static int ComparePackEntries(const void* a, const void* b) {
    const PackEntry* x = a;
    const PackEntry* y = b;
    if (x->mode != y->mode) {
        return x->mode < y->mode ? -1 : 1;
    }
    if (x->pal_len != y->pal_len) {
        return x->pal_len < y->pal_len ? -1 : 1;
    }
    if (x->pal_hash != y->pal_hash) {
        return x->pal_hash < y->pal_hash ? -1 : 1;
    }
    return 0;
}

static bool ParseName(const char* name, PackEntry* e) {
    char mode[8];
    unsigned long long tex_hash, pal_hash = 0;
    unsigned page_w, page_h, x, y, w, h, pal_min = 0, pal_max = 0;
    int n =
        sscanf(name, "texpage-%7[^-]-%16llx-%16llx-%ux%u-%u-%u-%ux%u-P%u-%u",
               mode, &tex_hash, &pal_hash, &page_w, &page_h, &x, &y, &w, &h,
               &pal_min, &pal_max);
    if (n != 11) {
        pal_hash = 0;
        n = sscanf(name, "texpage-%7[^-]-%16llx-%ux%u-%u-%u-%ux%u", mode,
                   &tex_hash, &page_w, &page_h, &x, &y, &w, &h);
        if (n != 8) {
            return false;
        }
    }
    const char* m = mode;
    e->semi = !strncmp(m, "ST", 2);
    m += e->semi ? 2 : 0;
    if (!strcmp(m, "P4") && n == 11) {
        e->mode = MODE_P4;
    } else if (!strcmp(m, "P8") && n == 11) {
        e->mode = MODE_P8;
    } else if (!strcmp(m, "C16") && n == 8) {
        e->mode = MODE_C16;
    } else {
        return false;
    }
    if (!w || !h || x + w > 256 || y + h > 256 || pal_min > pal_max ||
        pal_max >= (e->mode == MODE_P4 ? 16u : 256u)) {
        return false;
    }
    e->tex_hash = tex_hash;
    e->pal_hash = pal_hash;
    e->pal_len = e->mode == MODE_C16 ? 0 : (uint16_t)(pal_max - pal_min + 1);
    e->pal_max = (uint16_t)pal_max;
    e->x = (uint16_t)x;
    e->y = (uint16_t)y;
    e->w = (uint16_t)w;
    e->h = (uint16_t)h;
    return true;
}

static void ResetMatches(void) {
    memset(matches, 0, sizeof(matches));
    match_count = 0;
    hit_count = 0;
}

static void ResetAtlas(void) {
    atlas_x = atlas_y = atlas_row_h = 0;
    atlas_full = false;
    for (int i = 0; i < pack_count; i++) {
        pack[i].loaded = false;
    }
}

static void ClosePack(void) {
    for (int i = 0; i < pack_count; i++) {
        free(pack[i].path);
    }
    free(pack);
    pack = NULL;
    pack_count = 0;
    memset(pal_len_used, 0, sizeof(pal_len_used));
    ResetMatches();
    ResetAtlas();
}

int Psyz_VideoSetTextureReplacementDir(const char* dir) {
    ClosePack();
    if (!dir || !*dir) {
        return 0;
    }
    // everything below dir, so extra images can live in their own folder
    int count = 0;
    char** names = SDL_GlobDirectory(dir, NULL, 0, &count);
    if (!names) {
        ERRORF("texture replacement: cannot read %s: %s", dir, SDL_GetError());
        return -1;
    }
    pack = calloc(count ? count : 1, sizeof(*pack));
    for (int i = 0; i < count; i++) {
        PackEntry* e = &pack[pack_count];
        const char* slash = strrchr(names[i], '/');
        const char* file = slash ? slash + 1 : names[i];
        size_t file_len = strlen(file);
        if (strncmp(file, "texpage-", 8) || file_len < 4 ||
            strcmp(file + file_len - 4, ".png")) {
            continue;
        }
        if (!ParseName(file, e)) {
            WARNF("texture replacement: unknown name %s", names[i]);
            continue;
        }
        size_t len = strlen(dir) + strlen(names[i]) + 2;
        e->path = malloc(len);
        snprintf(e->path, len, "%s/%s", dir, names[i]);
        pal_len_used[e->mode][e->pal_len] = true;
        pack_count++;
    }
    SDL_free(names);
    qsort(pack, pack_count, sizeof(*pack), ComparePackEntries);
    INFOF("texture replacement: %d images in %s", pack_count, dir);
    return pack_count;
}

void TexReplace_SetBackend(const TexReplaceBackend* b) { backend = *b; }

void TexReplace_AtlasLost(void) { ResetAtlas(); }

void TexReplace_Flushed(void) {
    if (atlas_full) {
        ResetAtlas();
    }
}

static bool Overlaps(
    int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void Invalidate(int x, int y, int w, int h) {
    if (!match_count) {
        return;
    }
    for (int i = 0; i < MATCH_SLOTS; i++) {
        Match* m = &matches[i];
        if (!m->key || m->stale) {
            continue;
        }
        if (Overlaps(x, y, w, h, m->page_x, m->page_y, m->page_w, 256) ||
            (m->clut_w &&
             Overlaps(x, y, w, h, m->clut_x, m->clut_y, m->clut_w, 1))) {
            m->stale = true;
        }
    }
}

static uint32_t CellBits(int x, int y, int w, int h) {
    uint32_t bits = 0;
    for (int cy = y / 256; cy <= (y + h - 1) / 256 && cy < 2; cy++) {
        for (int cx = x / 64; cx <= (x + w - 1) / 64 && cx < 16; cx++) {
            bits |= 1u << (cy * 16 + cx);
        }
    }
    return bits;
}

void TexReplace_LoadImage(int x, int y, int w, int h, const uint16_t* src) {
    for (int row = 0; row < h; row++) {
        uint16_t* dst = vram[(y + row) % VRAM_H];
        for (int col = 0; col < w; col++) {
            dst[(x + col) % VRAM_W] = src[row * w + col];
        }
    }
    rendered_cells &= ~CellBits(x, y, w, h);
    Invalidate(x, y, w, h);
}

void TexReplace_MoveImage(int sx, int sy, int w, int h, int dx, int dy) {
    uint16_t* tmp = malloc(sizeof(uint16_t) * w * h);
    if (!tmp) {
        return;
    }
    for (int row = 0; row < h; row++) {
        memcpy(&tmp[row * w], &vram[sy + row][sx], sizeof(uint16_t) * w);
    }
    for (int row = 0; row < h; row++) {
        memcpy(&vram[dy + row][dx], &tmp[row * w], sizeof(uint16_t) * w);
    }
    free(tmp);
    if (rendered_cells & CellBits(sx, sy, w, h)) {
        rendered_cells |= CellBits(dx, dy, w, h);
    }
    Invalidate(dx, dy, w, h);
}

void TexReplace_ClearImage(int x, int y, int w, int h, uint16_t color) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            vram[(y + row) % VRAM_H][(x + col) % VRAM_W] = color;
        }
    }
    Invalidate(x, y, w, h);
}

void TexReplace_Rendered(int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        rendered_cells |= CellBits(x, y, w, h);
        Invalidate(x, y, w, h);
    }
}

static uint64_t HashRect(int x, int y, int w, int h) {
    XXH3_state_t state;
    XXH3_64bits_reset(&state);
    for (int row = 0; row < h; row++) {
        XXH3_64bits_update(&state, &vram[y + row][x], sizeof(uint16_t) * w);
    }
    return XXH3_64bits_digest(&state);
}

static const PackEntry* FindFirst(int mode, int pal_len, uint64_t pal_hash) {
    const PackEntry key = {.mode = (uint8_t)mode,
                           .pal_len = (uint16_t)pal_len,
                           .pal_hash = pal_hash};
    int lo = 0, hi = pack_count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (ComparePackEntries(&pack[mid], &key) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return &pack[lo];
}

static void Rebuild(Match* m, int mode) {
    m->stale = false;
    m->first_hit = hit_count;
    m->hit_count = 0;
    if (rendered_cells & CellBits(m->page_x, m->page_y, m->page_w, 256)) {
        return;
    }
    const int shift = 2 - mode;
    const int page_y = m->page_y;
    const int max_len = mode == MODE_P4 ? 16 : mode == MODE_P8 ? 256 : 0;
    for (int len = mode == MODE_C16 ? 0 : 1; len <= max_len; len++) {
        if (!pal_len_used[mode][len] || m->clut_x + len > VRAM_W) {
            continue;
        }
        const uint64_t pal_hash =
            len ? XXH3_64bits(
                      &vram[m->clut_y][m->clut_x], sizeof(uint16_t) * len)
                : 0;
        const PackEntry* end = pack + pack_count;
        for (const PackEntry* e = FindFirst(mode, len, pal_hash);
             e < end && e->mode == mode && e->pal_len == len &&
             e->pal_hash == pal_hash;
             e++) {
            const int x0 = m->page_x + (e->x >> shift);
            const int x1 = m->page_x + ((e->x + e->w) >> shift);
            if (m->clut_x + e->pal_max >= VRAM_W || x1 > VRAM_W ||
                page_y + e->y + e->h > VRAM_H || hit_count == MAX_HITS) {
                continue;
            }
            if (HashRect(x0, page_y + e->y, x1 - x0, e->h) == e->tex_hash) {
                hits[hit_count++] = (int)(e - pack);
                m->hit_count++;
            }
        }
    }
}

static Match* GetMatch(unsigned page, int mode, unsigned clut) {
    const uint32_t key = (page | (unsigned)mode << 5 | clut << 7) + 1;
    uint32_t i = (key * 2654435761u) >> 20;
    for (;; i = (i + 1) % MATCH_SLOTS) {
        Match* m = &matches[i];
        if (m->key == key) {
            return m;
        }
        if (!m->key) {
            break;
        }
    }
    if (match_count >= MATCH_SLOTS * 3 / 4) {
        ResetMatches();
        return GetMatch(page, mode, clut);
    }
    Match* m = &matches[i];
    m->key = key;
    m->stale = true;
    m->page_x = (int16_t)((page & 15) * 64);
    m->page_y = (int16_t)((page >> 4) * 256);
    m->page_w = (int16_t)(64 << mode);
    m->clut_x = (int16_t)((clut & 63) * 16);
    m->clut_y = (int16_t)((clut >> 6) & 511);
    m->clut_w = (int16_t)(mode == MODE_P4 ? 16 : mode == MODE_P8 ? 256 : 0);
    match_count++;
    return m;
}

static bool AllocateAtlas(int w, int h, int* x, int* y) {
    if (atlas_x + w > TEXREPLACE_ATLAS_SIZE) {
        atlas_y += atlas_row_h;
        atlas_x = 0;
        atlas_row_h = 0;
    }
    if (atlas_y + h > TEXREPLACE_ATLAS_SIZE) {
        return false;
    }
    *x = atlas_x;
    *y = atlas_y;
    atlas_x += w;
    atlas_row_h = SDL_max(atlas_row_h, h);
    return true;
}

// DuckStation's alpha rules: an opaque image cuts out below half alpha; a
// semi-transparent one keeps 0000 transparent and marks the rest below 95%
static void ConvertAlpha(uint8_t* p, int count, bool semi) {
    for (int i = 0; i < count; i++, p += 4) {
        if (semi) {
            bool zero = !p[0] && !p[1] && !p[2] && !p[3];
            p[3] = zero ? 0 : p[3] <= 242 ? 128 : 255;
        } else if (p[3] >= 128) {
            p[3] = 255;
        } else {
            p[0] = p[1] = p[2] = p[3] = 0;
        }
    }
}

static bool Load(PackEntry* e) {
    if (e->loaded || e->failed || atlas_full) {
        return e->loaded;
    }
    int w, h, n;
    stbi_uc* pixels = stbi_load(e->path, &w, &h, &n, 4);
    if (!pixels || w > TEXREPLACE_ATLAS_SIZE || h > TEXREPLACE_ATLAS_SIZE) {
        WARNF("texture replacement: cannot load %s", e->path);
        stbi_image_free(pixels);
        e->failed = true;
        return false;
    }
    int x, y;
    if (!AllocateAtlas(w, h, &x, &y)) {
        // queued primitives still sample the atlas; start over once drawn
        atlas_full = true;
        stbi_image_free(pixels);
        return false;
    }
    ConvertAlpha(pixels, w * h, e->semi);
    backend.upload(x, y, w, h, pixels);
    stbi_image_free(pixels);
    e->image = (TexReplaceImage){
        .atlas_x = (uint16_t)x,
        .atlas_y = (uint16_t)y,
        .atlas_w = (uint16_t)w,
        .atlas_h = (uint16_t)h,
        .u0 = (uint8_t)e->x,
        .v0 = (uint8_t)e->y,
        .u1 = (uint8_t)(e->x + e->w - 1),
        .v1 = (uint8_t)(e->y + e->h - 1),
    };
    e->loaded = true;
    return true;
}

const TexReplaceImage* TexReplace_Find(
    unsigned tpage, unsigned clut, int u0, int v0, int u1, int v1) {
    if (!pack_count || !backend.upload) {
        return NULL;
    }
    int mode = (tpage >> 7) & 3;
    if (mode > MODE_C16) {
        mode = MODE_C16;
    }
    if (mode == MODE_C16) {
        clut = 0;
    }
    const unsigned page = tpage & 0x1F;
    Match* m = GetMatch(page, mode, clut & 0xFFFF);
    if (m->stale) {
        if (hit_count > MAX_HITS - 256) {
            ResetMatches();
            m = GetMatch(page, mode, clut & 0xFFFF);
        }
        Rebuild(m, mode);
    }
    for (int i = 0; i < m->hit_count; i++) {
        PackEntry* e = &pack[hits[m->first_hit + i]];
        if (u0 >= e->x && v0 >= e->y && u1 < e->x + e->w && v1 < e->y + e->h &&
            Load(e)) {
            return &e->image;
        }
    }
    return NULL;
}
