#include "map_editor.h"
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
	face::face_type_t face_type;

	switch (vert_num) {
	case 3: {
		face_type = face::TRIANGLE;
		internal_face_id = this->faces_tri.size();
		this->faces_tri.emplace_back();
		face_tri& tri = this->faces_tri.back();

		memcpy(tri.vert_ids, vert_ids, sizeof(int) * 3);

		break;
	}
	case 4: {
		face_type = face::QUAD;
		internal_face_id = this->faces_tri.size();
		this->faces_quad.emplace_back();
		face_quad& quad = this->faces_quad.back();

		memcpy(quad.vert_ids, vert_ids, sizeof(int) * 4);

		break;
	}
	default: {
		face_type = face::NGON;
		internal_face_id = this->faces_ngon.size();
		this->faces_ngon.emplace_back();
		face_ngon& ngon = this->faces_ngon.back();

		ngon.vert_ids = new int[vert_num];
		ngon.num_verts = vert_num;
		memcpy(ngon.vert_ids, vert_ids, sizeof(int) * vert_num);

		break;
	}
	}

	int face_id = this->faces_active.set_first_cleared();
	assert(face_id <= this->faces.size());
	if (face_id == this->faces.size())
		this->faces.emplace_back();

	this->faces[face_id] = { .type = face_type,
	                         .id = internal_face_id };

	return face_id;
}
