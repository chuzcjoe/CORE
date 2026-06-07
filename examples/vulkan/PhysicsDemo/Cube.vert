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
layout(location = 2) out vec3 fragPosWorld;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    fragPosWorld = worldPos.xyz;
    // No non-uniform scaling on the cube, so the upper-left 3x3 is a rotation
    // and we can transform the normal directly.
    fragNormalWorld = normalize(mat3(ubo.model) * inNormal);
    fragColor = inColor;
    gl_Position = ubo.project * ubo.view * worldPos;
}
