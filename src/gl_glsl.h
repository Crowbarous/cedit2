#ifndef GL_GLSL_H
#define GL_GLSL_H

#include "gl.h"

GLuint glsl_load_shader (const std::string& file_path, GLenum shader_type);
void glsl_delete_shader (GLuint& shader);

GLuint glsl_link_program (const GLuint* shaders, int num_shaders);
GLuint glsl_link_program (std::initializer_list<GLuint> shaders);
void glsl_delete_program (GLuint& program);

#endif /* GL_GLSL_H */
