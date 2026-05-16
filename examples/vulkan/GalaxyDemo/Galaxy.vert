#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 project;
    float time;
    float aspect;
    float pad0;
    float pad1;
} ubo;

// per-vertex
layout(location = 0) in vec2 inCorner;

// per-instance
layout(location = 1) in vec3 inInstancePos;
layout(location = 2) in vec3 inInstanceColor;
layout(location = 3) in float inInstanceSize;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out float fragBrightness;

void main() {
    // Differential rotation: inner stars orbit faster than outer
    vec2 xy = inInstancePos.xy;
    float r = length(xy);
    float phase = atan(xy.y, xy.x);
    // angular velocity falls off with radius (flat-ish rotation curve)
    float omega = 1.2 / (r + 0.35);
    float angle = phase + omega * ubo.time;
    vec2 rotated = vec2(cos(angle), sin(angle)) * r;
    vec3 worldPos = vec3(rotated, inInstancePos.z);

    // Project the particle center, then offset in clip space to billboard
    vec4 clipCenter = ubo.project * ubo.view * ubo.model * vec4(worldPos, 1.0);
    vec2 offset = inCorner * inInstanceSize;
    offset.x /= ubo.aspect;
    clipCenter.xy += offset * clipCenter.w;

    gl_Position = clipCenter;
    fragColor = inInstanceColor;
    fragUV = inCorner;
    // Subtle brightness twinkle based on instance phase
    fragBrightness = 0.85 + 0.15 * sin(phase * 7.0 + ubo.time * 2.0);
}
