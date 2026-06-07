#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragNormalWorld;
layout(location = 1) out vec3 fragColor;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    // The car's per-frame model matrix is translate * rotate(Y) only — no
    // non-uniform scale — so the upper-left 3x3 transforms normals correctly.
    fragNormalWorld = normalize(mat3(ubo.model) * inNormal);
    fragColor = inColor;
    gl_Position = ubo.project * ubo.view * worldPos;
}
