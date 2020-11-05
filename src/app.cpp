#include "app.h"
#include "gl.h"
#include "gl_glsl.h"
#include "util.h"

viewport3d_t viewport;
map::mesh some_mesh;

static GLuint mesh_program;

void app_init ()
{
	GLuint shaders[2] = { glsl_load_shader("mesh.frag", GL_FRAGMENT_SHADER),
	                      glsl_load_shader("mesh.vert", GL_VERTEX_SHADER) };
	mesh_program = glsl_link_program(shaders, 2);
	for (GLuint& s: shaders)
		glsl_delete_shader(s);

	viewport.camera =
		{ .pos = { 2.0, 2.0, 2.0 },
		  .angles = { 0.0, 180.0, 0.0 },
		  .fov = 90.0, .aspect = 1.0,
		  .z_near = 0.5, .z_far = 100.0 };
	viewport.set_size(0, 0, 640, 480);

	viewport.map = &some_mesh;

	// Add a square
	constexpr int vert_num = 4;
	const vec3 verts[vert_num] = { { 1.0, 1.0, 0.0 },
	                               { -1.0, 1.0, 0.0 },
	                               { -1.0, -1.0, 0.0 },
	                               { 1.0, -1.0, 0.0 } };
	int vert_ids[vert_num];
	for (int i = 0; i < vert_num; i++)
		vert_ids[i] = viewport.map->add_vertex(verts[i]);
	viewport.map->add_face(vert_ids, vert_num);
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
	int dim_w = this->dim_x_high - this->dim_x_low;
	int dim_h = this->dim_y_high - this->dim_y_low;
	glViewport(this->dim_x_low, this->dim_y_low, dim_w, dim_h);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	const camera_t& cam = this->camera;
	const mat4 transform = cam.get_proj() * cam.get_view();

	glUseProgram(mesh_program);
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));

	this->map->gpu_draw();
}

void viewport3d_t::set_size (int xl, int yl, int xh, int yh)
{
	assert(xh > xl && yh > yl);

	this->dim_x_low = xl;
	this->dim_y_low = yl;
	this->dim_x_high = xh;
	this->dim_y_high = yh;

	int w = xh - xl;
	int h = yh - yl;

	this->camera.aspect = (float) w / h;
}
