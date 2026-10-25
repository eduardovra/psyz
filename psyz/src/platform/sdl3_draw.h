#ifndef SDL3_DRAW_H
#define SDL3_DRAW_H

#include "texreplace.h"

static unsigned draw_grid_source_width = 1;
static unsigned draw_grid_target_width = 1;

static float GetDrawGridXScale(void) {
    return (float)draw_grid_target_width / (float)draw_grid_source_width;
}

int Draw_SetHorizontalGrid(
    unsigned int source_width, unsigned int target_width) {
    if (source_width == 0 || target_width == 0) {
        return -1;
    }
    if (draw_grid_source_width == source_width &&
        draw_grid_target_width == target_width) {
        return 0;
    }
    Draw_FlushBuffer();
    draw_grid_source_width = source_width;
    draw_grid_target_width = target_width;
    return 0;
}

typedef struct {
    float x, y; // fractional only for PGXP polygon vertices
    float w;    // PGXP polygon depth; the shader ignores it elsewhere
    unsigned short u, v, c, t;
    unsigned char r, g, b, a;
    unsigned int twin;
    unsigned int uvbox;       // texels the 2D filter may read, see SetFilter2D
    unsigned short hdrect[4]; // atlas x, y, w, h of a TPAGE_HD image
} Vertex;

// ===== SDL3 reserved TPAGE flags, invalid on real hardware =====
#define TPAGE_NOTEXTURE 0x8000 // flag untextured poly
#define TPAGE_DITHER 0x4000    // flag a dithered primitive
#define TPAGE_POLYGON 0x2000   // flag a polygon to sample at PS1 integer coords
#define TPAGE_FILTER 0x1000    // flag a 2D primitive for PSYZ_FILTER2D_XBR
#define TPAGE_HD 0x0800        // flag a primitive drawn from a replacement

#define VRGBA(p) (*(unsigned int*)(&((p).r)))
#define SET_TC(p, tpage, clut)                                                 \
    (p)->t = (u16)(tpage), (p)->c = (u16)(clut), (p)->twin = cur_twin;
#define SET_TC_ALL(p, t, c)                                                    \
    SET_TC(p, t, c)                                                            \
    SET_TC(&(p)[1], t, c) SET_TC(&(p)[2], t, c) SET_TC(&(p)[3], t, c)

#define MAX_VERTEX_COUNT 4096
#define MAX_INDEX_COUNT (MAX_VERTEX_COUNT / 4 * 6)

#define SEMITRANSP 0x02
#define TEXTURED 0x04
#define EXTRA_VERTEX 0x08
#define GOURAUD 0x10
#define TRIANGLE 0x20

static u_short cur_tpage = 0;
static Vertex vertex_buf[MAX_VERTEX_COUNT];
static unsigned short index_buf[MAX_INDEX_COUNT];
static Vertex* vertex_cur;
static unsigned short* index_cur;
static unsigned short n_vertices;
static int n_indices;

// represents a texture window as a 32-bit integer for fast aligned copies
#define TWIN_PACK(and_x, and_y, or_x, or_y)                                    \
    ((unsigned int)(and_x) | ((unsigned int)(and_y) << 8) |                    \
     ((unsigned int)(or_x) << 16) | ((unsigned int)(or_y) << 24))
static unsigned int cur_twin = TWIN_PACK(0xFF, 0xFF, 0x00, 0x00);

static void Draw_EnsureBufferWillNotOverflow(int vertices, int indices) {
    bool bufferFull = n_vertices + vertices > MAX_VERTEX_COUNT ||
                      n_indices + indices > MAX_INDEX_COUNT;
    if (bufferFull) {
        Draw_FlushBuffer();
    }
}
static void Draw_EnqueueBuffer(int vertices, int indices) {
    assert(n_vertices + vertices <= MAX_VERTEX_COUNT);
    assert(n_indices + indices <= MAX_INDEX_COUNT);

    vertex_cur += vertices;
    index_cur += indices;
    n_vertices += vertices;
    n_indices += indices;
}

// Perspective-correct texturing needs the depth of every vertex; a polygon
// missing any is drawn affine, like the hardware does, with w = 1.
static void PolygonSettleDepth(Vertex* v, int n) {
    for (int i = 0; i < n; i++) {
        if (v[i].w <= 0.0f) {
            for (i = 0; i < n; i++) {
                v[i].w = 1.0f;
            }
            return;
        }
    }
}

// A textured quad drawn straight on screen, like a sprite: sides on the axes
// and U following X, V following Y. 3D polygons almost never line up so.
static bool IsScreenAlignedQuad(const Vertex* v) {
    return v[0].y == v[1].y && v[2].y == v[3].y && v[0].x == v[2].x &&
           v[1].x == v[3].x && v[0].v == v[1].v && v[2].v == v[3].v &&
           v[0].u == v[2].u && v[1].u == v[3].u;
}

// Flags a 2D quad for the filter, which reads the texels around each one it
// draws; the box keeps it from reading the neighbours in the texture atlas.
static void SetFilter2D(Vertex* v, u16* tpage) {
    unsigned u0 = SDL_min(v[0].u, v[3].u), u1 = SDL_max(v[0].u, v[3].u);
    unsigned v0 = SDL_min(v[0].v, v[3].v), v1 = SDL_max(v[0].v, v[3].v);
    // a quad's far edge is one past its last texel
    u1 -= u1 > u0;
    v1 -= v1 > v0;
    for (int i = 0; i < 4; i++) {
        v[i].uvbox = TWIN_PACK(u0, v0, u1, v1);
    }
    *tpage |= TPAGE_FILTER;
}

typedef struct {
    int u0, v0, u1, v1; // inclusive
} TexelBox;

// Draws a primitive from the part of its replacement image that stands for
// the texels it reads, so filtering never blends in a neighbouring sprite.
static void SetReplacement(
    Vertex* v, int n, const TexReplaceImage* image, TexelBox box, u16* tpage) {
    const float sx = (float)image->atlas_w / (image->u1 - image->u0 + 1);
    const float sy = (float)image->atlas_h / (image->v1 - image->v0 + 1);
    const int x0 = (int)((box.u0 - image->u0) * sx + 0.5f);
    const int y0 = (int)((box.v0 - image->v0) * sy + 0.5f);
    const int x1 = (int)((box.u1 + 1 - image->u0) * sx + 0.5f);
    const int y1 = (int)((box.v1 + 1 - image->v0) * sy + 0.5f);
    for (int i = 0; i < n; i++) {
        v[i].uvbox = TWIN_PACK(box.u0, box.v0, box.u1, box.v1);
        v[i].hdrect[0] = (unsigned short)(image->atlas_x + x0);
        v[i].hdrect[1] = (unsigned short)(image->atlas_y + y0);
        v[i].hdrect[2] = (unsigned short)SDL_max(x1 - x0, 1);
        v[i].hdrect[3] = (unsigned short)SDL_max(y1 - y0, 1);
    }
    *tpage |= TPAGE_HD;
}

// real hardware use XY coords as signed 11-bit
static short s11(short v) { return (short)(((v & 0x7FF) ^ 1024) - 1024); }

static int writePacket(Vertex* v, int code, int n, u_long* packet, u16* pOut) {
    int w;
    if (!n) {
        return 0;
    }
    if (!Psyz_PgxpLookup((unsigned int)*packet, &v->x, &v->y, &v->w)) {
        v->x = s11(((short*)packet)[0]);
        v->y = s11(((short*)packet)[1]);
        v->w = 0.0f; // unknown, see PolygonSettleDepth
    }
    packet++;
    n--;
    if (!n) {
        return 1;
    }
    w = 1;
    if (code & TEXTURED) {
        v->u = ((u8*)packet)[0];
        v->v = ((u8*)packet)[1];
        *pOut = ((u16*)packet)[1];
        w++;
        packet++;
        n--;
        if (!n) {
            return w;
        }
    } else {
        *pOut = 0;
    }
    if (code & GOURAUD) {
        v++;
        v->r = ((u8*)packet)[0];
        v->g = ((u8*)packet)[1];
        v->b = ((u8*)packet)[2];
        v->a = code & SEMITRANSP ? 0x80 : 0xFF;
        w++;
    }
    return w;
}

static inline bool is_subtract_abr(const Vertex* v) {
    return v->a == 0x80 && (v->t & 0x60) == 0x40;
}

typedef enum {
    BLEND_ADD,
    BLEND_SUB,
    BLEND_SUB_OPAQUE,
    BLEND_COUNT,
} BlendMode;

static inline bool is_untextured(const Vertex* v) { return v->t & 0x8000; }
static int SubtractGroupEnd(int start, int run_end) {
    bool untextured = is_untextured(&vertex_buf[index_buf[start]]);
    int end = start + 3;
    while (end < run_end) {
        if (is_untextured(&vertex_buf[index_buf[end]]) != untextured) {
            break;
        }
        if (!untextured) {
            bool shared = false;
            for (int i = 0; i < 9; i++) {
                shared |= index_buf[end + i / 3] == index_buf[end - 3 + i % 3];
            }
            if (!shared) {
                break;
            }
        }
        end += 3;
    }
    return end;
}

void Draw_SetTexpageMode(ParamDrawTexpageMode* p) {
    // implements SetDrawMode, SetDrawEnv
    unsigned short mode = *(u_short*)p;
    SetDither((mode & 0x200) ? 1 : 0);
    cur_tpage = mode & 0x1FF;
    if (p->tex_y_extra_vram) {
        DEBUGF("tex_y_extra_vram not implemented");
    }
    if (p->tex_flip_x) {
        DEBUGF("tex_flip_x not implemented");
    }
    if (p->tex_flip_y) {
        DEBUGF("tex_flip_y not implemented");
    }
}
void Draw_SetTextureWindow(unsigned int mask_x, unsigned int mask_y,
                           unsigned int off_x, unsigned int off_y) {
    mask_x &= 0x1F;
    mask_y &= 0x1F;
    cur_twin = TWIN_PACK(
        (unsigned char)~(mask_x * 8), (unsigned char)~(mask_y * 8),
        (unsigned char)((off_x & mask_x) * 8),
        (unsigned char)((off_y & mask_y) * 8));
}
void Draw_SetMask(int bit0, int bit1) {
    if (bit0 || bit1) {
        NOT_IMPLEMENTED;
    }
}

#endif
