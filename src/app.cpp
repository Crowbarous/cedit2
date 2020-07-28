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

	constexpr int side = 2;
	constexpr float spacing = 2.0;
	constexpr float cube_size = 1.5;
	for (int i = 0; i < side*side*side; i++) {
		vec3 pos((i % side - side * 0.5),
			((i / side) % side - side * 0.5),
			(i / (side*side) - side * 0.5));
		pos *= spacing;
		mesh_add_cuboid(*viewport.mesh, pos, vec3(cube_size));
	}
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
	int dim_w = dim_x_high - dim_x_low;
	int dim_h = dim_y_high - dim_y_low;
	glViewport(dim_x_low, dim_y_low, dim_w, dim_h);

	const mat4 proj = camera.get_proj();
	const mat4 view = camera.get_view();

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glUseProgram(mesh_shader_program_id);

	glUniformMatrix4fv(mesh_gpu_constants::UNIFORM_LOC_VIEW,
			1, false, glm::value_ptr(view));
	glUniformMatrix4fv(mesh_gpu_constants::UNIFORM_LOC_PROJ,
			1, false, glm::value_ptr(proj));

	glEnable(GL_DEPTH_TEST);

	mesh->gpu_sync();
	mesh->gpu_draw();
}

void viewport3d_t::set_size (int xl, int yl, int xh, int yh)
{
	assert(xh > xl && yh > yl);

	dim_x_low = xl;
	dim_y_low = yl;
	dim_x_high = xh;
	dim_y_high = yh;

	int w = xh - xl;
	int h = yh - yl;

	camera.aspect = (float) w / h;
}
