#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 project;
} ubo;

// Per-vertex unit-cube data.
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

// Per-instance data: a model matrix (locations 2..5) and a color.
layout(location = 2) in mat4 inModel;
layout(location = 6) in vec3 inColor;

layout(location = 0) out vec3 fragNormalWorld;
layout(location = 1) out vec3 fragColor;

void main() {
    vec4 worldPos = inModel * vec4(inPos, 1.0);
    fragNormalWorld = normalize(mat3(inModel) * inNormal);
    fragColor = inColor;
    gl_Position = ubo.project * ubo.view * worldPos;
}
