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

	bool vert_exists (int i) const { return verts_active.bit_is_set(i); }
	bool face_exists (int i) const { return faces_active.bit_is_set(i); }
};

#endif // MAP_EDITOR_H
