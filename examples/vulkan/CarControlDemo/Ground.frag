#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec2 fragWorldXZ;

layout(binding = 1) uniform sampler2D groundTex;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 sampled = texture(groundTex, fragUV).rgb;

    // Fade the textured plane toward the horizon color so its edge doesn't
    // read as a hard seam against the skybox.
    float dist = length(fragWorldXZ);
    float fade = clamp(1.0 - dist / 70.0, 0.0, 1.0);
    vec3 horizon = vec3(0.58, 0.62, 0.66);
    outColor = vec4(mix(horizon, sampled, fade), 1.0);
}
