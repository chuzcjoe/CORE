#pragma once

#include "MTLContext.h"

namespace core {
namespace metal {

class MTLRenderPipeline {
 public:
  explicit MTLRenderPipeline(MTLContext* context);
  ~MTLRenderPipeline();

  void CreateRenderPipeline(MTL::Function* vertex_shader, MTL::Function* fragment_shader,
                            MTL::PixelFormat pixel_format);

  void SetTexture(CA::MetalDrawable* drawable) {
    render_pass_descriptor_->colorAttachments()->object(0)->setTexture(drawable->texture());
  }

  MTL::RenderPipelineState* pipeline_state() const { return pipeline_state_; }
  MTL::RenderPassDescriptor* render_pass_descriptor() const { return render_pass_descriptor_; }

 private:
  MTLContext* context_;
  MTL::RenderPipelineDescriptor* pipeline_descriptor_;
  MTL::RenderPipelineState* pipeline_state_;
  MTL::RenderPassDescriptor* render_pass_descriptor_;
};

}  // namespace metal
}  // namespace core