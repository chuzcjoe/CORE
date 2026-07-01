#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 project;
    mat4 viewmodel;  // places the weapon in view (camera) space; includes recoil
    float flash;     // 0..1 muzzle-flash intensity this frame
    float pad0;
    float pad1;
    float pad2;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in float inUI;  // 1 for view-space HUD geometry (crosshair)

layout(location = 0) out vec3 fragNormalView;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out float fragUI;
layout(location = 3) out float fragFlash;

void main() {
    vec3 viewPos;
    if (inUI > 0.5) {
        // Crosshair is authored directly in view space.
        viewPos = inPos;
        fragNormalView = vec3(0.0, 0.0, 1.0);
    } else {
        viewPos = (ubo.viewmodel * vec4(inPos, 1.0)).xyz;
        fragNormalView = normalize(mat3(ubo.viewmodel) * inNormal);
    }
    fragUV = inUV;
    fragUI = inUI;
    fragFlash = ubo.flash;
    gl_Position = ubo.project * vec4(viewPos, 1.0);
}
