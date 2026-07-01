#version 450

layout(location = 0) in vec3 fragNormalWorld;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple directional light + ambient.
    vec3 lightDir = normalize(vec3(0.4, 0.85, 0.3));
    vec3 n = normalize(fragNormalWorld);
    float diffuse = max(dot(n, lightDir), 0.0);
    float ambient = 0.30;
    vec3 shaded = fragColor * (ambient + 0.80 * diffuse);
    outColor = vec4(shaded, 1.0);
}
