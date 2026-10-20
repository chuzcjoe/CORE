#include "Status.h"

#define STB_IMAGE_IMPLEMENTATION
#include "MTLTexture.h"

#include <stb_image.h>

namespace core {
namespace metal {

MTLTexture::MTLTexture(MTLContext* context)
    : context_(context), texture_(nullptr), width_(0), height_(0), channels_(0) {}

MTLTexture::~MTLTexture() {
  if (texture_) {
    texture_->release();
    texture_ = nullptr;
  }
}

Status MTLTexture::LoadTextureFromFile(const char* file_path) {
  if (context_ == nullptr || context_->device() == nullptr) {
    return Status(ErrorCode::kInvalidState,
                  "Texture loading requires a live Metal device");
  }
  stbi_set_flip_vertically_on_load(true);
  unsigned char* image = stbi_load(file_path, &width_, &height_, &channels_, STBI_rgb_alpha);
  if (!image) {
    return Status(ErrorCode::kBackend, "Failed to load texture image");
  }

  MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
  texture_descriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
  texture_descriptor->setWidth(width_);
  texture_descriptor->setHeight(height_);

  MTL::Texture* loaded_texture =
      context_->device()->newTexture(texture_descriptor);
  if (loaded_texture == nullptr) {
    texture_descriptor->release();
    stbi_image_free(image);
    return Status(ErrorCode::kBackend, "Metal texture allocation failed");
  }
  if (texture_ != nullptr) {
    texture_->release();
  }
  texture_ = loaded_texture;

  MTL::Region region = MTL::Region(0, 0, 0, width_, height_, 1);
  NS::UInteger bytes_per_row = 4 * width_;

  texture_->replaceRegion(region, 0, image, bytes_per_row);

  texture_descriptor->release();
  stbi_image_free(image);
  printf("Texture loaded: %s (Width: %d, Height: %d, Channels: %d)\n", file_path, width_, height_,
         channels_);

  return Status();
}

}  // namespace metal
}  // namespace core
