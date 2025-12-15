#pragma once

#include <glad/glad.h>

namespace core {
namespace opengl {

class GLQuery {
 public:
  GLQuery(const GLenum query_type, bool verbose = false);
  ~GLQuery();

  void Begin();
  void End();

 private:
  unsigned int query_id_;
  GLenum query_type_;
  bool verbose_;
};

}  // namespace opengl
}  // namespace core
