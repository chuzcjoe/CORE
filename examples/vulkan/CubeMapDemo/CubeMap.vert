#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragDir;

void main() {
    vec4 pos = ubo.project * ubo.view * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
    fragDir = normalize(inPosition);
}
