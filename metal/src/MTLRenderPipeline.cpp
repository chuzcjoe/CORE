#include "MTLRenderPipeline.h"

#include "Status.h"

namespace core {
namespace metal {

MTLRenderPipeline::MTLRenderPipeline(MTLContext* context)
    : context_(context), pipeline_descriptor_(nullptr), pipeline_state_(nullptr) {}

MTLRenderPipeline::~MTLRenderPipeline() {
  if (vertex_shader_ != nullptr) {
    vertex_shader_->release();
  }
  if (fragment_shader_ != nullptr) {
    fragment_shader_->release();
  }
  if (pipeline_state_) {
    pipeline_state_->release();
  }
}

Status MTLRenderPipeline::CreateRenderPipeline(
    const std::string shader_file_path, MTL::PixelFormat pixel_format) {
  if (context_ == nullptr || context_->device() == nullptr ||
      pipeline_state_ != nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "Pipeline creation requires a live device and is single-use");
  }
  // Load shaders
  {
    const core::Status status =
        LoadMetalShader(shader_file_path, "vertex_main", "fragment_main");
    if (!status.ok()) {
      return status;
    }
  }

  // Create pipeline descriptor
  pipeline_descriptor_ = MTL::RenderPipelineDescriptor::alloc()->init();
  pipeline_descriptor_->setVertexFunction(vertex_shader_);
  pipeline_descriptor_->setFragmentFunction(fragment_shader_);
  pipeline_descriptor_->colorAttachments()->object(0)->setPixelFormat(pixel_format);

  // Create pipeline state
  NS::Error* error = nullptr;
  pipeline_state_ = context_->device()->newRenderPipelineState(pipeline_descriptor_, &error);
  pipeline_descriptor_->release();
  pipeline_descriptor_ = nullptr;
  if (pipeline_state_ == nullptr) {
    return Status(ErrorCode::kBackend,
                  error != nullptr ? error->localizedDescription()->utf8String()
                                   : "Metal pipeline creation failed",
                  "Metal", error != nullptr ? error->code() : 0);
  }

  // Create render pass descriptor
  render_pass_descriptor_ = MTL::RenderPassDescriptor::renderPassDescriptor();
  auto* color_attachment = render_pass_descriptor_->colorAttachments()->object(0);
  color_attachment->setLoadAction(MTL::LoadActionClear);
  color_attachment->setStoreAction(MTL::StoreActionStore);
  color_attachment->setClearColor(MTL::ClearColor(0.05, 0.05, 0.1, 1.0));

  return Status();
}

Status MTLRenderPipeline::LoadMetalShader(const std::string shader_path,
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
      return Status(ErrorCode::kBackend, "Failed to open .metal source file");
    }
    std::string src;
    file.seekg(0, std::ios::end);
    const std::streampos end = file.tellg();
    if (end < 0) {
      return Status(ErrorCode::kIo, "Failed to determine shader size");
    }
    src.resize(static_cast<size_t>(end));
    file.seekg(0, std::ios::beg);
    file.read(src.data(), static_cast<std::streamsize>(src.size()));
    if (!file) {
      return Status(ErrorCode::kIo, "Failed to read shader source");
    }
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

  if (library == nullptr) {
    return Status(ErrorCode::kBackend,
                  error != nullptr ? error->localizedDescription()->utf8String()
                                   : "Metal library creation failed",
                  "Metal", error != nullptr ? error->code() : 0);
  }
  NS::String* vname = NS::String::string(vertex_fn_name.c_str(), NS::UTF8StringEncoding);
  NS::String* fname = NS::String::string(fragment_fn_name.c_str(), NS::UTF8StringEncoding);
  if (vertex_shader_ != nullptr) {
    vertex_shader_->release();
  }
  if (fragment_shader_ != nullptr) {
    fragment_shader_->release();
  }
  vertex_shader_ = library->newFunction(vname);
  fragment_shader_ = library->newFunction(fname);

  library->release();
  if (vertex_shader_ == nullptr || fragment_shader_ == nullptr) {
    return Status(ErrorCode::kBackend,
                  "Failed to create Metal shader functions");
  }

  return Status();
}

}  // namespace metal
}  // namespace core
