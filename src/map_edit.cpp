#include "map_edit.h"
#include "active_bitset.h"
#include "gl.h"
#include "gl_immediate.h"
#include "util.h"
#include <cassert>
#include <vector>

struct vertex {
	vec3 position;
};

/*
 * Faces are treated differently depending on whether they are triangles,
 * quads, or ngons. (They end up in different GPU buffers, too)
 */
enum face_type_t {
	TRIANGLE,
	QUAD,
	NGON,
};

struct face {
	face_type_t type;
	/* Depending on `type`, is an index into different arrays */
	int internal_id;
};

/*
 * There will of course be code that is common to all or two of the face types.
 * Such code can rely on structs having members with the names
 *   `face_id`, `verts`, `num_verts`
 */
template <face_type_t> struct face_internal;
template <> struct face_internal<TRIANGLE> {
	constexpr static int num_verts = 3;
	int verts[num_verts]; /* Indices into `map_mesh::verts` */
	int face_id; /* Which `struct face` references this */

	face_internal (int _) { }
};
template <> struct face_internal<QUAD> {
	constexpr static int num_verts = 4;
	int verts[num_verts];
	int face_id;

	face_internal (int _) { }
};
template <> struct face_internal<NGON> {
	int* verts;
	int num_verts;
	int face_id;

	face_internal (int nv)
		: verts(new int[nv]),
		  num_verts(nv) { }
	face_internal& operator= (const face_internal& other)
	{
		if (this == &other)
			return *this;
		delete[] this->verts;
		this->verts = other.verts;
		this->num_verts = other.num_verts;
		this->face_id = other.face_id;
		return *this;
	}
	~face_internal ()
	{
		delete[] this->verts;
	}
};

using face_tri = face_internal<TRIANGLE>;
using face_quad = face_internal<QUAD>;
using face_ngon = face_internal<NGON>;


struct map_mesh {
	std::vector<vertex> verts;
	active_bitset verts_active;

	std::vector<face> faces;
	active_bitset faces_active;

	bool vert_exists (int i) const { return verts_active.bit_is_set(i); }
	bool face_exists (int i) const { return faces_active.bit_is_set(i); }

	std::vector<face_tri> faces_tri;
	std::vector<face_quad> faces_quad;
	std::vector<face_ngon> faces_ngon;
	/* Cannot have template member variables, so use a template getter */
	template <face_type_t FACE_TYPE>
	std::vector<face_internal<FACE_TYPE>>& get_internal_face_vector ();

	vec3 get_face_normal_tri (int internal_face_id) const;
	vec3 get_face_normal_quad (int internal_face_id) const;
	vec3 get_face_normal_ngon (int internal_face_id) const;
};



template <> [[always_inline]]
std::vector<face_tri>& map_mesh::get_internal_face_vector<TRIANGLE> ()
{
	return this->faces_tri;
}
template <> [[always_inline]]
std::vector<face_quad>& map_mesh::get_internal_face_vector<QUAD> ()
{
	return this->faces_quad;
}
template <> [[always_inline]]
std::vector<face_ngon>& map_mesh::get_internal_face_vector<NGON> ()
{
	return this->faces_ngon;
}

map_mesh* mesh_new ()
{
	return new map_mesh;
}

void mesh_delete (map_mesh*& m)
{
	delete m;
	m = nullptr;
}

/* Adding elements */

int mesh_add_vertex (map_mesh* m, vec3 position)
{
	int vert_id = m->verts_active.set_first_cleared();
	assert(vert_id <= m->verts.size());
	if (vert_id == m->verts.size())
		m->verts.emplace_back();

	m->verts[vert_id] = { .position = position };
	return vert_id;
}

template <face_type_t FACE_TYPE>
static int mesh_add_face_low (
		map_mesh* m,
		const int* vert_ids,
		int vert_num,
		face_type_t& face_type_var)
{
	face_type_var = FACE_TYPE;

	auto& vec = m->get_internal_face_vector<FACE_TYPE>();
	int ret = vec.size();
	vec.emplace_back(vert_num);
	auto& f = vec.back();

	memcpy(f.verts, vert_ids, sizeof(int) * vert_num);

	return ret;
}

int mesh_add_face (map_mesh* m, const int* vert_ids, int vert_num)
{
	assert(vert_num >= 3);

	const int face_id = m->faces_active.set_first_cleared();
	face_type_t face_type;

	int id;
	switch (vert_num) {
	case 3:
		id = mesh_add_face_low<TRIANGLE>(m, vert_ids, vert_num, face_type);
		break;
	case 4:
		id = mesh_add_face_low<QUAD>(m, vert_ids, vert_num, face_type);
		break;
	default:
		id = mesh_add_face_low<NGON>(m, vert_ids, vert_num, face_type);
		break;
	};

	assert(face_id <= m->faces.size());
	if (face_id == m->faces.size())
		m->faces.emplace_back();
	m->faces[face_id] = { .type = face_type,
	                      .internal_id = id };

	return face_id;
}

/* Removing elements */

template <face_type_t FACE_TYPE>
static void mesh_remove_face_low (map_mesh* m, int internal_id)
{
	auto& vec = m->get_internal_face_vector<FACE_TYPE>();
	face_internal<FACE_TYPE>& fi = vec.back();

	const int other_face_id = fi.face_id;
	face& other_face = m->faces[other_face_id];
	assert(m->face_exists(other_face_id));
	assert(other_face.type == FACE_TYPE);
	other_face.internal_id = internal_id;

	vec[internal_id] = vec.back();
	vec.pop_back();
}

void mesh_remove_face (map_mesh* m, int face_id)
{
	assert(m->face_exists(face_id));

	face& f = m->faces[face_id];
	switch (f.type) {
	case TRIANGLE:
		mesh_remove_face_low<TRIANGLE>(m, f.internal_id);
		break;
	case QUAD:
		mesh_remove_face_low<QUAD>(m, f.internal_id);
		break;
	case NGON:
		mesh_remove_face_low<NGON>(m, f.internal_id);
		break;
	}

	m->faces_active.clear_bit(face_id);
}

/* Face normals */

vec3 map_mesh::get_face_normal_tri (int id) const
{
	const face_tri& tri = this->faces_tri[id];
	int a = tri.verts[0];
	int b = tri.verts[1];
	int c = tri.verts[2];
	vec3 delta1 = this->verts[a].position - this->verts[b].position;
	vec3 delta2 = this->verts[a].position - this->verts[c].position;
	return glm::normalize(glm::cross(delta1, delta2));
}

vec3 map_mesh::get_face_normal_quad (int id) const
{
	// Find the vectors of two diagonals (which might themselves not
	// intersect, since the quad is not necessarily planar) and cross them
	const face_quad& quad = this->faces_quad[id];
	int a = quad.verts[0];
	int b = quad.verts[1];
	int c = quad.verts[2];
	int d = quad.verts[3];
	vec3 delta1 = this->verts[a].position - this->verts[c].position;
	vec3 delta2 = this->verts[b].position - this->verts[d].position;
	return glm::normalize(glm::cross(delta1, delta2));
}

vec3 map_mesh::get_face_normal_ngon (int id) const
{
	// Find the normal "at" each vertex and average those
	const face_ngon& ngon = this->faces_ngon[id];
	const int n = ngon.num_verts;

	vec3 edge_vectors[n];
	for (int i = 0; i < n; i++) {
		edge_vectors[i] = this->verts[ngon.verts[i]].position
			- this->verts[ngon.verts[(i + 1) % n]].position;
	}

	vec3 result(0.0);
	for (int i = 0; i < n; i++)
		result += glm::cross(edge_vectors[i], edge_vectors[(i + 1) % n]);

	return glm::normalize(result);
}

vec3 mesh_get_face_normal (map_mesh* m, int face_id)
{
	assert(m->face_exists(face_id));
	face& f = m->faces[face_id];
	switch (f.type) {
	case TRIANGLE:
		return m->get_face_normal_tri(f.internal_id);
	case QUAD:
		return m->get_face_normal_quad(f.internal_id);
	case NGON:
	default:
		return m->get_face_normal_ngon(f.internal_id);
	}
}

/* ============================== GPU STUFF ============================== */

void mesh_gpu_draw (const map_mesh* m)
{
	imm_begin(GL_TRIANGLES);

	auto draw_tri = [m] (const int* vert_ids, int a, int b, int c) {
		imm_vertex(m->verts[vert_ids[a]].position);
		imm_vertex(m->verts[vert_ids[b]].position);
		imm_vertex(m->verts[vert_ids[c]].position);
	};

	for (int i = 0; i < m->faces_tri.size(); i++) {
		imm_normal(m->get_face_normal_tri(i));
		draw_tri(m->faces_tri[i].verts, 0, 1, 2);
	}

	for (int i = 0; i < m->faces_quad.size(); i++) {
		const face_quad& quad = m->faces_quad[i];
		imm_normal(m->get_face_normal_quad(i));
		draw_tri(quad.verts, 0, 1, 2);
		draw_tri(quad.verts, 0, 2, 3);
	}

	for (int i = 0; i < m->faces_ngon.size(); i++) {
		const face_ngon& ngon = m->faces_ngon[i];
		imm_normal(m->get_face_normal_ngon(i));
		for (int j = 2; j < ngon.num_verts; j++)
			draw_tri(ngon.verts, 0, j-1, j);
	}

	imm_end();
}
