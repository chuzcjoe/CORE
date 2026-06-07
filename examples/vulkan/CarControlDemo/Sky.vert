#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 fragDir;

void main() {
    // Force the cube to sit at the far plane so it always renders behind
    // everything else, and use the local position as the cubemap sample dir.
    vec4 pos = ubo.project * ubo.view * vec4(inPos, 1.0);
    gl_Position = pos.xyww;
    fragDir = normalize(inPos);
}
