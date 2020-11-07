#include "map_edit.h"
#include "gl_immediate.h"
#include <cassert>

namespace map
{

/*
 * Some functions do book keeping that might erroneuously continue
 * to compile when `mesh::face` changes and silently do the wrong thing
 */
#define REWRITE_THIS_IF_FACE_STRUCT_CHANGES() \
	static_assert(sizeof(face) == sizeof(face::vert_ids) \
	                            + sizeof(face::num_verts) \
	                            + sizeof(face::internal_id), \
		"Struct map::mesh::face changed!!! Revisit all functions " \
		"with this assertion before updating the macro")


mesh::mesh () { }

mesh::~mesh ()
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	for (int i = 0; i < this->faces.size(); i++) {
		face& f = this->faces[i];
		if (this->face_exists(i)) {
			assert(f.vert_ids != nullptr);
			delete[] f.vert_ids;
		} else {
			assert(f.vert_ids == nullptr);
		}
	}
}

int mesh::add_vertex (const vec3& position)
{
	int vert_id = this->verts_active.set_first_cleared();

	assert(vert_id <= this->verts.size());
	if (vert_id == this->verts.size())
		this->verts.emplace_back();

	this->verts[vert_id] = { .position = position,
	                         .faces_referencing = 0};
	return vert_id;
}

int mesh::add_face (const int* vert_ids, int vert_num)
{
	int face_id = this->faces_active.set_first_cleared();

	assert(face_id <= this->faces.size());
	if (face_id == this->faces.size())
		this->faces.emplace_back();

	this->construct_face_at_id(face_id, vert_ids, vert_num);
	return face_id;
}

int mesh::add_face (std::initializer_list<int> vert_ids)
{
	return this->add_face(
			(const int*) vert_ids.begin(),
			(int) vert_ids.size());
}

void mesh::construct_face_at_id (
		int face_id,
		const int* in_vert_ids,
		int vert_num)
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	assert(vert_num >= 3);

	face& f = this->faces[face_id];
	assert(f.vert_ids == nullptr);

	f.vert_ids = new int[vert_num];
	f.num_verts = vert_num;

	for (int i = 0; i < vert_num; i++) {
		assert(this->vert_exists(in_vert_ids[i]));
		this->verts[in_vert_ids[i]].faces_referencing++;
		f.vert_ids[i] = in_vert_ids[i];
	}

	switch (vert_num) {
	case 3:
		f.internal_id = this->face_indices_tri.size();
		this->face_indices_tri.push_back(face_id);
		break;
	case 4:
		f.internal_id = this->face_indices_quad.size();
		this->face_indices_quad.push_back(face_id);
		break;
	default:
		f.internal_id = this->face_indices_ngon.size();
		this->face_indices_ngon.push_back(face_id);
		break;
	}
}

void mesh::remove_face (int face_id)
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	assert(this->face_exists(face_id));

	face& f = this->faces[face_id];

	for (int i = 0; i < f.num_verts; i++) {
		const int vert_id = f.vert_ids[i];
		vertex& v = this->verts[vert_id];
		assert(v.faces_referencing > 0);
		v.faces_referencing--;
		if (v.faces_referencing == 0) {
			// TODO: For now, just kill "stray" vertices; but since vertex
			// IDs are user-facing, the user code will not expect vertices
			// to which it holds IDs to die, so think of something else
			this->remove_vert(vert_id);
		}
	}

	int other_face_id;
	switch (f.num_verts) {
	case 3:
		other_face_id = this->face_indices_tri.back();
		replace_with_last(this->face_indices_tri, f.internal_id);
		break;
	case 4:
		other_face_id = this->face_indices_quad.back();
		replace_with_last(this->face_indices_quad, f.internal_id);
		break;
	default:
		other_face_id = this->face_indices_ngon.back();
		replace_with_last(this->face_indices_ngon, f.internal_id);
		break;
	}

	if (other_face_id != face_id) {
		assert(this->face_exists(other_face_id));
		this->faces[other_face_id].internal_id = f.internal_id;
	}

	faces_active.clear_bit(face_id);
	f = { .vert_ids = nullptr,
		.num_verts = 0,
		.internal_id = 0 };
}

void mesh::remove_vert (int vert_id)
{
	assert(this->vert_exists(vert_id));
	this->verts_active.clear_bit(vert_id);

	vertex& v = this->verts[vert_id];
	assert(v.faces_referencing == 0);
	v = { .position = vec3(0.0),
	      .faces_referencing = 0 };
}

vec3 mesh::get_vert_pos (int vert_id) const
{
	assert(this->vert_exists(vert_id));
	return this->verts[vert_id].position;
}


static vec3 normalize_or_zero (vec3 v)
{
	return (v == vec3(0.0)) ? v : glm::normalize(v);
}

#define VERT_POS(i) (this->get_vert_pos(f.vert_ids[i]))
vec3 mesh::get_face_normal_tri (int face_id) const
{
	// Simply cross some two edges
	const face& f = this->faces[face_id];
	const vec3 edge1 = VERT_POS(0) - VERT_POS(1);
	const vec3 edge2 = VERT_POS(0) - VERT_POS(2);
	return normalize_or_zero(glm::cross(edge1, edge2));
}

vec3 mesh::get_face_normal_quad (int face_id) const
{
	// Cross the two diagonals (which might themselves
	// not intersect, since the quad is not necessarily planar)
	const face& f = this->faces[face_id];
	const vec3 diag1 = VERT_POS(0) - VERT_POS(2);
	const vec3 diag2 = VERT_POS(1) - VERT_POS(3);
	return normalize_or_zero(glm::cross(diag1, diag2));
}

vec3 mesh::get_face_normal_ngon (int face_id) const
{
	// Find the normal "at" each vertex and average those
	// (no normalization being done meanwhile, the result
	// ends up weighed by edge length, which seems logical anyway)

	const face& f = this->faces[face_id];
	const int n = f.num_verts;

	vec3 edges[n];
	for (int i = 0; i < n; i++)
		edges[i] = VERT_POS(i) - VERT_POS((i + 1) % n);

	vec3 result(0.0);
	for (int i = 0; i < n; i++)
		result += glm::cross(edges[i], edges[(i + 1) % n]);

	return normalize_or_zero(result);
}
#undef VERT_POS

vec3 mesh::get_face_normal (int face_id) const
{
	assert(this->face_exists(face_id));
	switch (this->faces[face_id].num_verts) {
	case 3:
		return this->get_face_normal_tri(face_id);
	case 4:
		return this->get_face_normal_quad(face_id);
	default:
		return this->get_face_normal_ngon(face_id);
	}
}


void mesh::gpu_draw () const
{
	imm::begin(GL_TRIANGLES);

	auto triangle = [this] (int face_id, int a, int b, int c) -> void {
		auto& f = this->faces[face_id];
#define VERT_POS(i) (this->get_vert_pos(f.vert_ids[i]))
		imm::vertex(VERT_POS(a));
		imm::vertex(VERT_POS(b));
		imm::vertex(VERT_POS(c));
#undef VERT_POS
	};

#if 1
	// One way: just render everything as is (only works with immediate)
	for (int face_id = 0; face_id < this->faces.size(); face_id++) {
		if (!this->face_exists(face_id))
			continue;
		imm::normal(this->get_face_normal(face_id));
		for (int i = 2; i < this->faces[face_id].num_verts; i++)
			triangle(face_id, 0, i-1, i);
	}
#else
	// Another way: render triangles, quads, and ngons separately
	for (int face_id: this->face_indices_tri) {
		imm::normal(this->get_face_normal_tri(face_id));
		triangle(face_id, 0, 1, 2);
	}
	for (int face_id: this->face_indices_quad) {
		imm::normal(this->get_face_normal_quad(face_id));
		triangle(face_id, 0, 1, 2);
		triangle(face_id, 0, 2, 3);
	}
	for (int face_id: this->face_indices_ngon) {
		imm::normal(this->get_face_normal_ngon(face_id));
		for (int i = 2; i < this->faces[face_id].num_verts; i++)
			triangle(face_id, 0, i-1, i);
	}
#endif
	imm::end();
}


template <typename print_func>
static void print_sanely (FILE* os, size_t size, print_func f)
{
	constexpr static int MAX_SIZE_FOR_OUTPUT = 256;
	if (size == 0)
		fputs("<none>", os);
	else if ((int) size > MAX_SIZE_FOR_OUTPUT)
		fputs("<a lot>", os);
	else
		f();
}

void mesh::dump_info (FILE* os) const
{
	fprintf(os, "Mesh at %p:\n", this);

	fprintf(os, "Vertices: %i total, active:\n", (int) this->verts.size());
	print_sanely(os, this->verts.size(), [&] { this->verts_active.dump_info(os); });

	fprintf(os, "Faces: %i total, active:\n", (int) this->faces.size());
	print_sanely(os, this->faces.size(), [&] { this->verts_active.dump_info(os); });

	fputs("Ownership indices:", os);

	fputs("\ntris:  ", os);
	print_sanely(os, this->face_indices_tri.size(), [&] {
			for (int i: this->face_indices_tri)
				fprintf(os, "%i ", i);
		});

	fputs("\nquads: ", os);
	print_sanely(os, this->face_indices_quad.size(), [&] {
			for (int i: this->face_indices_quad)
				fprintf(os, "%i ", i);
		});

	fputs("\nngons: ", os);
	print_sanely(os, this->face_indices_ngon.size(), [&] {
			for (int i: this->face_indices_ngon)
				fprintf(os, "%i ", i);
		});
}

} /* namespace map */
