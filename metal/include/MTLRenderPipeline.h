#pragma once

#include "MTLContext.h"
#include "Status.h"

namespace core {
namespace metal {

class MTLRenderPipeline {
 public:
  explicit MTLRenderPipeline(MTLContext* context);
  ~MTLRenderPipeline();

  Status CreateRenderPipeline(const std::string shader_file_path,
                              MTL::PixelFormat pixel_format);

  void SetTexture(CA::MetalDrawable* drawable) {
    render_pass_descriptor_->colorAttachments()->object(0)->setTexture(drawable->texture());
  }

  MTL::RenderPipelineState* pipeline_state() const { return pipeline_state_; }
  MTL::RenderPassDescriptor* render_pass_descriptor() const { return render_pass_descriptor_; }

 private:
  Status LoadMetalShader(const std::string shader_path,
                         const std::string vertex_fn_name,
                         const std::string fragment_fn_name);

  MTLContext* context_;
  MTL::RenderPipelineDescriptor* pipeline_descriptor_ = nullptr;
  MTL::RenderPipelineState* pipeline_state_ = nullptr;
  MTL::RenderPassDescriptor* render_pass_descriptor_ = nullptr;

  MTL::Function* vertex_shader_ = nullptr;
  MTL::Function* fragment_shader_ = nullptr;
};

}  // namespace metal
}  // namespace core
