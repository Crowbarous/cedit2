#include "gl_immediate.h"
#include "gl_glsl.h"
#include <vector>

namespace imm
{

static GLuint vao;
static GLuint vbo;
static GLenum current_render_mode;
static bool after_begin;

struct vert {
	vec3 position;
	vec3 normal;
	vec2 tex_coord;
	vec3 color;
};

static vert current_vertex;
static std::vector<vert> buffer;

void init ()
{
	vao = gl_gen_vertex_array();
	glBindVertexArray(vao);

	vbo = gl_gen_buffer();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	using namespace attrib_loc;
	gl_vertex_attrib_ptr(POSITION, 3, GL_FLOAT, false,
	                     sizeof(vert), offsetof(vert, position));
	gl_vertex_attrib_ptr(NORMAL, 3, GL_FLOAT, true,
	                     sizeof(vert), offsetof(vert, normal));
	gl_vertex_attrib_ptr(TEX_COORD, 2, GL_FLOAT, false,
	                     sizeof(vert), offsetof(vert, tex_coord));
	gl_vertex_attrib_ptr(COLOR, 3, GL_FLOAT, false,
	                     sizeof(vert), offsetof(vert, color));

	after_begin = false;
	current_vertex = { .position = vec3(0.0),
	                   .normal = vec3(0.0),
	                   .tex_coord = vec2(0.0),
	                   .color = vec3(1.0) };
}

void deinit ()
{
	buffer.clear();
	after_begin = false;

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void begin (GLenum render_mode)
{
	assert(!after_begin);
	after_begin = true;

	current_render_mode = render_mode;
	buffer.clear();
}

void end ()
{
	assert(after_begin);
	after_begin = false;

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(vert) * buffer.size(),
			buffer.data(), GL_DYNAMIC_DRAW);

	if (current_render_mode == GL_QUADS)
		current_render_mode = GL_TRIANGLES;

	glDrawArrays(current_render_mode, 0, buffer.size());
}

void vertex (vec3 v)
{
	assert(after_begin);
	current_vertex.position = v;

	buffer.push_back(current_vertex);

	unsigned bs = buffer.size();
	if (current_render_mode == GL_QUADS && bs % 6 == 4) {
		buffer.push_back(buffer[bs - 4]);
		buffer.push_back(buffer[bs - 2]);
	}
}

void color (vec3 c)
{
	current_vertex.color = c;
}

void normal (vec3 n)
{
	current_vertex.normal = n;
}

void tex_coord (vec2 t)
{
	current_vertex.tex_coord = t;
}

} /* namespace imm */
