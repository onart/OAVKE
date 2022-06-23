#version 450

layout(early_fragment_tests) in;
layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(std140, push_constant) uniform ui{
    vec4 color;
};

void main() {
    outColor = color*vec4(fragColor, 1.0);
}