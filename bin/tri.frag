#version 450

layout(early_fragment_tests) in;
layout(location = 0) in vec2 texc;

layout(location = 0) out vec4 outColor;

layout(std140, push_constant) uniform ui{
    vec4 color;
    float tIndex;
};

layout(set = 1, binding = 1) uniform sampler2DArray tex;

void main() {
    outColor = color*texture(tex, vec3(texc,tIndex));
}