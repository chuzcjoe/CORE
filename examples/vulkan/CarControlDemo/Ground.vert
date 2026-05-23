#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec2 fragWorldXZ;

void main() {
    // One texture tile every 4 world units, so the asphalt feels grounded in
    // real scale.
    fragUV = inPos.xz * 0.25;
    fragWorldXZ = inPos.xz;
    gl_Position = ubo.project * ubo.view * vec4(inPos, 1.0);
}
