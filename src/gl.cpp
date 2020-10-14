#include "app.h"
#include "gl.h"
#include "util.h"
#include <array>
#include <vector>

bool app_opengl_debug = false;
int app_opengl_msaa = -1;

struct render_state_t {
	int res_x;
	int res_y;
	SDL_Window* window;
	SDL_GLContext gl_ctx;
};
static render_state_t rctx;

void render_init ()
{
	rctx.res_x = 640;
	rctx.res_y = 480;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		fatal("SDL init failed: %s", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_constants::VER_MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_constants::VER_MINOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	constexpr GLenum windowflags =
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	rctx.window = SDL_CreateWindow("app",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			rctx.res_x, rctx.res_y, windowflags);
	if (rctx.window == nullptr)
		fatal("SDL window creation failed: %s", SDL_GetError());

	rctx.gl_ctx = SDL_GL_CreateContext(rctx.window);
	if (rctx.gl_ctx == nullptr)
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
}

void render_deinit ()
{
	SDL_Quit();
}


void render_frame ()
{
	glViewport(0, 0, rctx.res_x, rctx.res_y);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewport.render();

	SDL_GL_SwapWindow(rctx.window);

	if (GLenum err = glGetError(); err != 0)
		warning("OpenGL error: %i (0x%x)", err, err);
}

void render_resize_window (int w, int h)
{
	rctx.res_x = w;
	rctx.res_y = h;
}
