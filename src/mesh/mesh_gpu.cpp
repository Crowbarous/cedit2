#include "mesh.h"
#include "util.h"

GLuint mesh_shader_program_id = 0;
void init_mesh ()
{
	GLuint shaders[2] =
		{ glsl_load_shader("mesh.vert", GL_VERTEX_SHADER),
		  glsl_load_shader("mesh.frag", GL_FRAGMENT_SHADER) };

	mesh_shader_program_id = glsl_link_program(shaders, 2);

	for (GLuint& s: shaders)
		glsl_delete_shader(s);
}

bool mesh_t::gpu_initialized () const
{
	return (gpu.vao_id != 0);
}

void mesh_t::gpu_init ()
{
	assert(!gpu_initialized());

	glGenVertexArrays(1, &gpu.vao_id);
	assert(gpu.vao_id != 0);
	glBindVertexArray(gpu.vao_id);

	glGenBuffers(1, &gpu.vbo_id);
	assert(gpu.vbo_id != 0);
	glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo_id);

	std::vector<gpu_triangle_t> buffer_data;

	for (int face_idx = 0; face_idx < faces.size(); face_idx++) {
		if (!face_exists(face_idx))
			continue;

		face_t& f = faces[face_idx];
		vec3 face_normal = get_face_normal(face_idx);

		// pan triangulation; flat shading

		const auto& vi = f.vert_idx;

		auto make_vert = [&] (int i) -> gpu_vert_t {
			return { .position = verts[vi[i]].position,
			         .normal = face_normal };
		};

		for (int i = 2; i < vi.size(); i++) {
			buffer_data.push_back(
					{ make_vert(0),
					  make_vert(i),
					  make_vert(i-1) });

			int tri_idx = gpu.triangle_which_face.size();
			f.gpu_triangles.push_back(tri_idx);

			gpu.triangle_which_face.push_back(face_idx);
		}
	}

	gpu.vbo_capacity = ceil_po2(buffer_data.size());
	buffer_data.reserve(gpu.vbo_capacity);

	glBufferData(GL_ARRAY_BUFFER,
			sizeof(gpu_triangle_t) * gpu.vbo_capacity,
			buffer_data.data(), GL_DYNAMIC_DRAW);

	gpu.va_pos.send_attrib(sizeof(gpu_vert_t), offsetof(gpu_vert_t, position));
	gpu.va_norm.send_attrib(sizeof(gpu_vert_t), offsetof(gpu_vert_t, normal));
}

void mesh_t::gpu_deinit ()
{
	glDeleteVertexArrays(1, &gpu.vao_id);
	gpu.vao_id = 0;

	glDeleteBuffers(1, &gpu.vbo_id);
	gpu.vbo_id = 0;

	gpu.triangle_which_face.clear();
	gpu.dirty_faces.clear();
}

void mesh_t::gpu_sync ()
{
	if (!gpu_initialized()) {
		gpu_init();
		return;
	}
}

void mesh_t::gpu_add_face (int face_idx)
{
	if (!gpu_initialized())
		return;
	assert(face_exists(face_idx));

	face_t& f = faces[face_idx];
	assert(f.gpu_triangles.empty());

	int face_num_tris = f.vert_idx.size()-2;
	f.gpu_triangles.resize(face_num_tris);

	int old_num_tris = gpu_get_triangles_num();
	int new_num_tris = old_num_tris + face_num_tris;

	// append this face's triangles to the end of the buffer

	gpu.triangle_which_face.resize(new_num_tris);

	for (int i = 0; i < face_num_tris; i++) {
		int tri_idx = old_num_tris + i;
		gpu.triangle_which_face[tri_idx] = face_idx;
		f.gpu_triangles[i] = tri_idx;
	}

	gpu_mark_face_dirty(face_idx);
}

void mesh_t::gpu_remove_face (int face_idx)
{
	if (!gpu_initialized())
		return;
	assert(face_exists(face_idx));

	face_t& f = faces[face_idx];
	int face_num_tris = f.gpu_triangles.size();
	assert(face_num_tris == f.vert_idx.size()-2);

	int old_num_tris = gpu_get_triangles_num();
	int new_num_tris = old_num_tris - face_num_tris;
	assert(new_num_tris >= 0);

	// Take `face_num_tris` triangles at the end and move them
	// where this face's triangles used to be

	std::set<int> faces_affected;

	for (int i = new_num_tris; i < old_num_tris; i++)
		faces_affected.insert(gpu.triangle_which_face[i]);

	for (int other_face_idx: faces_affected) {
		assert(face_exists(other_face_idx));
		gpu_mark_face_dirty(other_face_idx);

		for (int& tri_idx: faces[other_face_idx].gpu_triangles) {
			if (tri_idx >= new_num_tris) {
				tri_idx = f.gpu_triangles.back();
				f.gpu_triangles.pop_back();
				gpu.triangle_which_face[tri_idx] = other_face_idx;
			}
		}
	}

	gpu.triangle_which_face.resize(new_num_tris);

	assert(f.gpu_triangles.empty());
}

void mesh_t::gpu_draw () const
{
	glBindVertexArray(gpu.vao_id);
	glDrawArrays(GL_TRIANGLES, 0, 3 * gpu.triangle_which_face.size());
}

int mesh_t::gpu_get_triangles_num () const
{
	return gpu.triangle_which_face.size();
}

int mesh_t::gpu_get_triangles_capacity () const
{
	return gpu.vbo_capacity;
}

void mesh_t::gpu_mark_face_dirty (int face_idx)
{
	gpu.dirty_faces.insert(face_idx);
}
