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

	void gpu_init ();
	void gpu_deinit ();
	void gpu_sync ();
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
	 * These receive `face_ids` and not `internal_id`s, because
	 * the vert index arrays are in the faces themselves!
	 */
	vec3 get_face_normal_tri (int face_id) const;
	vec3 get_face_normal_quad (int face_id) const;
	vec3 get_face_normal_ngon (int face_id) const;

	/* ======================= GPU STUFF ======================= */

	struct gpu_vertex {
		vec3 position;
		vec3 normal;
		int face_id;
	};

	bool gpu_is_initialized () const;
	static void gpu_set_attrib_pointers ();
	void gpu_dump_face_tri (gpu_vertex*, int face_id) const;
	void gpu_dump_face_quad (gpu_vertex*, int face_id) const;
	void gpu_dump_face_ngon (gpu_vertex*, int face_id) const;

	struct gpu_drawn_buffer {
		GLuint vertex_array = 0;
		GLuint buffer = 0;
		gpu_vertex* mapped = nullptr;

		int size; /* Both in `gpu_vertex`es, not in bytes */
		int capacity;

		void init (const gpu_vertex* initial_data, int initial_size);
		void deinit ();
		void draw () const;
	};

	gpu_drawn_buffer gpu_tris;
	gpu_drawn_buffer gpu_quads;
	gpu_drawn_buffer gpu_ngons;
};

/*
 * The vertex format for drawing the mesh is exposed so that user code
 * knows what to expect in the shaders (which mesh code doesn't control)
 */

namespace mesh_vertex_attrib
{
constexpr int POSITION_LOC = 0;
constexpr int POSITION_NUM_ELEMS = 3;
constexpr GLenum POSITION_DATA_TYPE = GL_FLOAT;

constexpr int NORMAL_LOC = 1;
constexpr int NORMAL_NUM_ELEMS = 3;
constexpr GLenum NORMAL_DATA_TYPE = GL_FLOAT;

constexpr int FACEID_LOC = 2;
constexpr int FACEID_NUM_ELEMS = 1;
constexpr GLenum FACEID_DATA_TYPE = GL_INT;
};

} /* namespace map */

#endif /* MAP_EDIT_H */
