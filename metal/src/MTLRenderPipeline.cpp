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

void MTLRenderPipeline::CreateRenderPipeline(const std::string shader_file_path,
                                             MTL::PixelFormat pixel_format) {
  // Load shaders
  LoadMetalShader(shader_file_path, "vertex_main", "fragment_main");

  // Create pipeline descriptor
  pipeline_descriptor_ = MTL::RenderPipelineDescriptor::alloc()->init();
  pipeline_descriptor_->setVertexFunction(vertex_shader_);
  pipeline_descriptor_->setFragmentFunction(fragment_shader_);
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

void MTLRenderPipeline::LoadMetalShader(const std::string shader_path,
                                        const std::string vertex_fn_name,
                                        const std::string fragment_fn_name) {
  NS::Error* error = nullptr;
  MTL::Library* library = nullptr;

  auto ends_with = [](const std::string& s, const char* suf) -> bool {
    const size_t n = std::strlen(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
  };

  if (ends_with(shader_path, ".metal")) {
    // Compile from source at runtime
    std::ifstream file(shader_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open .metal source file");
    }
    std::string src;
    file.seekg(0, std::ios::end);
    src.resize(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(src.data(), static_cast<std::streamsize>(src.size()));
    file.close();

    NS::String* source = NS::String::string(src.c_str(), NS::UTF8StringEncoding);
    MTL::CompileOptions* opts = MTL::CompileOptions::alloc()->init();
    library = context_->device()->newLibrary(source, opts, &error);
    opts->release();
  } else {
    // Load a precompiled metallib from file path
    NS::String* nsPath = NS::String::string(shader_path.c_str(), NS::UTF8StringEncoding);
    library = context_->device()->newLibrary(nsPath, &error);
  }

  if (error || library == nullptr) {
    throw std::runtime_error("Failed to create Metal library (source or metallib)");
  }
  NS::String* vname = NS::String::string(vertex_fn_name.c_str(), NS::UTF8StringEncoding);
  NS::String* fname = NS::String::string(fragment_fn_name.c_str(), NS::UTF8StringEncoding);
  vertex_shader_ = library->newFunction(vname);
  fragment_shader_ = library->newFunction(fname);

  if (vertex_shader_ == nullptr || fragment_shader_ == nullptr) {
    throw std::runtime_error("Failed to create Metal shader functions");
  }

  library->release();
}

}  // namespace metal
}  // namespace core