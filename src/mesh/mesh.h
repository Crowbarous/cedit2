#ifndef MESH_H
#define MESH_H

#include "active_bitset.h"
#include "gl.h"
#include "math.h"
#include <array>
#include <set>
#include <vector>

extern GLuint mesh_shader_program_id;
void init_mesh ();

namespace mesh_gpu_constants
{
	/* Must match the uniform locations in the shader */
	constexpr GLuint UNIFORM_LOC_VIEW = 0;
	constexpr GLuint UNIFORM_LOC_PROJ = 16;
}

struct mesh_diff_t;

class mesh_t {
public:
	/* Returned: index of the newly added element */
	int add_vert (vec3 pos);
	int add_face (const int* vert_indices, int num_verts);
	int add_face (std::initializer_list<int> vert_indices);

	void remove_vert (int idx);
	void remove_face (int idx);

	bool vert_exists (int idx) const;
	bool face_exists (int idx) const;
	bool edge_exists (int vert1_idx, int vert2_idx) const;

	vec3 get_vert_position (int vert_idx) const;
	vec3 get_face_normal (int face_idx) const;

	void gpu_sync (); /* Takes care of initialization if needed */
	void gpu_draw () const;

	void gpu_init ();
	void gpu_deinit ();
	bool gpu_initialized () const;

	void debug_dump_info () const;

private:
	struct vert_t {
		vec3 position;

		/* Faces of which this vert is part */
		struct adj_face_t {
			int face_idx;
			/* IDs of neighbouring vertices in this face */
			int prev_vert_idx;
			int next_vert_idx;
		};
		std::vector<adj_face_t> adj_faces;

		vert_t (vec3 pos): position(pos) { }
	};

	struct face_t {
		std::vector<int> vert_idx;

		/* Indices in VRAM of triangles which belong to this face */
		std::vector<int> gpu_triangles;

		int& operator[] (int i) { return vert_idx[i]; }
		const int& operator[] (int i) const { return vert_idx[i]; }
	};

	std::vector<vert_t> verts;
	std::vector<face_t> faces;

	using active_set = active_bitset64;
	active_set verts_active;
	active_set faces_active;

	/* Rendering the mesh */

	/* The layout of the rendered data in VRAM */
	struct gpu_vert_t {
		vec3 position;
		vec3 normal;
	};
	using gpu_triangle_t = std::array<gpu_vert_t, 3>;

	struct gpu_info_t {
		/* We don't use index buffers because we want flat shading */
		GLuint vao_id = 0; /* Zero indicates mesh not being uploaded */
		GLuint vbo_id = 0;
		int vbo_capacity; /* The actual size of VBO */

		/*
		 * For each triangle, store to which face it belongs.
		 * size() of this vector is the number of triangles to draw
		 */
		std::vector<int> triangle_which_face;

		/* Which faces need to be reuploaded next sync */
		std::set<int> dirty_faces;

		/* Vertex format specification */

		constexpr static vert_attribute_t va_pos =
			{ .location = 0,
			  .num_components = 3,
			  .data_type = GL_FLOAT,
			  .normalize = false };

		constexpr static vert_attribute_t va_norm =
			{ .location = 1,
			  .num_components = 3,
			  .data_type = GL_FLOAT,
			  .normalize = true };
	} gpu;

	void gpu_apply_attributes_to_vao () const;

	int gpu_get_triangles_num () const;
	int gpu_get_triangles_capacity () const;

	void gpu_add_face (int idx);
	void gpu_remove_face (int idx);
	void gpu_mark_face_dirty (int idx);
};

void mesh_add_cuboid (mesh_t& mesh, vec3 center, vec3 side_lengths);

#endif // MESH_H
