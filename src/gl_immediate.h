#ifndef GL_IMMEDIATE_H
#define GL_IMMEDIATE_H

#include "gl.h"
#include "math.h"

void imm_init ();
void imm_deinit ();

void imm_begin (GLenum render_mode);
void imm_end ();

void imm_vertex (vec3);
void imm_normal (vec3);
void imm_tex_coord (vec2);
void imm_color (vec3);

namespace imm_attrib_loc
{
static constexpr int POSITION = 0;
static constexpr int NORMAL = 1;
static constexpr int TEX_COORD = 2;
static constexpr int COLOR = 3;
}

#endif /* GL_IMMEDIATE_H */
