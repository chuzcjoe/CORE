#include "GLQuery.h"

#include <iostream>

namespace core {
namespace opengl {

GLQuery::GLQuery(const GLenum query_type, bool verbose)
    : query_type_(query_type), verbose_(verbose) {
  glGenQueries(1, &query_id_);
}

GLQuery::~GLQuery() { glDeleteQueries(1, &query_id_); }

void GLQuery::Begin() { glBeginQuery(query_type_, query_id_); }

void GLQuery::End() {
  glEndQuery(query_type_);
  if (!verbose_) {
    return;
  }

  switch (query_type_) {
    case GL_TIME_ELAPSED:
      GLuint64 time_ns = 0;
      glGetQueryObjectui64v(query_id_, GL_QUERY_RESULT, &time_ns);
      double time_ms = time_ns / 1e6;
      std::cout << "Query Time: " << time_ms << " ms" << std::endl;
      break;
  }
}

}  // namespace opengl
}  // namespace core