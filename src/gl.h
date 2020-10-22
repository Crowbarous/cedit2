#ifndef GL_H
#define GL_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

/* Various constants regarding OpenGL */
namespace gl_constants
{
	constexpr int VER_MAJOR = 3;
	constexpr int VER_MINOR = 3;

	constexpr const char* PATH_SHADER = "shader/";

	/*
	 * Whether to put #line 5 "my_shader.frag" instead of just #line 5.
	 * This isn't in the spec; nvidia seems to support it, AMD is less happy
	 */
	constexpr bool GLSL_FILENAME_IN_LINE_DIRECTIVE = true;
}

extern bool app_opengl_debug;
extern int app_opengl_msaa;

void render_init ();
void render_deinit ();

void render_frame ();
void render_resize_window (int w, int h);

/* Thin wrappers aronund GL functions, where appropriate */

/*
 *  Also does EnableVertexAttribArray so that you don't forget to
 *  and takes care of the stupid (void*) cast of the start pointer
 */
void gl_vertex_attrib_ptr (
		int attrib_location,
		int num_elements,
		GLenum data_type,
		bool should_normalize,
		size_t stride,
		size_t start_pointer);

GLuint gl_gen_vertex_array ();
void gl_delete_vertex_array (GLuint&);

GLuint gl_gen_buffer ();
void gl_delete_buffer (GLuint&);

#endif /* GL_H */
