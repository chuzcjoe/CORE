#version 450

layout(location = 0) in vec2 fragGroundUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Anti-aliased checkerboard that fades into the horizon color.
    vec2 grid = floor(fragGroundUV);
    float check = mod(grid.x + grid.y, 2.0);
    vec3 a = vec3(0.78, 0.78, 0.82);
    vec3 b = vec3(0.42, 0.42, 0.48);
    vec3 c = mix(a, b, check);

    float dist = length(fragGroundUV);
    float fade = clamp(1.0 - dist / 40.0, 0.0, 1.0);
    vec3 horizon = vec3(0.08, 0.10, 0.14);
    outColor = vec4(mix(horizon, c, fade), 1.0);
}
