out vec4 frag_color;

in float pixel_shade;

void main ()
{
	frag_color.rg = vec2(pixel_shade);
	frag_color.b = 1.0;
	frag_color.a = 1.0;
}
