#include <metal_stdlib>
using namespace metal;

struct VertexIn {
  float2 position;
  float3 color;
};

struct VertexOut {
  float4 position [[position]];
  float3 color;
};

vertex VertexOut vertex_main(uint vertexId [[vertex_id]],
                             constant VertexIn* vertices [[buffer(0)]]) {
  VertexOut out;
  VertexIn v = vertices[vertexId];
  out.position = float4(v.position, 0.0, 1.0);
  out.color = v.color;
  return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0);
}
