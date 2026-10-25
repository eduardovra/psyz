#ifndef TEXREPLACE_H
#define TEXREPLACE_H

// Replaces textures with the high-resolution images of a DuckStation
// "texpage" pack, matched by hashing what the game put in VRAM.

#include <stdbool.h>
#include <stdint.h>

#define TEXREPLACE_ATLAS_SIZE 8192

// A pack image placed in the atlas, for the page texels it stands for
typedef struct {
    uint16_t atlas_x, atlas_y, atlas_w, atlas_h;
    uint8_t u0, v0, u1, v1; // page texels, inclusive
} TexReplaceImage;

typedef struct {
    // copies RGBA pixels into the atlas; alpha 0 is transparent, 128 a
    // semi-transparent texel and 255 an opaque one
    void (*upload)(int x, int y, int w, int h, const uint8_t* rgba);
} TexReplaceBackend;

void TexReplace_SetBackend(const TexReplaceBackend* backend);
// the queued primitives were drawn, so a full atlas can start over
void TexReplace_Flushed(void);
// the backend released the atlas, so every image needs loading again
void TexReplace_AtlasLost(void);

// VRAM writes, mirrored into the copy the hashes read
void TexReplace_LoadImage(int x, int y, int w, int h, const uint16_t* src);
void TexReplace_MoveImage(int sx, int sy, int w, int h, int dx, int dy);
void TexReplace_ClearImage(int x, int y, int w, int h, uint16_t color);
// the GPU drew into this area, so the copy no longer knows its texels
void TexReplace_Rendered(int x, int y, int w, int h);

// The image to draw page texels u0..u1, v0..v1 of tpage with clut, or NULL
const TexReplaceImage* TexReplace_Find(
    unsigned tpage, unsigned clut, int u0, int v0, int u1, int v1);

#endif
