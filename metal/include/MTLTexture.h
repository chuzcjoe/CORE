#pragma once

#include "MTLContext.h"
#include "Status.h"

namespace core {
namespace metal {

class MTLTexture {
 public:
  MTLTexture(MTLContext* context);
  ~MTLTexture();

  Status LoadTextureFromFile(const char* file_path);

  MTL::Texture* texture() const { return texture_; }

 private:
  MTLContext* context_;
  MTL::Texture* texture_;

  int width_;
  int height_;
  int channels_;
};

}  // namespace metal
}  // namespace core
