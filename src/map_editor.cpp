#include "map_editor.h"
#include "gl_immediate.h"
#include <cassert>

int map_piece_mesh::add_vertex (vec3 position)
{
	int vert_id = this->verts_active.set_first_cleared();
	assert(vert_id <= this->verts.size());
	if (vert_id == this->verts.size())
		this->verts.emplace_back();

	this->verts[vert_id] = { .position = position };

	return vert_id;
}

int map_piece_mesh::add_face (const int* vert_ids, int vert_num)
{
	assert(vert_num >= 3);

	int internal_face_id;
	face_type_t face_type;

	const int face_id = this->faces_active.set_first_cleared();

	switch (vert_num) {
	case 3: {
		face_type = TRIANGLE;
		internal_face_id = this->faces_tri.size();
		this->faces_tri.emplace_back();
		face_tri& tri = this->faces_tri.back();

		tri.face_id = face_id;
		memcpy(tri.vert_ids, vert_ids, sizeof(int) * 3);

		break;
	}
	case 4: {
		face_type = QUAD;
		internal_face_id = this->faces_tri.size();
		this->faces_quad.emplace_back();
		face_quad& quad = this->faces_quad.back();

		quad.face_id = face_id;
		memcpy(quad.vert_ids, vert_ids, sizeof(int) * 4);

		break;
	}
	default: {
		face_type = NGON;
		internal_face_id = this->faces_ngon.size();
		this->faces_ngon.emplace_back();
		face_ngon& ngon = this->faces_ngon.back();

		ngon.face_id = face_id;
		ngon.vert_ids = new int[vert_num];
		ngon.num_verts = vert_num;
		memcpy(ngon.vert_ids, vert_ids, sizeof(int) * vert_num);

		break;
	}
	}

	assert(face_id <= this->faces.size());
	if (face_id == this->faces.size())
		this->faces.emplace_back();
	this->faces[face_id] = { .type = face_type,
	                         .internal_id = internal_face_id };

	return face_id;
}

void map_piece_mesh::remove_face (int face_id)
{
	assert(this->face_exists(face_id));
	face& f = this->faces[face_id];

	int other_face_id = -1;

	switch (f.type) {
	case TRIANGLE: {
		other_face_id = this->faces_tri.back().face_id;
		face& other_face = this->faces[other_face_id];
		other_face.internal_id = f.internal_id;
		assert(other_face.type == TRIANGLE);

		this->faces_tri[f.internal_id] = this->faces_tri.back();
		this->faces_tri.pop_back();

		break;
	}
	case QUAD: {
		other_face_id = this->faces_quad.back().face_id;
		face& other_face = this->faces[other_face_id];
		other_face.internal_id = f.internal_id;
		assert(other_face.type == QUAD);

		this->faces_quad[f.internal_id] = this->faces_quad.back();
		this->faces_quad.pop_back();

		break;
	}
	case NGON: {
		other_face_id = this->faces_ngon.back().face_id;
		face& other_face = this->faces[other_face_id];
		other_face.internal_id = f.internal_id;
		assert(other_face.type == NGON);

		this->faces_ngon[f.internal_id] = this->faces_ngon.back();
		this->faces_ngon.pop_back();

		break;
	}
	}
	assert(this->face_exists(other_face_id));

	faces_active.clear_bit(face_id);
}


vec3 map_piece_mesh::get_face_normal (int face_id) const
{
	assert(this->face_exists(face_id));
	const face& f = this->faces[face_id];
	switch (f.type) {
	case TRIANGLE:
		return this->get_face_normal_tri(f.internal_id);
	case QUAD:
		return this->get_face_normal_quad(f.internal_id);
	case NGON: default:
		return this->get_face_normal_ngon(f.internal_id);
	}
}

vec3 map_piece_mesh::get_face_normal_tri (int id) const
{
	const face_tri& tri = this->faces_tri[id];
	int a = tri.vert_ids[0];
	int b = tri.vert_ids[1];
	int c = tri.vert_ids[2];
	vec3 delta1 = this->verts[a].position - this->verts[b].position;
	vec3 delta2 = this->verts[a].position - this->verts[c].position;
	return glm::normalize(glm::cross(delta1, delta2));
}

vec3 map_piece_mesh::get_face_normal_quad (int id) const
{
	// Find the vectors of two diagonals (which might themselves not
	// intersect, since the quad is not necessarily planar) and cross them
	const face_quad& quad = this->faces_quad[id];
	int a = quad.vert_ids[0];
	int b = quad.vert_ids[1];
	int c = quad.vert_ids[2];
	int d = quad.vert_ids[3];
	vec3 delta1 = this->verts[a].position - this->verts[c].position;
	vec3 delta2 = this->verts[b].position - this->verts[d].position;
	return glm::normalize(glm::cross(delta1, delta2));
}

vec3 map_piece_mesh::get_face_normal_ngon (int id) const
{
	// Find the normal "at" each vertex and average those
	const face_ngon& ngon = this->faces_ngon[id];
	const int n = ngon.num_verts;

	vec3 edge_vectors[n];
	for (int i = 0; i < n; i++) {
		edge_vectors[i] = this->verts[ngon.vert_ids[i]].position
		                - this->verts[ngon.vert_ids[(i + 1) % n]].position;
	}

	vec3 result(0.0);
	for (int i = 0; i < n; i++)
		result += glm::cross(edge_vectors[i], edge_vectors[(i + 1) % n]);

	return glm::normalize(result);
}

void map_piece_mesh::gpu_draw () const
{
	auto draw_tri = [this] (const int* vert_ids, int a, int b, int c) {
		imm_begin(GL_TRIANGLES);
		imm_vertex(this->verts[vert_ids[a]].position);
		imm_vertex(this->verts[vert_ids[b]].position);
		imm_vertex(this->verts[vert_ids[c]].position);
		imm_end();
	};

	for (int i = 0; i < this->faces_tri.size(); i++) {
		imm_normal(this->get_face_normal_tri(i));
		draw_tri(this->faces_tri[i].vert_ids, 0, 1, 2);
	}

	for (int i = 0; i < this->faces_quad.size(); i++) {
		const face_quad& quad = this->faces_quad[i];
		imm_normal(this->get_face_normal_quad(i));
		draw_tri(quad.vert_ids, 0, 1, 2);
		draw_tri(quad.vert_ids, 0, 2, 3);
	}

	for (int i = 0; i < this->faces_ngon.size(); i++) {
		const face_ngon& ngon = this->faces_ngon[i];
		imm_normal(this->get_face_normal_ngon(i));
		for (int j = 2; j < ngon.num_verts; j++)
			draw_tri(ngon.vert_ids, 0, j-1, j);
	}
}
