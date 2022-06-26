#version 450
// #extension GL_KHR_vulkan_glsl: enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 tc;

layout(location = 0) out vec2 texc;

layout(std140, set = 0, binding = 0) uniform UBO{mat4 rotation;} ubo;

void main() {
    gl_Position = ubo.rotation*vec4(inPosition, 1.0);
    texc=tc;
}
