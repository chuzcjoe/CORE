#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragDir;

void main() {
    gl_Position = ubo.project * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragDir = inPosition;
}