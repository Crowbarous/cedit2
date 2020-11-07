#ifndef MAP_EDIT_H
#define MAP_EDIT_H

#include "active_bitset.h"
#include "gl.h"
#include "math.h"
#include "util.h"

namespace map
{

class mesh {
public:
	int add_vertex (const vec3& position);

	int add_face (const int* vert_ids, int vert_num);
	int add_face (std::initializer_list<int> vert_ids);

	void remove_face (int face_id);

	void gpu_draw () const;

	vec3 get_vert_pos (int vert_id) const;
	vec3 get_face_normal (int face_id) const;

	mesh ();
	~mesh ();
	mesh (const mesh&) = delete;
	mesh (mesh&&) = delete;
	mesh& operator= (const mesh&) = delete;
	mesh& operator= (mesh&&) = delete;

	void dump_info (FILE* outstream) const;

private:
	struct vertex {
		vec3 position;
		int faces_referencing;
	};
	std::vector<vertex> verts;
	active_bitset verts_active;

	struct face {
		int* vert_ids;
		int num_verts;
		int internal_id; /* Index into the appropriate face_indices_* */
	};
	std::vector<face> faces;
	active_bitset faces_active;

	std::vector<int> face_indices_tri;
	std::vector<int> face_indices_quad;
	std::vector<int> face_indices_ngon;

	void construct_face_at_id (int, const int*, int);
	void remove_vert (int vert_id);

	bool vert_exists (int i) const { return this->verts_active.bit_is_set(i); }
	bool face_exists (int i) const { return this->faces_active.bit_is_set(i); }

	/*
	 * These STILL receive `face_ids` and not `internal_id`s, because
	 * the vert index arrays are in the faces themselves!
	 */
	vec3 get_face_normal_tri (int face_id) const;
	vec3 get_face_normal_quad (int face_id) const;
	vec3 get_face_normal_ngon (int face_id) const;
};

/* For rendering the meshes */

namespace mesh_vertex_attrib
{
constexpr int POSITION_LOC = 0;
constexpr int POSITION_NUM_ELEMS = 3;
constexpr GLenum POSITION_DATA_TYPE = GL_FLOAT;

constexpr int NORMAL_LOC = 1;
constexpr int NORMAL_NUM_ELEMS = 3;
constexpr GLenum NORMAL_DATA_TYPE = GL_FLOAT;
};

} /* namespace map */

#endif /* MAP_EDIT_H */
