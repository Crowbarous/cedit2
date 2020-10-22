#ifndef MAP_EDIT_H
#define MAP_EDIT_H

#include "active_bitset.h"
#include "math.h"
#include "gl.h"
#include <vector>

class map_piece_mesh {
public:
	int add_vertex (vec3 position);
	int add_face (const int* vert_ids, int vert_num);

	void remove_face (int face_id);
	vec3 get_face_normal (int face_id) const;

	void gpu_draw () const;
	void gpu_init ();
	void gpu_deinit ();
	void gpu_sync ();

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
		int verts[3];
		int face_id;
	};
	struct face_quad {
		int verts[4];
		int face_id;
	};
	struct face_ngon {
		int* verts;
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

	/*
	 * ================== GPU STUFF ==================
	 */

	bool gpu_is_initialized () const { return this->gpu_tri.vao != 0; }
	struct gpu_drawable {
		GLuint vao = 0;
		GLuint vbo = 0;
		void init ();
		void deinit ();
		void bind () const;
	};
	gpu_drawable gpu_tri;
	gpu_drawable gpu_quad;
	gpu_drawable gpu_ngon;

	struct gpu_vertex {
		vec3 position;
		vec3 normal;
	};

	static void gpu_set_vertex_format ();
	void gpu_dump_tri (int tri_id, gpu_vertex* dest);
	void gpu_dump_quad (int quad_id, gpu_vertex* dest);
	void gpu_dump_ngon (int ngon_id, gpu_vertex* dest);
};

namespace mesh_shader_attrib_loc
{
constexpr static int POSITION = 0;
constexpr static int NORMAL = 1;
};

#endif /* MAP_EDIT_H */
