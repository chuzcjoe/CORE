#pragma once

#include <glad/glad.h>

#include <string>

namespace core {
namespace io {

void WriteTextureToFile(const GLint texture, const int width, const int height,
                        const std::string& file_path);

}  // namespace io
}  // namespace core
