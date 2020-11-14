// As required by immediate mode
layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec3 vert_norm;
layout (location = 2) in int vert_face_id;

layout (location = 0) uniform mat4 transform = mat4(1.0);

flat out int face_id;

void main ()
{
	gl_Position = transform * vec4(vert_pos, 1.0);

	const vec3 dir = normalize(-vec3(0.3, 0.6, 0.7));
	face_id = vert_face_id;
}
