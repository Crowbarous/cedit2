#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "active_bitset.h"
#include "math.h"
#include "gl.h"
#include <vector>

class map_piece_mesh {

	struct vertex {
		vec3 position;
	};
	std::vector<vertex> verts;
	active_bitset verts_active;

	struct face {
		enum face_type_t: uint8_t {
			QUAD,
			TRIANGLE,
			NGON,
		} type;
		int id; /* Depending on `type`, is an index into different arrays */
	};
	std::vector<face> faces;
	active_bitset faces_active;

	struct face_tri { int vert_ids[3]; };
	struct face_quad { int vert_ids[4]; };
	struct face_ngon {
		int* vert_ids;
		int num_verts;
	};

	std::vector<face_tri> faces_tri;
	std::vector<face_quad> faces_quad;
	std::vector<face_ngon> faces_ngon;

	active_bitset faces_tri_active;
	active_bitset faces_quad_active;
	active_bitset faces_ngon_active;

	bool vert_exists (int i) const { return verts_active.bit_is_set(i); }
	bool face_exists (int i) const { return faces_active.bit_is_set(i); }
};

#endif // MAP_EDITOR_H
