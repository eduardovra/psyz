#version 450

layout(location = 0) in vec2 pos;   // SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2
layout(location = 1) in vec4 color; // SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM

layout(location = 0) out vec4 vertexColor;

void main() {
    float x = (pos.x / (1024.0 / 2.0)) - 1.0;
    float y = (pos.y / (512.0 / 2.0)) - 1.0;
    gl_Position = vec4(x, -y, 0.0, 1.0);
    vertexColor = color;
}
