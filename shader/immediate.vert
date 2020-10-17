layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec3 vert_normal;
layout (location = 2) in vec2 vert_tex_coord;
layout (location = 3) in vec3 vert_color;

layout (location = 0) uniform mat4 transform = mat4(1.0);

out vec3 color;
out vec2 tex_coord;
out vec3 normal;

void main ()
{
	gl_Position = transform * vec4(vert_position, 1.0);
	normal = vert_normal;
	tex_coord = vert_tex_coord;
	color = vert_color;
}
