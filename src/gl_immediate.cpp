#include "gl_immediate.h"
#include "gl_glsl.h"
#include <vector>

static GLuint imm_vao;
static GLuint imm_vbo;
static GLuint imm_program;
static GLenum imm_render_mode;
static bool imm_after_begin;

struct vertex {
	vec3 position;
	vec3 normal;
	vec2 tex_coord;
	vec3 color;
};
static vertex imm_current_vertex;
static std::vector<vertex> imm_buffer;

void imm_init ()
{
	glGenVertexArrays(1, &imm_vao);
	glBindVertexArray(imm_vao);

	glGenBuffers(1, &imm_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, imm_vbo);

	using namespace imm_attrib_loc;
	gl_vertex_attrib_ptr(POSITION, 3, GL_FLOAT, false,
			sizeof(vertex), offsetof(vertex, position));
	gl_vertex_attrib_ptr(NORMAL, 3, GL_FLOAT, true,
			sizeof(vertex), offsetof(vertex, normal));
	gl_vertex_attrib_ptr(TEX_COORD, 2, GL_FLOAT, false,
			sizeof(vertex), offsetof(vertex, tex_coord));
	gl_vertex_attrib_ptr(COLOR, 3, GL_FLOAT, false,
			sizeof(vertex), offsetof(vertex, color));

	imm_after_begin = false;
	imm_current_vertex = { .position = vec3(0.0),
	                       .normal = vec3(0.0),
	                       .tex_coord = vec2(0.0),
	                       .color = vec3(1.0) };
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

void imm_normal (vec3 n)
{
	imm_current_vertex.normal = n;
}

void imm_tex_coord (vec2 t)
{
	imm_current_vertex.tex_coord = t;
}
