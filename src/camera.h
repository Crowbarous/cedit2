#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

struct camera_t {
	vec3 pos;
	vec3 angles;
	float fov;
	float aspect;
	float z_near, z_far;

	vec3 get_rt_vector () const;
	vec3 get_fwd_vector () const;
	vec3 get_up_vector () const;

	mat4 get_proj () const;
	mat4 get_view () const;

	camera_t ()
		: pos(0.0), angles(0.0), fov(90.0), aspect(1.0),
		  z_near(0.5), z_far(100.0) { }

	camera_t (vec3 p, vec3 a, float f, float asp, float zn, float zf)
		: pos(p), angles(a), fov(f), aspect(asp), z_near(zn), z_far(zf) { }
};

#endif /* CAMERA_H */

