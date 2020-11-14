#include "app.h"
#include "gl.h"
#include "gl_immediate.h"
#include "util.h"
#include <array>
#include <vector>

bool app_opengl_debug = false;
int app_opengl_msaa = -1;

struct render_state_t {
	int res_x;
	int res_y;
	SDL_Window* window;
	SDL_GLContext gl_context;
};
static render_state_t render_state;

void render_init ()
{
	render_state.res_x = 640;
	render_state.res_y = 480;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		fatal("SDL init failed: %s", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_constants::VER_MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_constants::VER_MINOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	constexpr GLenum windowflags =
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	render_state.window = SDL_CreateWindow("app",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			render_state.res_x,
			render_state.res_y,
			windowflags);

	if (render_state.window == nullptr)
		fatal("SDL window creation failed: %s", SDL_GetError());

	render_state.gl_context = SDL_GL_CreateContext(render_state.window);
	if (render_state.gl_context == nullptr)
		fatal("SDL GL context creation failed: %s", SDL_GetError());

	glewExperimental = true;
	if (glewInit() != GLEW_OK)
		fatal("GLEW init failed");

	SDL_GL_SetSwapInterval(-1);

	if (app_opengl_msaa > 0) {
		glEnable(GL_MULTISAMPLE);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, app_opengl_msaa);
	}

	if (app_opengl_debug) {
		auto msg_callback = [] (
				GLenum src, GLenum type, GLuint id,
				GLuint severity, GLsizei len,
				const char* msg, const void* param)
			-> void {
				warning("OpenGL: %s\n", msg);
			};
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(msg_callback, nullptr);
	}

	imm::init();
}

void render_deinit ()
{
	imm::deinit();

	SDL_GL_DeleteContext(render_state.gl_context);
	SDL_DestroyWindow(render_state.window);
	SDL_Quit();
}


void render_frame ()
{
	glViewport(0, 0, render_state.res_x, render_state.res_y);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewport.render();

	SDL_GL_SwapWindow(render_state.window);

	if (GLenum err = glGetError(); err != 0)
		warning("OpenGL error: %i (0x%x)", err, err);
}

void render_resize_window (int w, int h)
{
	render_state.res_x = w;
	render_state.res_y = h;
}

void gl_vertex_attrib_ptr (
		int attrib_location,
		int num_elements,
		GLenum data_type,
		bool should_normalize,
		size_t stride,
		size_t start_pointer)
{
	glEnableVertexAttribArray(attrib_location);

	switch (data_type) {
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
		glVertexAttribIPointer(attrib_location, num_elements,
				data_type, stride, (void*) start_pointer);
		break;
	case GL_DOUBLE:
		glVertexAttribLPointer(attrib_location, num_elements,
				data_type, stride, (void*) start_pointer);
		break;
	case GL_FLOAT:
	case GL_HALF_FLOAT:
		glVertexAttribPointer(attrib_location, num_elements, data_type,
				should_normalize, stride, (void*) start_pointer);
		break;
	default:
		// Yes, there are more valid values, such as
		// GL_UNSIGNED_INT_2_10_10_10_REV etc.
		// Revisit this if we *really* want to use them
		assert(false && "See above");
	}
}

GLuint gl_gen_vertex_array ()
{
	GLuint r;
	glGenVertexArrays(1, &r);
	if (unlikely(r == 0))
		fatal("Couldn\'t allocate an OpenGL vertex array");
	return r;
}

void gl_delete_vertex_array (GLuint& a)
{
	glDeleteVertexArrays(1, &a);
	a = 0;
}

GLuint gl_gen_buffer ()
{
	GLuint r;
	glGenBuffers(1, &r);
	if (unlikely(r == 0))
		fatal("Couldn\'t allocate an OpenGL buffer");
	return r;
}

void gl_delete_buffer (GLuint& b)
{
	glDeleteBuffers(1, &b);
	b = 0;
}
