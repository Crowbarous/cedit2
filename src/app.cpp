#include "app.h"
#include "gl.h"
#include "gl_glsl.h"
#include "gl_immediate.h"
#include "util.h"

viewport3d_t viewport;

static GLuint mesh_program;

void app_init ()
{
	GLuint shaders[2] = { glsl_load_shader_file(GL_FRAGMENT_SHADER, "mesh.frag"),
	                      glsl_load_shader_file(GL_VERTEX_SHADER, "mesh.vert") };
	mesh_program = glsl_link_program(shaders, 2);
	for (GLuint& s: shaders)
		glsl_delete_shader(s);

	viewport.camera =
		{ .pos = { 2.0, 2.0, 2.0 },
		  .angles = { 0.0, 180.0, 0.0 },
		  .fov = 90.0, .aspect = 1.0,
		  .z_near = 0.5, .z_far = 100.0 };
}

void app_deinit ()
{
	glsl_delete_program(mesh_program);
}


move_flags_t camera_move_flags;
bool camera_is_moving ()
{
	return (camera_move_flags.forward != camera_move_flags.backward)
	    || (camera_move_flags.left != camera_move_flags.right);
}

void app_update ()
{
	if (camera_is_moving()) {
		float speed = 0.1;
		switch (camera_move_flags.speed) {
		case move_flags_t::FAST:
			speed *= 2.5;
			break;
		case move_flags_t::SLOW:
			speed *= 0.33;
			break;
		default:
			break;
		}

		vec3 direction(0.0);

		camera_t& cam = viewport.camera;
		const vec3 right = cam.get_right_vector();
		const vec3 forward = cam.get_forward_vector();

		if (camera_move_flags.forward)
			direction += forward;
		if (camera_move_flags.backward)
			direction -= forward;

		if (camera_move_flags.left)
			direction -= right;
		if (camera_move_flags.right)
			direction += right;

		if (direction != vec3(0.0))
			cam.pos += speed * glm::normalize(direction);
	}
}

void viewport3d_t::render () const
{
	glViewport(this->pos.x,
	           render_context.resolution_y - this->pos.y - this->size.y,
	           this->size.x,
	           this->size.y);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	const camera_t& cam = this->camera;
	const mat4 transform = cam.get_proj() * cam.get_view();

	glUseProgram(mesh_program);
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));

	imm::begin(GL_QUADS);
	imm::vertex({ -1, -1, 0 });
	imm::vertex({ 1, -1, 0 });
	imm::vertex({ 1, 1, 0 });
	imm::vertex({ -1, 1, 0 });
	imm::end();
}

void viewport3d_t::set_dimension (vec2 p, vec2 s)
{
	assert(s.x > 0.0 && s.y > 0.0);

	this->pos = p;
	this->size = s;
	this->camera.aspect = size.x / size.y;
}
