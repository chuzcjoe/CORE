#version 450

layout(location = 0) in vec3 fragNormalWorld;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragPosWorld;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple directional light + ambient + a subtle rim for definition.
    vec3 lightDir = normalize(vec3(0.4, 0.85, 0.3));
    vec3 n = normalize(fragNormalWorld);
    float diffuse = max(dot(n, lightDir), 0.0);
    float ambient = 0.25;
    float rim = pow(1.0 - max(dot(n, vec3(0.0, 0.0, 1.0)), 0.0), 3.0) * 0.15;
    vec3 shaded = fragColor * (ambient + 0.85 * diffuse) + vec3(rim);
    outColor = vec4(shaded, 1.0);
}
