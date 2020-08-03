#version 330 core

out vec4 frag_color;

in float pixel_shade;

void main ()
{
	frag_color.rgb = vec3(pixel_shade);
	frag_color.b += 0.3;
	frag_color.a = 1.0;
}
