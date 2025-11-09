#include "MTLRenderPipeline.h"

namespace core {
namespace metal {

MTLRenderPipeline::MTLRenderPipeline(MTLContext* context)
    : context_(context), pipeline_descriptor_(nullptr), pipeline_state_(nullptr) {}

MTLRenderPipeline::~MTLRenderPipeline() {
  if (pipeline_state_) {
    pipeline_state_->release();
  }
}

void MTLRenderPipeline::CreateRenderPipeline(MTL::Function* vertex_shader,
                                             MTL::Function* fragment_shader,
                                             MTL::PixelFormat pixel_format) {
  // Create pipeline descriptor
  pipeline_descriptor_ = MTL::RenderPipelineDescriptor::alloc()->init();
  pipeline_descriptor_->setVertexFunction(vertex_shader);
  pipeline_descriptor_->setFragmentFunction(fragment_shader);
  pipeline_descriptor_->colorAttachments()->object(0)->setPixelFormat(pixel_format);

  // Create pipeline state
  NS::Error* error;
  pipeline_state_ = context_->device()->newRenderPipelineState(pipeline_descriptor_, &error);
  pipeline_descriptor_->release();

  // Create render pass descriptor
  render_pass_descriptor_ = MTL::RenderPassDescriptor::renderPassDescriptor();
  auto* color_attachment = render_pass_descriptor_->colorAttachments()->object(0);
  color_attachment->setLoadAction(MTL::LoadActionClear);
  color_attachment->setStoreAction(MTL::StoreActionStore);
  color_attachment->setClearColor(MTL::ClearColor(0.05, 0.05, 0.1, 1.0));
}

}  // namespace metal
}  // namespace core