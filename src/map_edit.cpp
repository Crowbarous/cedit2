#include "map_edit.h"
#include <cassert>

namespace map
{

/*
 * Some functions do book keeping that might erroneuously continue
 * to compile when `mesh::face` changes and silently do the wrong thing
 */
#define REWRITE_THIS_IF_FACE_STRUCT_CHANGES() \
	static_assert(sizeof(face) == sizeof(face::vert_ids) \
	                            + sizeof(face::num_verts) \
	                            + sizeof(face::internal_id), \
		"Struct map::mesh::face changed!!! Revisit all functions " \
		"with this assertion before updating the macro")


mesh::mesh () { }

mesh::~mesh ()
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	for (int i = 0; i < this->faces.size(); i++) {
		face& f = this->faces[i];
		if (this->face_exists(i)) {
			assert(f.vert_ids != nullptr);
			delete[] f.vert_ids;
		} else {
			assert(f.vert_ids == nullptr);
		}
	}
}

int mesh::add_vertex (const vec3& position)
{
	int vert_id = this->verts_active.set_first_cleared();

	assert(vert_id <= this->verts.size());
	if (vert_id == this->verts.size())
		this->verts.emplace_back();

	this->verts[vert_id] = { .position = position,
	                         .faces_referencing = 0};
	return vert_id;
}

int mesh::add_face (const int* vert_ids, int vert_num)
{
	int face_id = this->faces_active.set_first_cleared();

	assert(face_id <= this->faces.size());
	if (face_id == this->faces.size())
		this->faces.emplace_back();

	this->construct_face_at_id(face_id, vert_ids, vert_num);
	return face_id;
}

int mesh::add_face (std::initializer_list<int> vert_ids)
{
	return this->add_face(
			(const int*) vert_ids.begin(),
			(int) vert_ids.size());
}

void mesh::construct_face_at_id (
		int face_id,
		const int* in_vert_ids,
		int vert_num)
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	assert(vert_num >= 3);

	face& f = this->faces[face_id];
	assert(f.vert_ids == nullptr);

	f.vert_ids = new int[vert_num];
	f.num_verts = vert_num;

	for (int i = 0; i < vert_num; i++) {
		assert(this->vert_exists(in_vert_ids[i]));
		this->verts[in_vert_ids[i]].faces_referencing++;
		f.vert_ids[i] = in_vert_ids[i];
	}

	switch (vert_num) {
	case 3:
		f.internal_id = this->face_indices_tri.size();
		this->face_indices_tri.push_back(face_id);
		break;
	case 4:
		f.internal_id = this->face_indices_quad.size();
		this->face_indices_quad.push_back(face_id);
		break;
	default:
		f.internal_id = this->face_indices_ngon.size();
		this->face_indices_ngon.push_back(face_id);
		break;
	}
}

void mesh::remove_face (int face_id)
{
	REWRITE_THIS_IF_FACE_STRUCT_CHANGES();
	assert(this->face_exists(face_id));

	face& f = this->faces[face_id];

	for (int i = 0; i < f.num_verts; i++) {
		const int vert_id = f.vert_ids[i];
		vertex& v = this->verts[vert_id];
		assert(v.faces_referencing > 0);
		v.faces_referencing--;
		if (v.faces_referencing == 0) {
			// TODO: For now, just kill "stray" vertices; but since vertex
			// IDs are user-facing, the user code will not expect vertices
			// to which it holds IDs to die, so think of something else
			this->remove_vert(vert_id);
		}
	}

	int other_face_id;
	switch (f.num_verts) {
	case 3:
		other_face_id = this->face_indices_tri.back();
		replace_with_last(this->face_indices_tri, f.internal_id);
		break;
	case 4:
		other_face_id = this->face_indices_quad.back();
		replace_with_last(this->face_indices_quad, f.internal_id);
		break;
	default:
		other_face_id = this->face_indices_ngon.back();
		replace_with_last(this->face_indices_ngon, f.internal_id);
		break;
	}

	if (other_face_id != face_id) {
		assert(this->face_exists(other_face_id));
		this->faces[other_face_id].internal_id = f.internal_id;
	}

	faces_active.clear_bit(face_id);
	f = { .vert_ids = nullptr,
		.num_verts = 0,
		.internal_id = 0 };
}

void mesh::remove_vert (int vert_id)
{
	assert(this->vert_exists(vert_id));
	this->verts_active.clear_bit(vert_id);

	vertex& v = this->verts[vert_id];
	assert(v.faces_referencing == 0);
	v = { .position = vec3(0.0),
	      .faces_referencing = 0 };
}

vec3 mesh::get_vert_pos (int vert_id) const
{
	assert(this->vert_exists(vert_id));
	return this->verts[vert_id].position;
}


static vec3 normalize_or_zero (vec3 v)
{
	return (v == vec3(0.0)) ? v : glm::normalize(v);
}

#define VERT_POS(i) (this->get_vert_pos(f.vert_ids[i]))
vec3 mesh::get_face_normal_tri (int face_id) const
{
	// Simply cross some two edges
	const face& f = this->faces[face_id];
	const vec3 edge1 = VERT_POS(0) - VERT_POS(1);
	const vec3 edge2 = VERT_POS(0) - VERT_POS(2);
	return normalize_or_zero(glm::cross(edge1, edge2));
}

vec3 mesh::get_face_normal_quad (int face_id) const
{
	// Cross the two diagonals (which might themselves
	// not intersect, since the quad is not necessarily planar)
	const face& f = this->faces[face_id];
	const vec3 diag1 = VERT_POS(0) - VERT_POS(2);
	const vec3 diag2 = VERT_POS(1) - VERT_POS(3);
	return normalize_or_zero(glm::cross(diag1, diag2));
}

vec3 mesh::get_face_normal_ngon (int face_id) const
{
	// Find the normal "at" each vertex and average those
	// (no normalization being done meanwhile, the result
	// ends up weighed by edge length, which seems logical anyway)

	const face& f = this->faces[face_id];
	const int n = f.num_verts;

	vec3 edges[n];
	for (int i = 0; i < n; i++)
		edges[i] = VERT_POS(i) - VERT_POS((i + 1) % n);

	vec3 result(0.0);
	for (int i = 0; i < n; i++)
		result += glm::cross(edges[i], edges[(i + 1) % n]);

	return normalize_or_zero(result);
}
#undef VERT_POS

vec3 mesh::get_face_normal (int face_id) const
{
	assert(this->face_exists(face_id));
	switch (this->faces[face_id].num_verts) {
	case 3:
		return this->get_face_normal_tri(face_id);
	case 4:
		return this->get_face_normal_quad(face_id);
	default:
		return this->get_face_normal_ngon(face_id);
	}
}

/* ======================= GPU STUFF ======================= */

/* Vertex format */
void mesh::gpu_set_attrib_pointers ()
{
	using namespace mesh_vertex_attrib;
	gl_vertex_attrib_ptr(
			POSITION_LOC,
			POSITION_NUM_ELEMS,
			POSITION_DATA_TYPE,
			GL_FALSE,
			sizeof(gpu_vertex),
			offsetof(gpu_vertex, position));
	gl_vertex_attrib_ptr(
			NORMAL_LOC,
			NORMAL_NUM_ELEMS,
			NORMAL_DATA_TYPE,
			GL_TRUE,
			sizeof(gpu_vertex),
			offsetof(gpu_vertex, normal));
	gl_vertex_attrib_ptr(
			FACEID_LOC,
			FACEID_NUM_ELEMS,
			FACEID_DATA_TYPE,
			GL_FALSE,
			sizeof(gpu_vertex),
			offsetof(gpu_vertex, face_id));
}


/* gpu_drawn_buffer */

constexpr int BUFFER_MIN_CAPACITY = 32;

void mesh::gpu_drawn_buffer::init (
		const gpu_vertex* initial_data,
		int initial_size)
{
	this->vertex_array = gl_gen_vertex_array();
	this->buffer = gl_gen_buffer();

	this->size = initial_size;
	this->capacity = std::max(initial_size, BUFFER_MIN_CAPACITY);

	glBindBuffer(GL_ARRAY_BUFFER, this->buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(gpu_vertex) * this->capacity,
		     initial_data,
		     GL_STREAM_DRAW);
}

void mesh::gpu_drawn_buffer::deinit ()
{
	gl_delete_buffer(this->buffer);
	gl_delete_vertex_array(this->vertex_array);
}

/*
 * Triangulating faces into a given buffer.
 * TODO: actual good triangulation algorithms
 */

/*
 * !!! This macro relies on those functions
 * defining all those variables under the same names
 * */
#define DUMP_SINGLE_TRIANGLE(destination, v1, v2, v3) \
	do { \
		int _dest_offs = 0; \
		for (int _idx: { v1, v2, v3 }) { \
			(destination)[_dest_offs++] \
				= { .position = this->get_vert_pos(f.vert_ids[_idx]), \
			            .normal = (normal), \
			            .face_id = (face_id) }; \
		} \
	} while (0)

void mesh::gpu_dump_face_tri (gpu_vertex* destination, int face_id) const
{
	// Triangulating a triangle isn't too hard...
	assert(this->face_exists(face_id));
	const face& f = this->faces[face_id];
	const vec3 normal = this->get_face_normal_tri(face_id);
	DUMP_SINGLE_TRIANGLE(destination, 0, 1, 2);
}

void mesh::gpu_dump_face_quad (gpu_vertex* destination, int face_id) const
{
	assert(this->face_exists(face_id));
	const face& f = this->faces[face_id];
	const vec3 normal = this->get_face_normal_quad(face_id);

	// Split along the 0-2 diagonal, whatever
	DUMP_SINGLE_TRIANGLE(destination, 0, 1, 2);
	DUMP_SINGLE_TRIANGLE(destination + 3, 0, 2, 3);
}

void mesh::gpu_dump_face_ngon (gpu_vertex* destination, int face_id) const
{
	assert(this->face_exists(face_id));
	const face& f = this->faces[face_id];
	const vec3 normal = this->get_face_normal_ngon(face_id);

	// Pan triangulation
	for (int i = 2; i < f.num_verts; i++)
		DUMP_SINGLE_TRIANGLE(destination + 3*(i-2), 0, i-1, i);
}

/* The higher-level GPU functions */

bool mesh::gpu_is_initialized () const
{
	return this->gpu_tris.vertex_array != 0;
}

void mesh::gpu_init ()
{
	assert(!this->gpu_is_initialized());

	// TODO!!!
	this->gpu_tris.init(nullptr, 0);
	this->gpu_quads.init(nullptr, 0);
	this->gpu_ngons.init(nullptr, 0);
}

void mesh::gpu_deinit ()
{
	assert(this->gpu_is_initialized());

	this->gpu_tris.deinit();
	this->gpu_quads.deinit();
	this->gpu_ngons.deinit();
}

void mesh::gpu_sync ()
{
	if (!this->gpu_is_initialized())
		return this->gpu_init();
}

void mesh::gpu_draw () const
{
	// this->gpu_tris.draw();
	// this->gpu_quads.draw();
	// this->gpu_ngons.draw();
}

/* ==================== UBER FUCTION TO DUMP ALL THE INFO ==================== */

template <typename print_func>
static void print_sanely (FILE* os, size_t size, print_func f)
{
	constexpr static int MAX_SIZE_FOR_OUTPUT = 256;
	if (size == 0)
		fputs("<none>", os);
	else if ((int) size > MAX_SIZE_FOR_OUTPUT)
		fputs("<a lot>", os);
	else
		f();
}

void mesh::dump_info (FILE* os) const
{
	fprintf(os, "Mesh at %p:\n", this);

	fprintf(os, "Vertices: %i total, active:\n", (int) this->verts.size());
	print_sanely(os, this->verts.size(), [&] { this->verts_active.dump_info(os); });

	fprintf(os, "Faces: %i total, active:\n", (int) this->faces.size());
	print_sanely(os, this->faces.size(), [&] { this->faces_active.dump_info(os); });

	fputs("Ownership indices:", os);

	fputs("\ntris:  ", os);
	print_sanely(os, this->face_indices_tri.size(), [&] {
			for (int i: this->face_indices_tri)
				fprintf(os, "%i ", i);
		});
	fputs("\nquads: ", os);
	print_sanely(os, this->face_indices_quad.size(), [&] {
			for (int i: this->face_indices_quad)
				fprintf(os, "%i ", i);
		});
	fputs("\nngons: ", os);
	print_sanely(os, this->face_indices_ngon.size(), [&] {
			for (int i: this->face_indices_ngon)
				fprintf(os, "%i ", i);
		});

	auto dump_buffer_info = [&] (const gpu_drawn_buffer& f) -> void {
		fprintf(os, "VAO id %i, VBO id, %i, size %i, capacity %i\n",
				f.vertex_array, f.buffer, f.size, f.capacity);
	};
	fputs("\nGPU:\nTris:  ", os);
	dump_buffer_info(this->gpu_tris);
	fputs("Quads: ", os);
	dump_buffer_info(this->gpu_quads);
	fputs("Ngons: ", os);
	dump_buffer_info(this->gpu_ngons);
}

} /* namespace map */
