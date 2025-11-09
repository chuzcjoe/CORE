#pragma once

#include "MTLContext.h"

namespace core {
namespace metal {

class MTLTexture {
 public:
  MTLTexture(MTLContext* context);
  ~MTLTexture();

  void LoadTextureFromFile(const char* file_path);

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
