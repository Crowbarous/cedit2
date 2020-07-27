#include "camera.h"

mat4 camera_t::get_proj () const
{
	return glm::perspective(glm::radians(fov), aspect, z_near, z_far);
}

mat4 camera_t::get_view () const
{
	return glm::lookAt(pos, pos + get_fwd_vector(), get_up_vector());
}

vec3 camera_t::get_fwd_vector () const
{
	const vec2 pitch_yaw = glm::radians(vec2(angles.x, angles.y));
	const vec2 c = glm::cos(pitch_yaw);
	const vec2 s = glm::sin(pitch_yaw);
	return { c.y * c.x, s.y * c.x, s.x };
}

vec3 camera_t::get_rt_vector () const
{
	const vec3 fwd = get_fwd_vector();
	const vec3 world_up(0.0, 0.0, 1.0);
	return glm::normalize(glm::cross(fwd, world_up));
}

vec3 camera_t::get_up_vector () const
{
	const vec3 fwd = get_fwd_vector();
	const vec3 right = glm::cross(fwd, vec3(0.0, 0.0, 1.0));
	return glm::normalize(glm::cross(right, fwd));
}

