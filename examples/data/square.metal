#include <metal_stdlib>
using namespace metal;

// Layout must match the VertexData struct in MetalTextureDemo/main.cpp.
struct VertexData {
    float4 position;
    float2 texture_coordinate;
};

struct RasterizerData {
    float4 position [[position]];
    float2 texture_coordinate;
};

vertex RasterizerData vertex_main(uint vertex_id [[vertex_id]],
                                  device const VertexData* vertices [[buffer(0)]]) {
    RasterizerData out;
    out.position = vertices[vertex_id].position;
    out.texture_coordinate = vertices[vertex_id].texture_coordinate;
    return out;
}

fragment float4 fragment_main(RasterizerData in [[stage_in]],
                              texture2d<float> color_texture [[texture(0)]]) {
    constexpr sampler texture_sampler(mag_filter::linear, min_filter::linear);
    return color_texture.sample(texture_sampler, in.texture_coordinate);
}
