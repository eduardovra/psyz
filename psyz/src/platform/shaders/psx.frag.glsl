#version 450

layout(location = 0) in vec4 vertexColor;
layout(location = 1) in vec2 rawUV;
layout(location = 2) flat in uint tpage;
layout(location = 3) flat in uint clut;
layout(location = 4) flat in uint textureMode;
layout(location = 5) flat in uint subPixelMask;
layout(location = 6) flat in uint texelShift;
layout(location = 7) flat in uint indexShift;
layout(location = 8) flat in uint indexMask;
layout(location = 9) flat in uint dither;
layout(location = 10) flat in ivec2 pageBase;
layout(location = 11) flat in uvec4 texWindow;
layout(location = 12) in vec2 filterUV;
layout(location = 13) flat in uvec4 filterBox;
layout(location = 14) flat in uint filter2d;

layout(set = 2, binding = 0) uniform sampler2D texVram;

uvec2 resolveTexel() {
    vec2 uv = rawUV + 1.0 / 512.0;
    return uvec2(clamp(floor(uv), vec2(0.0), vec2(255.0)));
}

layout(location = 0) out vec4 FragColor;

uint rgb5551ToU16(vec4 c) {
    uint r = uint(c.r * 31.0 + 0.5);
    uint g = uint(c.g * 31.0 + 0.5);
    uint b = uint(c.b * 31.0 + 0.5);
    uint a = uint(c.a + 0.5);
    return r | (g << 5u) | (b << 10u) | (a << 15u);
}

// the colour of texel uv of the page, through the texture window and CLUT
vec4 fetchTexel(uvec2 uv) {
    uv = (uv & texWindow.xy) | texWindow.zw;
    if (textureMode == 1u) { // 16-bit bitmap
        return texelFetch(texVram, pageBase + ivec2(uv), 0);
    }
    uint subPixel = uv.x & subPixelMask;
    ivec2 texelPos = ivec2(uv.x >> texelShift, uv.y);
    uint word16 = rgb5551ToU16(texelFetch(texVram, pageBase + texelPos, 0));
    uint colorIdx = (word16 >> (subPixel * indexShift)) & indexMask;
    ivec2 clutBase = ivec2((clut % 64u) * 16u, clut / 64u);
    return texelFetch(texVram, clutBase + ivec2(colorIdx, 0), 0);
}

// xBR level 2 (Hyllian), without blending: a pixel takes the colour of its
// texel or of a neighbour across an edge, so the palette, transparency and
// semi-transparency bits stay exact.
vec4 tap(ivec2 base, int dx, int dy) {
    ivec2 t = clamp(
        base + ivec2(dx, dy), ivec2(filterBox.xy), ivec2(filterBox.zw));
    return fetchTexel(uvec2(t));
}

// The luma xBR compares texels by. Transparent texels, and those with the
// semi-transparency bit (often blended away), keep apart from opaque ones.
float texelValue(vec4 c) {
    const vec3 luma = vec3(0.2126, 0.7152, 0.0722) * 48.0;
    return c == vec4(0.0) ? 160.0 : dot(c.rgb, luma) + c.a * 80.0;
}

float tapValue(ivec2 base, int dx, int dy) {
    return texelValue(tap(base, dx, dy));
}

vec4 df(vec4 a, vec4 b) { return abs(a - b); }
vec4 eq(vec4 a, vec4 b) { return vec4(lessThan(df(a, b), vec4(15.0))); }
vec4 neq(vec4 a, vec4 b) { return vec4(notEqual(a, b)); }
vec4 weightedDistance(
    vec4 a, vec4 b, vec4 c, vec4 d, vec4 e, vec4 f, vec4 g, vec4 h) {
    return df(a, b) + df(a, c) + df(d, e) + df(d, f) + 4.0 * df(g, h);
}

vec4 xbrTexel() {
    ivec2 base = ivec2(floor(filterUV));
    vec2 fp = fract(filterUV);
    vec4 B = tap(base, 0, -1);
    vec4 D = tap(base, -1, 0);
    vec4 E = tap(base, 0, 0);
    vec4 F = tap(base, 1, 0);
    vec4 H = tap(base, 0, 1);

    // one vector component per corner: bottom-right, top-right, top-left
    // and bottom-left, each seen as the bottom-right one rotated
    vec4 b = vec4(texelValue(B), texelValue(D), texelValue(H), texelValue(F));
    vec4 c = vec4(tapValue(base, 1, -1), tapValue(base, -1, -1),
                  tapValue(base, -1, 1), tapValue(base, 1, 1));
    vec4 e = vec4(texelValue(E));
    vec4 d = b.yzwx;
    vec4 f = b.wxyz;
    vec4 h = b.zwxy;
    vec4 g = c.zwxy;
    vec4 i = c.wxyz;
    vec4 i4 = vec4(tapValue(base, 2, 1), tapValue(base, 1, -2),
                   tapValue(base, -2, -1), tapValue(base, -1, 2));
    vec4 i5 = vec4(tapValue(base, 1, 2), tapValue(base, 2, -1),
                   tapValue(base, -1, -2), tapValue(base, -2, 1));
    vec4 h5 = vec4(tapValue(base, 0, 2), tapValue(base, 2, 0),
                   tapValue(base, 0, -2), tapValue(base, -2, 0));
    vec4 f4 = h5.yzwx;

    const vec4 ao = vec4(1.0, -1.0, -1.0, 1.0);
    vec4 fx = ao * fp.y + vec4(1.0, 1.0, -1.0, -1.0) * fp.x;
    vec4 fxLeft = ao * fp.y + vec4(0.5, 2.0, -0.5, -2.0) * fp.x;
    vec4 fxUp = ao * fp.y + vec4(2.0, 0.5, -2.0, -0.5) * fp.x;

    vec4 lv0 = neq(e, f) * neq(e, h);
    vec4 lv1 = lv0 * min((1.0 - eq(f, b)) * (1.0 - eq(h, d)) +
                             eq(e, i) * (1.0 - eq(f, i4)) * (1.0 - eq(h, i5)) +
                             eq(e, g) + eq(e, c),
                         1.0);
    vec4 lv2Left = neq(e, g) * neq(d, g);
    vec4 lv2Up = neq(e, c) * neq(b, c);

    vec4 wd1 = weightedDistance(e, c, g, i, h5, f4, h, f);
    vec4 wd2 = weightedDistance(h, d, i5, f, i4, b, e, i);
    vec4 edri = vec4(lessThanEqual(wd1, wd2)) * lv0;
    vec4 edr = vec4(lessThan(wd1, wd2)) * lv1;
    vec4 edrLeft =
        vec4(lessThanEqual(2.0 * df(f, g), df(h, c))) * lv2Left * edr;
    vec4 edrUp =
        vec4(greaterThanEqual(df(f, g), 2.0 * df(h, c))) * lv2Up * edr;

    // with no blending, an edge takes the pixels past its line
    vec4 fx45 = edr * vec4(greaterThan(fx, vec4(1.5, 0.5, -0.5, 0.5)));
    vec4 fx45i = edri * vec4(greaterThan(fx, vec4(1.75, 0.75, -0.25, 0.75)));
    vec4 fx30 = edrLeft * vec4(greaterThan(fxLeft, vec4(1.0, 1.0, -0.5, 0.0)));
    vec4 fx60 = edrUp * vec4(greaterThan(fxUp, vec4(2.0, 0.0, -1.0, 0.5)));
    vec4 edge = max(max(fx30, fx60), max(fx45, fx45i));
    bvec4 px = lessThanEqual(df(e, f), df(e, h));

    vec4 res1 = E;
    if (edge.x > 0.0) {
        res1 = px.x ? F : H;
    }
    if (edge.z > 0.0) {
        res1 = px.z ? D : B;
    }
    vec4 res2 = E;
    if (edge.y > 0.0) {
        res2 = px.y ? B : F;
    }
    if (edge.w > 0.0) {
        res2 = px.w ? H : D;
    }
    float d1 = abs(texelValue(res1) - e.x);
    float d2 = abs(texelValue(res2) - e.x);
    return d2 >= d1 ? res2 : res1;
}

const mat4 ditherMatrix = mat4(
    -4.0, +0.0, -3.0, +1.0,
    +2.0, -2.0, +3.0, -1.0,
    -3.0, +1.0, -4.0, +0.0,
    +3.0, -1.0, +2.0, -2.0);
vec3 applyDither(vec3 c) {
    if (dither == 0u) return c;
    int dx = int(gl_FragCoord.x) & 3;
    int dy = int(gl_FragCoord.y) & 3;
    float off = ditherMatrix[dy][dx];
    vec3 c8 = c * 255.0 + off;
    vec3 c5 = clamp(floor(c8 / 8.0), 0.0, 31.0);
    return c5 / 31.0;
}

void main() {
    vec4 texColor;
    if (textureMode == 0u) { // untextured
        texColor = vec4(1, 1, 1, 2);
    } else if (filter2d != 0u) {
        texColor = xbrTexel();
    } else {
        texColor = fetchTexel(resolveTexel());
    }
    // check for full transparency
    if (texColor == vec4(0, 0, 0, 0)) {
        FragColor = vec4(0);
        return;
    }
    // check for setSemiTrans(p, 1)
    bool isSemiTrans = vertexColor.a < 0.75;
    // when a color has the 0x8000 bit left then it has the semitrans flag on
    bool colorSemiTrans = texColor.a > 0;
    // PS1-accurate texture-color modulation: (tex5 * col8) >> 7, clamp to 31
    vec3 modColor;
    if (textureMode == 0u) {
        // untextured: PS1 uses color directly, no modulation
        modColor = texColor.rgb * vertexColor.rgb;
    } else {
        vec3 tex5 = floor(texColor.rgb * 31.0 + 0.5);
        vec3 col8 = min(floor(vertexColor.rgb * 127.5 + 0.5), vec3(255.0));
        vec3 prod8 = min(tex5 * col8 / 16.0, vec3(255.0));
        modColor = dither != 0u ? prod8 / 255.0
                                : floor(prod8 / 8.0) / 31.0;
    }
    modColor = applyDither(modColor);
    // pre-multiplied alpha output for ONE, ONE_MINUS_SRC_ALPHA blending
    if (colorSemiTrans && isSemiTrans) {
        uint abr = (tpage & 0x60u) >> 5u;
        if (abr == 0u) {
            FragColor = vec4(modColor * 0.5, 0.5); // 50% blend
        } else if (abr == 1u) {
            FragColor = vec4(modColor, 0.0); // additive
        } else if (abr == 2u) {
            FragColor = vec4(modColor, 0.0); // subtractive
        } else {                                  // abr == 3u
            FragColor = vec4(modColor * 0.25, 0.0); // B + F/4
        }
    } else {
        FragColor = vec4(modColor, 1.0); // full opacity
    }
}
