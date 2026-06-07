#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in float fragBrightness;

layout(location = 0) out vec4 outColor;

void main() {
    // Soft circular star sprite via gaussian falloff
    float d2 = dot(fragUV, fragUV);
    if (d2 > 1.0) discard;
    // Sharp core + soft halo
    float core = exp(-d2 * 6.0);
    float halo = exp(-d2 * 1.5) * 0.35;
    float intensity = (core + halo) * fragBrightness;
    outColor = vec4(fragColor * intensity, intensity);
}
