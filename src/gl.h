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
	constexpr bool DEBUG = false;

	constexpr int VER_MAJOR = 3;
	constexpr int VER_MINOR = 3;

	constexpr const char* PATH_SHADER = "shader/";
	constexpr bool GLSL_FILENAME_IN_LINE_DIRECTIVE = true;

	constexpr bool MULTISAMPLE = false;
	constexpr bool MULTISAMPLE_SAMPLES = 4;
}

void render_init ();
void render_deinit ();

void render_frame ();
void render_resize_window (int w, int h);

GLuint glsl_load_shader (const std::string& file_path, GLenum shader_type);
void glsl_delete_shader (GLuint& shader);
GLuint glsl_link_program (const GLuint* shaders, int num_shaders);
GLuint glsl_link_program (std::initializer_list<GLuint> shaders);

struct vert_attribute_t {
	GLuint location;       /* layout (location = ...) */
	GLuint num_components; /* vec3 -> 3 */
	GLenum data_type;      /* GL_FLOAT etc. */
	bool normalize;

	/*
	 * Stride and offset are the concern of the whole mesh format,
	 * rather than a single attribute, so receive them by argument
	 */
	inline void send_attrib (size_t stride, size_t offset) const
	{
		glVertexAttribPointer(location, num_components, data_type,
				normalize, stride, (void*) offset);
		glEnableVertexAttribArray(location);
	}
};

#endif // GL_H
