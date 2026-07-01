#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormalView;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in float fragUI;
layout(location = 3) in float fragFlash;

layout(location = 0) out vec4 outColor;

void main() {
    // HUD crosshair: flat white, no texture or lighting.
    if (fragUI > 0.5) {
        outColor = vec4(0.95, 0.95, 0.95, 1.0);
        return;
    }

    vec3 albedo = texture(texSampler, fragUV).rgb;

    // Simple directional light in view space.
    vec3 N = normalize(fragNormalView);
    vec3 L = normalize(vec3(0.35, 0.7, 0.55));
    float diff = max(dot(N, L), 0.0);
    vec3 color = albedo * (0.45 + 0.6 * diff);

    // Muzzle flash briefly lights up the weapon when firing.
    color += clamp(fragFlash, 0.0, 1.0) * vec3(1.0, 0.85, 0.4) * 0.5;

    outColor = vec4(color, 1.0);
}
