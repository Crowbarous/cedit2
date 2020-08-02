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
		const vec3 face_normal = get_face_normal(face_idx);

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

	gpu_apply_attributes_to_vao();
}

void mesh_t::gpu_apply_attributes_to_vao () const
{
	gpu.va_pos.send_attrib(sizeof(gpu_vert_t), offsetof(gpu_vert_t, position));
	gpu.va_norm.send_attrib(sizeof(gpu_vert_t), offsetof(gpu_vert_t, normal));
}

void mesh_t::gpu_deinit ()
{
	glDeleteVertexArrays(1, &gpu.vao_id);
	gpu.vao_id = 0;

	glDeleteBuffers(1, &gpu.vbo_id);
	gpu.vbo_id = 0;

	gpu.vbo_capacity = 0;

	gpu.triangle_which_face.clear();
	gpu.dirty_faces.clear();
}

void mesh_t::gpu_sync ()
{
	if (!gpu_initialized()) {
		gpu_init();
		return;
	}

	int num_triangles = gpu_get_triangles_num();

	if (num_triangles > gpu.vbo_capacity) {
		// We have to grow the VBO

		int old_capacity = gpu.vbo_capacity;
		gpu.vbo_capacity = ceil_po2(num_triangles);

		// Recreate the buffer with new capacity
		glBindVertexArray(gpu.vao_id);

		GLuint vbo_id_new;
		glGenBuffers(1, &vbo_id_new);
		assert(vbo_id_new > 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_id_new);
		glBufferData(GL_ARRAY_BUFFER,
				sizeof(gpu_triangle_t) * gpu.vbo_capacity,
				nullptr, GL_DYNAMIC_DRAW);

		// Copy in the data: old VBO is the source
		glBindBuffer(GL_COPY_READ_BUFFER, gpu.vbo_id);

		glCopyBufferSubData(
				GL_COPY_READ_BUFFER, // src: old VBO bound
				GL_ARRAY_BUFFER,     // dest: new VBO bound
				0, 0, sizeof(gpu_triangle_t) * old_capacity);

		// Delete old VBO
		glDeleteBuffers(1, &gpu.vbo_id);
		gpu.vbo_id = vbo_id_new;

		gpu_apply_attributes_to_vao();
	}

	if (gpu.dirty_faces.empty())
		return;

	glBindVertexArray(gpu.vao_id);
	gpu_triangle_t* buf_data = (gpu_triangle_t*)
		glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	assert(buf_data != nullptr);

	for (int face_idx: gpu.dirty_faces) {
		assert(face_exists(face_idx));

		face_t& f = faces[face_idx];
		assert(f.gpu_triangles.size() == f.vert_idx.size()-2);

		const vec3 face_normal = get_face_normal(face_idx);
		const auto& vi = f.vert_idx;

		auto make_vert = [&] (int i) -> gpu_vert_t {
			return { .position = verts[vi[i]].position,
				 .normal = face_normal };
		};

		for (int i = 2; i < vi.size(); i++) {
			buf_data[f.gpu_triangles[i-2]] =
				{ make_vert(0),
				  make_vert(i),
				  make_vert(i-1) };
		}
	}

	gpu.dirty_faces.clear();

	GLboolean unmap_ok = glUnmapBuffer(GL_ARRAY_BUFFER);
	assert(unmap_ok == GL_TRUE);
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

	// Append this face's triangles to the end of the buffer

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

	gpu.dirty_faces.erase(face_idx);

	face_t& f = faces[face_idx];
	int face_num_tris = f.gpu_triangles.size();
	assert(face_num_tris == f.vert_idx.size()-2);

	int old_num_tris = gpu_get_triangles_num();
	int new_num_tris = old_num_tris - face_num_tris;
	assert(new_num_tris >= 0);

	// Take `face_num_tris` triangles at the end and move them
	// where this face's triangles used to be

	std::set<int> faces_affected;
	for (int i = new_num_tris; i < old_num_tris; i++) {
		int other_face_idx = gpu.triangle_which_face[i];
		if (other_face_idx != face_idx)
			faces_affected.insert(other_face_idx);
	}

	if (faces_affected.empty()) {
		// this face itself was at the end after all
		f.gpu_triangles.clear();
	} else {
		for (int other_face_idx: faces_affected) {
			assert(face_exists(other_face_idx));
			gpu_mark_face_dirty(other_face_idx);

			for (int& tri_idx: faces[other_face_idx].gpu_triangles) {
				if (tri_idx < new_num_tris)
					continue;

				// Triangle indices past `new_num_tris` need to be replaced
				tri_idx = f.gpu_triangles.back();
				f.gpu_triangles.pop_back();
				gpu.triangle_which_face[tri_idx] = other_face_idx;
			}
		}

		assert(f.gpu_triangles.empty());
	}

	// Shrink the buffer
	gpu.triangle_which_face.resize(new_num_tris);
}

void mesh_t::gpu_draw () const
{
	glBindVertexArray(gpu.vao_id);
	glDrawArrays(GL_TRIANGLES, 0, 3 * gpu_get_triangles_num());
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
