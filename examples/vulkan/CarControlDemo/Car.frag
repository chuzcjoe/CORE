#version 450

layout(location = 0) in vec3 fragNormalWorld;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Key + fill directional lighting so the car has shape from any orbit angle.
    vec3 keyDir = normalize(vec3(0.4, 0.85, 0.3));
    vec3 fillDir = normalize(vec3(-0.5, 0.3, -0.4));
    vec3 n = normalize(fragNormalWorld);
    float key = max(dot(n, keyDir), 0.0);
    float fill = max(dot(n, fillDir), 0.0);
    float ambient = 0.20;

    vec3 shaded = fragColor * (ambient + 0.80 * key + 0.20 * fill);
    outColor = vec4(shaded, 1.0);
}
