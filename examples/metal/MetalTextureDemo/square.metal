#include <metal_stdlib>
using namespace metal;

struct VertexIn {
  float4 position;
  float2 texture_coordinate;
};

struct VertexOut {
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
    float2 texture_coordinate;
};

vertex VertexOut vertex_main(uint vertexID [[vertex_id]],
             constant VertexIn* vertexData) {
    VertexOut out;
    out.position = vertexData[vertexID].position;
    out.texture_coordinate = vertexData[vertexID].texture_coordinate;
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    // Sample the texture to obtain a color
    const float4 colorSample = colorTexture.sample(textureSampler, in.texture_coordinate);
    return colorSample;
}
