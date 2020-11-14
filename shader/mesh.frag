out vec4 frag_color;

flat in int face_id;

void main ()
{
	frag_color.rgb = vec3(float(face_id + 20) / 100);
	frag_color.a = 1.0;
}
