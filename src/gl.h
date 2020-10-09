#ifndef GL_H
#define GL_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

/* Various constants regarding OpenGL */
namespace gl_constants
{
	constexpr int VER_MAJOR = 3;
	constexpr int VER_MINOR = 3;

	constexpr const char* PATH_SHADER = "shader/";

	/* Whether to put #line 5 "my_shader.frag" instead of just #line 5 */
	constexpr bool GLSL_FILENAME_IN_LINE_DIRECTIVE = true;
}

extern bool app_opengl_debug;
extern int app_opengl_msaa;

void render_init ();
void render_deinit ();

void render_frame ();
void render_resize_window (int w, int h);

GLuint glsl_load_shader (const std::string& file_path, GLenum shader_type);
void glsl_delete_shader (GLuint& shader);
GLuint glsl_link_program (const GLuint* shaders, int num_shaders);
GLuint glsl_link_program (std::initializer_list<GLuint> shaders);

#endif // GL_H
