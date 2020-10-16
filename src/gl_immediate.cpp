#include "gl_immediate.h"
#include "gl_glsl.h"
#include <vector>

static GLuint imm_vao;
static GLuint imm_vbo;
static GLuint imm_program;
static GLenum imm_render_mode;
static bool imm_after_begin;
static mat4 imm_transform_matrix;

struct vertex {
	vec3 position;
	vec3 color;
};
static vertex imm_current_vertex;
static std::vector<vertex> imm_buffer;

static constexpr int ATTRIB_LOC_POSITION = 0;
static constexpr int ATTRIB_LOC_COLOR = 1;
static constexpr int UNIFORM_LOC_TRANSFORM = 0;

void imm_init ()
{
	glGenVertexArrays(1, &imm_vao);
	glBindVertexArray(imm_vao);

	glGenBuffers(1, &imm_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);

	gl_vertex_attrib_ptr(ATTRIB_LOC_POSITION, 3, GL_FLOAT, false,
			sizeof(vertex), offsetof(vertex, position));
	gl_vertex_attrib_ptr(ATTRIB_LOC_COLOR, 3, GL_FLOAT, false,
			sizeof(vertex), offsetof(vertex, color));

	GLuint shaders[2] = { glsl_load_shader("immediate.frag", GL_FRAGMENT_SHADER),
	                      glsl_load_shader("immediate.vert", GL_VERTEX_SHADER) };
	imm_program = glsl_link_program(shaders, 2);
	for (GLuint& s: shaders)
		glsl_delete_shader(s);

	imm_after_begin = false;
	imm_transform_matrix = mat4(1.0);
}

void imm_deinit ()
{
	imm_buffer.clear();
	imm_after_begin = false;

	glsl_delete_program(imm_program);
	glDeleteBuffers(1, &imm_vbo);
	glDeleteVertexArrays(1, &imm_vao);
}

void imm_begin (GLenum render_mode)
{
	assert(!imm_after_begin);
	imm_after_begin = true;

	imm_render_mode = render_mode;
	imm_buffer.clear();
}

#include <iostream>
void imm_end ()
{
	assert(imm_after_begin);
	imm_after_begin = false;

	glBindVertexArray(imm_vao);
	glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(vertex) * imm_buffer.size(),
			imm_buffer.data(), GL_DYNAMIC_DRAW);

	glUseProgram(imm_program);
	glUniformMatrix4fv(UNIFORM_LOC_TRANSFORM, 1, GL_FALSE,
			glm::value_ptr(imm_transform_matrix));

	glDrawArrays(imm_render_mode, 0, imm_buffer.size());
}

void imm_vertex (vec3 v)
{
	assert(imm_after_begin);
	imm_current_vertex.position = v;
	imm_buffer.push_back(imm_current_vertex);
}

void imm_color (vec3 c)
{
	imm_current_vertex.color = c;
}

void imm_set_transform (const mat4& m)
{
	imm_transform_matrix = m;
}
