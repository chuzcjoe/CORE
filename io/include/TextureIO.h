#pragma once

#include <glad/glad.h>

#include <string>

#include "Status.h"

namespace core {
namespace io {

Status WriteTextureToFile(const GLint texture, const int width,
                          const int height, const std::string& file_path);

}  // namespace io
}  // namespace core
