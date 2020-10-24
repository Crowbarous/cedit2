#ifndef MAP_EDIT_H
#define MAP_EDIT_H

#include "math.h"

struct map_mesh;

map_mesh* mesh_new ();
void mesh_delete (map_mesh*&);

int mesh_add_vertex (map_mesh*, vec3 position);
int mesh_add_face (map_mesh*, const int* vert_ids, int vert_num);

void mesh_remove_face (map_mesh*, int face_id);
void mesh_get_face_normal (const map_mesh*, int face_id);


void mesh_gpu_draw (const map_mesh*);
void mesh_gpu_init (map_mesh*);
void mesh_gpu_deinit (map_mesh*);
void mesh_gpu_sync (map_mesh*);

namespace mesh_shader_attrib_loc
{
constexpr static int POSITION = 0;
constexpr static int NORMAL = 1;
};

#endif /* MAP_EDIT_H */
