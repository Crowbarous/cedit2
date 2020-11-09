#include "map_edit.h"
#include <cassert>
#include <algorithm>

namespace map
{

/*
 * ======================= MESH IMPLEMENTATION OVERVIEW =======================
 *   The mesh stores vertices and faces in a generally naive way,
 * from the CPU perspective.
 *   However, additionally maintained are (effectively unordered) lists
 * of those faces which are triangles, quads, and n-gons, respectively.
 *   Those are used to maintain the GPU buffers - separate for triangles,
 * quads, and n-gons. Triangles and quads are of fixed size (3 and 6 vertices,
 * respectively), so whenever we need to delete/move/add a face, it's O(1).
 * N-gons aren't so nice, but there are generally fewer of them in a mesh.
 * ============================================================================
 */

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
	                         .adj_faces = { } };
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
		this->verts[in_vert_ids[i]].adj_faces.push_back(face_id);
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

		const auto iter = std::find(v.adj_faces.begin(),
		                            v.adj_faces.end(),
		                            face_id);
		assert(iter != v.adj_faces.end());
		replace_with_last(v.adj_faces, iter - v.adj_faces.begin());

		if (v.adj_faces.empty()) {
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
	assert(v.adj_faces.empty());
	v = { .position = vec3(0.0),
	      .adj_faces = { } };
}

[[always_inline]]
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
	// ends up weighed by edge length, which makes sense anyway)

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

constexpr int BUFFER_MIN_CAPACITY = 128;

constexpr static int buffer_grow_capacity (int c)
{
	c *= 3;
	c /= 2;
	return c;
}

constexpr GLbitfield BUFFER_CREATE_FLAGS = 0
			| GL_DYNAMIC_STORAGE_BIT
			| GL_MAP_WRITE_BIT
			| GL_MAP_PERSISTENT_BIT;
constexpr GLbitfield BUFFER_MAP_FLAGS = 0
			| GL_MAP_WRITE_BIT
			| GL_MAP_PERSISTENT_BIT
			| GL_MAP_FLUSH_EXPLICIT_BIT;

void mesh::gpu_drawn_buffer::init (
		const gpu_vertex* initial_data,
		int initial_size)
{
	assert(this->vertex_array == 0 && this->buffer == 0
	    && this->mapped == nullptr && this->size == 0 && this->capacity == 0);

	this->size = initial_size;

	this->capacity = BUFFER_MIN_CAPACITY;
	while (this->capacity < this->size)
		this->capacity = buffer_grow_capacity(this->capacity);

	this->vertex_array = gl_gen_vertex_array();
	this->buffer = gl_gen_buffer();
	glBindVertexArray(this->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, this->buffer);

	mesh::gpu_set_attrib_pointers();

	glBufferStorage(GL_ARRAY_BUFFER,
			sizeof(gpu_vertex) * this->capacity,
			nullptr, BUFFER_CREATE_FLAGS);

	this->mapped = (gpu_vertex*) glMapBufferRange(
			GL_ARRAY_BUFFER,
			0, sizeof(gpu_vertex) * this->capacity,
			BUFFER_MAP_FLAGS);
	assert(this->mapped != nullptr);

	if (initial_data != nullptr && initial_size != 0) {
		memcpy(this->mapped,
		       initial_data,
		       sizeof(gpu_vertex) * this->size);
	}
}

void mesh::gpu_drawn_buffer::deinit ()
{
	glBindBuffer(GL_ARRAY_BUFFER, this->buffer);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	gl_delete_buffer(this->buffer);
	gl_delete_vertex_array(this->vertex_array);
	this->size = 0;
	this->capacity = 0;
	this->mapped = nullptr;
}

void mesh::gpu_drawn_buffer::resize (int new_size)
{
	const int old_size = this->size;
	const int old_capacity = this->capacity;

	if (old_capacity == 0)
		return this->init(nullptr, new_size);

	this->size = new_size;

	int new_capacity;
	if (old_capacity >= 1024 && old_capacity > 4 * new_size)
		new_capacity = BUFFER_MIN_CAPACITY;
	else if (old_capacity < new_size)
		new_capacity = old_capacity;
	else
		return;

	while (new_capacity < new_size)
		new_capacity = buffer_grow_capacity(new_capacity);

	GLuint old_buffer = this->buffer;
	GLuint new_buffer = gl_gen_buffer();

	glUnmapBuffer(old_buffer);
	glBindBuffer(GL_COPY_READ_BUFFER, old_buffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, new_buffer);
	glBufferStorage(GL_COPY_WRITE_BUFFER,
			sizeof(gpu_vertex) * new_capacity,
			nullptr, BUFFER_CREATE_FLAGS);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
			0, 0, sizeof(gpu_vertex) * old_size);
	gl_delete_buffer(old_buffer);
	this->mapped = (gpu_vertex*) glMapBufferRange(
			GL_COPY_WRITE_BUFFER,
			0, sizeof(gpu_vertex) * new_capacity,
			BUFFER_MAP_FLAGS);
	assert(this->mapped != nullptr);
	this->buffer = new_buffer;
	this->capacity = new_capacity;
}

void mesh::gpu_drawn_buffer::sync ()
{
	glBindBuffer(GL_ARRAY_BUFFER, this->buffer);
	glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, sizeof(gpu_vertex) * this->size);
}

void mesh::gpu_drawn_buffer::draw () const
{
	glBindVertexArray(this->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, this->size);
}

/*
 * Triangulating faces into a given buffer.
 * TODO: actual good triangulation algorithms
 */

/*
 * !!! This magic macro relies on those functions
 * defining all those variables under the same names
 */
#define DUMP_SINGLE_TRIANGLE(destination, v1, v2, v3) \
	do { \
		int _dest_offs = 0; \
		for (int _idx: { v1, v2, v3 }) { \
			(destination)[_dest_offs++] \
				= { .position = this->get_vert_pos(f.vert_ids[_idx]), \
			            .normal = (normal), \
			            .face_id = (face_id) }; \
		} \
	} while (false)

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

	// Split along whichever diagonal is shorter
	const float d02 = length_squared(this->get_vert_pos(f.vert_ids[0])
	                               - this->get_vert_pos(f.vert_ids[2]));
	const float d13 = length_squared(this->get_vert_pos(f.vert_ids[1])
	                               - this->get_vert_pos(f.vert_ids[3]));
	if (d02 < d13) {
		DUMP_SINGLE_TRIANGLE(destination, 0, 1, 2);
		DUMP_SINGLE_TRIANGLE(destination+3, 0, 2, 3);
	} else {
		DUMP_SINGLE_TRIANGLE(destination, 1, 2, 3);
		DUMP_SINGLE_TRIANGLE(destination+3, 1, 3, 0);
	}
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

/*
 * The higher-level GPU functions
 */

bool mesh::gpu_ready () const
{
	// Just check for some of the IDs being valid
	return this->gpu_tris.vertex_array != 0;
}

void mesh::gpu_init ()
{
	assert(!this->gpu_ready());

	std::vector<gpu_vertex> buffer;
	int n;

	// Init triangles
	n = this->face_indices_tri.size();
	buffer.resize(3 * n);
	for (int i = 0; i < n; i++) {
		this->gpu_dump_face_tri(
				buffer.data() + 3*i,
				this->face_indices_tri[i]);
	}
	this->gpu_tris.init(buffer.data(), buffer.size());

	// Init quads
	n = this->face_indices_quad.size();
	buffer.resize(6 * n); // two triangles in each quad
	for (int i = 0; i < n; i++) {
		this->gpu_dump_face_quad(
				buffer.data() + 6*i,
				this->face_indices_quad[i]);
	}
	this->gpu_quads.init(buffer.data(), buffer.size());

	// Init ngons
	n = this->face_indices_ngon.size();
	buffer.clear(); // cannot know the size beforehand; grow on the fly
	for (int i = 0; i < n; i++) {
		const int face_id = this->face_indices_ngon[i];
		const int num_tris = this->faces[face_id].num_verts - 2;
		buffer.resize(buffer.size() + 3*num_tris);
		this->gpu_dump_face_ngon(
				buffer.data() + buffer.size() - 3*num_tris,
				face_id);
	}
	this->gpu_ngons.init(buffer.data(), buffer.size());
}

void mesh::gpu_deinit ()
{
	assert(this->gpu_ready());

	this->gpu_tris.deinit();
	this->gpu_quads.deinit();
	this->gpu_ngons.deinit();
}

void mesh::gpu_sync ()
{
	if (!this->gpu_ready())
		return this->gpu_init();

	this->gpu_tris.sync();
	this->gpu_quads.sync();
	this->gpu_ngons.sync();
}

void mesh::gpu_draw () const
{
	this->gpu_tris.draw();
	this->gpu_quads.draw();
	this->gpu_ngons.draw();
}

/* ==================== UBER FUCTION TO DUMP ALL THE INFO ==================== */

template <typename print_func>
static void print_with_limit (FILE* os, size_t size, print_func f)
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

	fprintf(os, "Vertices: %i total, %i active:\n",
			(int) this->verts.size(),
			this->verts_active.popcount());
	print_with_limit(os, this->verts.size(), [&] { this->verts_active.dump_info(os); });

	assert(this->faces_active.popcount() == this->face_indices_tri.size()
	                                      + this->face_indices_quad.size()
	                                      + this->face_indices_ngon.size());
	fprintf(os, "\nFaces: %i total, %i active:\n",
			(int) this->faces.size(),
			this->faces_active.popcount());
	print_with_limit(os, this->faces.size(), [&] { this->faces_active.dump_info(os); });

	fputs("\nOwnership indices:", os);
	fputs("\ntris:  ", os);
	print_with_limit(os, this->face_indices_tri.size(), [&] {
			for (int i: this->face_indices_tri)
				fprintf(os, "%i ", i);
		});
	fputs("\nquads: ", os);
	print_with_limit(os, this->face_indices_quad.size(), [&] {
			for (int i: this->face_indices_quad)
				fprintf(os, "%i ", i);
		});
	fputs("\nngons: ", os);
	print_with_limit(os, this->face_indices_ngon.size(), [&] {
			for (int i: this->face_indices_ngon)
				fprintf(os, "%i ", i);
		});

	fputs("\nGPU:\n", os);
	auto dump_buffer_info = [&] (const char* tag, const gpu_drawn_buffer& f) {
		fprintf(os, "%s: VAO id %i, VBO id %i, "
			"size: %i vertices (%i bytes), "
			"capacity: %i vertices (%i bytes)\n",
			tag, f.vertex_array, f.buffer,
			f.size, f.size * (int) sizeof(gpu_vertex),
			f.capacity, f.capacity * (int) sizeof(gpu_vertex));
	};
	dump_buffer_info("Tris ", this->gpu_tris);
	dump_buffer_info("Quads", this->gpu_quads);
	dump_buffer_info("Ngons", this->gpu_ngons);
}

} /* namespace map */
