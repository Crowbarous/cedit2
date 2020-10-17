#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "active_bitset.h"
#include "math.h"
#include <vector>

class map_piece_mesh {
public:
	int add_vertex (vec3 position);
	int add_face (const int* vert_ids, int vert_num);

	void remove_face (int face_id);
	vec3 get_face_normal (int face_id) const;

	void gpu_draw () const;

private:
	struct vertex {
		vec3 position;
	};
	std::vector<vertex> verts;
	active_bitset verts_active;

	enum face_type_t: uint8_t {
		TRIANGLE,
		QUAD,
		NGON,
	};
	struct face {
		face_type_t type;
		/* Depending on `type`, is an index into different arrays */
		int internal_id;
	};
	std::vector<face> faces;
	active_bitset faces_active;

	struct face_tri {
		int vert_ids[3];
		int face_id;
	};
	struct face_quad {
		int vert_ids[4];
		int face_id;
	};
	struct face_ngon {
		int* vert_ids;
		int num_verts;
		int face_id;
	};

	vec3 get_face_normal_tri (int internal_id) const;
	vec3 get_face_normal_quad (int internal_id) const;
	vec3 get_face_normal_ngon (int internal_id) const;

	std::vector<face_tri> faces_tri;
	std::vector<face_quad> faces_quad;
	std::vector<face_ngon> faces_ngon;

	bool vert_exists (int i) const { return this->verts_active.bit_is_set(i); }
	bool face_exists (int i) const { return this->faces_active.bit_is_set(i); }
};

#endif /* MAP_EDITOR_H */
