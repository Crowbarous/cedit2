#include "app.h"
#include "gl.h"
#include "util.h"

viewport3d_t viewport;
mesh_t some_mesh;

void app_init ()
{
	viewport.camera =
		{ .pos = { 2.0, 2.0, 2.0 },
		  .angles = { 0.0, 180.0, 0.0 },
		  .fov = 90.0, .aspect = 1.0,
		  .z_near = 0.5, .z_far = 100.0 };
	viewport.mesh = &some_mesh;
	viewport.set_size(0, 0, 640, 480);
}

void app_deinit ()
{
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
		const vec3 right = cam.get_rt_vector();
		const vec3 forward = cam.get_fwd_vector();

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

	const mat4 proj = this->camera.get_proj();
	const mat4 view = this->camera.get_view();

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	glEnable(GL_DEPTH_TEST);
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
