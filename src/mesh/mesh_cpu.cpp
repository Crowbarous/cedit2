#include "mesh.h"
#include "util.h"
#include <algorithm>
#include <cassert>
#include <cstdio>

int mesh_t::add_vert (vec3 pos)
{
	int idx = verts_active.set_first_cleared();

	assert(idx < verts.size() + 1);
	if (idx == verts.size()) {
		verts.emplace_back(pos);
	} else {
		verts[idx].~vert_t();
		new (&verts[idx]) vert_t(pos);
	}

	return idx;
}

int mesh_t::add_face (const int* vert_indices, int num_verts)
{
	assert(num_verts >= 3);

	int face_idx = faces_active.set_first_cleared();

	assert(face_idx < faces.size() + 1);
	if (face_idx == faces.size())
		faces.emplace_back();

	faces[face_idx].vert_idx.assign(vert_indices, vert_indices + num_verts);

	for (int i = 0; i < num_verts; i++) {
		int prev = (i - 1 + num_verts) % num_verts;
		int next = (i + 1) % num_verts;
		verts[vert_indices[i]].adj_faces.push_back(
				{ face_idx,
				  vert_indices[prev],
				  vert_indices[next] });
	}

	gpu_add_face(face_idx);

	return face_idx;
}

int mesh_t::add_face (std::initializer_list<int> vert_indices)
{
	const int* b = vert_indices.begin();
	const int* e = vert_indices.end();
	return add_face(b, e - b);
}

void mesh_t::remove_face (int face_idx)
{
	assert(face_exists(face_idx));
	auto& vi = faces[face_idx].vert_idx;

	for (int vert_idx: vi) {
		auto& af = verts[vert_idx].adj_faces;
		for (int i = 0; i < af.size(); i++) {
			if (af[i].face_idx == face_idx) {
				af[i] = af.back();
				af.pop_back();
				break;
			}
		}
	}

	gpu_remove_face(face_idx);
	faces_active.clear(face_idx);

	if (face_idx == faces.size()-1)
		faces.pop_back();
	else
		faces[face_idx].vert_idx.clear();
}

void mesh_t::remove_vert (int vert_idx)
{
	assert(vert_exists(vert_idx));
	verts_active.clear(vert_idx);

	// Clear adj_faces before calling remove_face() on its
	// (former) elements so that remove_face() has less to do

	int num_faces = verts[vert_idx].adj_faces.size();
	std::vector<int> adj_face_ids(num_faces);

	for (int i = 0; i < num_faces; i++)
		adj_face_ids[i] = verts[vert_idx].adj_faces[i].face_idx;

	verts[vert_idx].adj_faces.clear();

	for (int i = 0; i < num_faces; i++)
		remove_face(adj_face_ids[i]);
}

bool mesh_t::vert_exists (int idx) const
{
	return verts_active.is_set(idx);
}

bool mesh_t::face_exists (int idx) const
{
	return faces_active.is_set(idx);
}

bool mesh_t::edge_exists (int vert1_idx, int vert2_idx) const
{
	if (!vert_exists(vert1_idx) || !vert_exists(vert2_idx))
		return false;

	// see where we'll have a smaller vector to look through
	if (verts[vert1_idx].adj_faces.size()
	  > verts[vert2_idx].adj_faces.size())
		std::swap(vert1_idx, vert2_idx);

	for (const auto& af: verts[vert1_idx].adj_faces) {
		if (af.prev_vert_idx == vert2_idx
		 || af.next_vert_idx == vert2_idx)
			return true;
	}

	return false;
}

vec3 mesh_t::get_face_normal (int face_idx) const
{
	// get the normal at some three vertices, whatever
	const face_t& f = faces[face_idx];
	const vec3& p1 = verts[f.vert_idx[0]].position;
	const vec3& p2 = verts[f.vert_idx[1]].position;
	const vec3& p3 = verts[f.vert_idx[2]].position;
	return glm::normalize(glm::cross(p3 - p2, p1 - p2));
}

vec3 mesh_t::get_vert_position (int vert_idx) const
{
	assert(vert_exists(vert_idx));
	return verts[vert_idx].position;
}


void mesh_t::debug_dump_info () const
{
	FILE* const stream = stderr;

	fprintf(stream, "Mesh 0x%p: %li vertices, %li faces (including inactive)\n",
			this, verts.size(), faces.size());
	fprintf(stream, "GPU data: VAO %i, VBO %i, actual tris: %i, capacity: %i\n",
			gpu.vao_id, gpu.vbo_id,
			gpu_get_triangles_num(), gpu_get_triangles_capacity());
}


void mesh_add_cuboid (mesh_t& m, vec3 center, vec3 side_len)
{
	int vert_ids[8];

	side_len *= 0.5;
	const vec3 low = center - side_len;
	const vec3 high = center + side_len;

	for (int i = 0; i < 8; i++) {
		vert_ids[i] = m.add_vert(
				{ (i & 1 ? low : high).x,
				  (i & 2 ? low : high).y,
				  (i & 4 ? low : high).z });
	}

	auto quad = [&] (int a, int b, int c, int d) {
		m.add_face({ vert_ids[a],
		             vert_ids[b],
		             vert_ids[c],
		             vert_ids[d] });
	};

	quad(0, 2, 3, 1);
	quad(0, 1, 5, 4);
	quad(0, 4, 6, 2);
	quad(7, 5, 1, 3);
	quad(7, 3, 2, 6);
	quad(7, 6, 4, 5);
}
