#version 450

layout(location = 0) in vec3 fragDir;

layout(binding = 1) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(skybox, fragDir);
}