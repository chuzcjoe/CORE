#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec2 fragGroundUV;

void main() {
    fragGroundUV = inPos.xz;
    gl_Position = ubo.project * ubo.view * vec4(inPos, 1.0);
}
