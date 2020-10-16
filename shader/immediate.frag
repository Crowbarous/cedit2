out vec4 frag_color;
in vec3 vertex_color;

void main ()
{
	frag_color.rgb = vertex_color;
	frag_color.a = 1.0;
}
