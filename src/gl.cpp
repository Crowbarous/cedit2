#include "app.h"
#include "gl.h"
#include "mesh/mesh.h"
#include "util.h"
#include <array>
#include <fstream>
#include <sstream>
#include <vector>

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);

	if constexpr (gl_constants::MULTISAMPLE) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,
				gl_constants::MULTISAMPLE_SAMPLES);
	}

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

	if constexpr (gl_constants::MULTISAMPLE)
		glEnable(GL_MULTISAMPLE);

	if constexpr (gl_constants::DEBUG) {
		auto msg_callback = [] (
				GLenum src, GLenum type, GLuint id,
				GLuint severity, GLsizei len,
				const char* msg, const void* param)
			-> void { warning("OpenGL: %s\n", msg); };
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(msg_callback, nullptr);
	}

	init_mesh();
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

static void glsl_append_source (
		const std::string& original_path,
		const std::string& path,
		std::ostringstream& src,
		int recursion_depth)
{
	static constexpr int max_recursion_depth = 100;
	if (recursion_depth > max_recursion_depth) {
		fatal("Shader %s has a recursive #include chain of depth > %i. "
		      "Note that it is NOT possible to guard #include with "
		      "conditional compilation directives!",
		      original_path.c_str(), max_recursion_depth);
	}

	std::ifstream f(gl_constants::PATH_SHADER + path);
	if (!f) {
		if (recursion_depth == 0) {
			fatal("Shader %s: cannot open file", path.c_str());
		} else {
			fatal("Shader %s (included from %s): cannot open file",
					path.c_str(), original_path.c_str());
		}
	}

	auto line_directive = [&] (int number, const std::string& file_name) {
		src << "#line " << number;
		if constexpr (gl_constants::GLSL_FILENAME_IN_LINE_DIRECTIVE)
			src << " \"" << file_name << '\"';
		src << '\n';
	};

	int line_nr = 1;
	std::string line;
	for (; std::getline(f, line); line_nr++) {
		if (line.rfind("#include ", 0) == 0) {
			std::string incl_path = line.substr(9, -1);
			line_directive(0, incl_path);
			glsl_append_source(original_path, incl_path,
					src, recursion_depth + 1);
			line_directive(line_nr + 1, path);
		} else {
			src << line << '\n';
		}

		// we can only put #line once we are past #version
		if (line.rfind("#version ", 0) == 0)
			line_directive(line_nr, path);
	}
}

GLuint glsl_load_shader (const std::string& file_path, GLenum shader_type)
{
	std::ostringstream src("");
	glsl_append_source(file_path, file_path, src, 0);

	GLuint id = glCreateShader(shader_type);
	if (id == 0) {
		fatal("Shader %s: failed to create a new shader",
				file_path.c_str());
	}

	std::string s = src.str();
	const char* ptr = s.c_str();
	glShaderSource(id, 1, &ptr, nullptr);
	glCompileShader(id);

	int success = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);

	if (success)
		return id;

	int log_length = 0;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
	char log[log_length+1];
	log[log_length] = '\0';
	glGetShaderInfoLog(id, log_length, &log_length, log);

	fatal("Shader %s failed to compile. Log:\n%s",
			file_path.c_str(), log);
}

void glsl_delete_shader (GLuint& shader)
{
	glDeleteShader(shader);
	shader = 0;
}

GLuint glsl_link_program (const GLuint* shaders, int num_shaders)
{
	if (shaders == nullptr || num_shaders <= 0)
		fatal("Tried to link a program without any shaders");

	GLuint id = glCreateProgram();
	if (id == 0) {
		fatal("Failed to create program (was going to link %i shaders)",
				num_shaders);
	}

	for (int i = 0; i < num_shaders; i++)
		glAttachShader(id, shaders[i]);

	glLinkProgram(id);

	int link_success = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &link_success);

	if (link_success) {
		for (int i = 0; i < num_shaders; i++)
			glDetachShader(id, shaders[i]);
		return id;
	}

	int log_length = 0;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
	char log[log_length+1];
	log[log_length] = '\0';
	glGetProgramInfoLog(id, log_length, &log_length, log);

	fatal("Program with id %i failed to link. Log:\n%s", id, log);
}

GLuint glsl_link_program (std::initializer_list<GLuint> shaders)
{
	const GLuint* b = shaders.begin();
	const GLuint* e = shaders.end();
	return glsl_link_program(b, e - b);
}
