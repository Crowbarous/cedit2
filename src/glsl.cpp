#include "glsl.h"
#include "util.h"
#include <cassert>
#include <fstream>
#include <sstream>

static const char* GLSL_PROLOGUE =
	"#version 330 core\n"
	"#extension GL_ARB_explicit_uniform_location: require\n";


static void glsl_line_directive (
		std::ostringstream& src,
		int line_nr,
		const std::string& filename)
{
	src << "#line " << line_nr;
	if constexpr (gl_constants::GLSL_FILENAME_IN_LINE_DIRECTIVE)
		src << " \"" << filename << '\"';
	src << '\n';
}

static void glsl_append_source (
		const std::string& original_path,
		const std::string& path,
		std::ostringstream& src,
		int recursion_depth)
{
	constexpr int max_recursion_depth = 100;
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

	int line_nr = 1;
	std::string line;
	for (; std::getline(f, line); line_nr++) {
		if (line.rfind("#include ", 0) == 0) {
			std::string incl_path = line.substr(9, -1);
			glsl_line_directive(src, 0, incl_path);
			glsl_append_source(original_path, incl_path,
					src, recursion_depth + 1);
			glsl_line_directive(src, line_nr + 1, path);
		} else {
			src << line << '\n';
		}
	}
}

GLuint glsl_load_shader (const std::string& file_path, GLenum shader_type)
{
	assert(shader_type == GL_FRAGMENT_SHADER
	    || shader_type == GL_VERTEX_SHADER
	    || shader_type == GL_GEOMETRY_SHADER);

	std::ostringstream src(GLSL_PROLOGUE, std::ios_base::app);
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
	return glsl_link_program(shaders.begin(), shaders.size());
}
