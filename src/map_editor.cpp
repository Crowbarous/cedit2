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

void map_piece_mesh::gpu_draw () const
{
	imm_begin(GL_TRIANGLES);

	for (const face_tri& tri: this->faces_tri) {
		imm_normal({ 1.0, 1.0, 1.0 });
		for (int i = 0; i < 3; i++)
			imm_vertex(this->verts[tri.vert_ids[i]].position);
	}

	for (const face_quad& quad: this->faces_quad) {
		imm_normal({ 0.0, 0.0, 1.0 });

		auto tri = [this, &quad] (int a, int b, int c) -> void {
			imm_vertex(this->verts[quad.vert_ids[a]].position);
			imm_vertex(this->verts[quad.vert_ids[b]].position);
			imm_vertex(this->verts[quad.vert_ids[c]].position);
		};

		tri(0, 1, 2);
		tri(0, 2, 3);
	}

	for (const face_ngon& ngon: this->faces_ngon) {
		imm_normal({ 0.0, 1.0, 1.0 });

		auto tri = [this, &ngon] (int a, int b, int c) -> void {
			imm_vertex(this->verts[ngon.vert_ids[a]].position);
			imm_vertex(this->verts[ngon.vert_ids[b]].position);
			imm_vertex(this->verts[ngon.vert_ids[c]].position);
		};

		for (int i = 2; i < ngon.num_verts; i++)
			tri(0, i-1, i);
	}

	imm_end();
}
