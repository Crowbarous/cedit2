out vec4 frag_color;

in vec3 color;
in vec2 tex_coord;
in vec3 normal;

void main ()
{
	frag_color.rgb = color;
	frag_color.a = 1.0;
}
