layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec3 vert_color;

layout (location = 0) uniform mat4 transform = mat4(1.0);

out vec3 vertex_color;

void main ()
{
	gl_Position = transform * vec4(vert_position, 1.0);
	vertex_color = vert_color;
}
