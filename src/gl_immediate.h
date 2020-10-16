#ifndef GL_IMMEDIATE_H
#define GL_IMMEDIATE_H

#include "gl.h"
#include "math.h"

void imm_init ();
void imm_deinit ();

void imm_begin (GLenum render_mode);
void imm_end ();

void imm_color (vec3);
void imm_vertex (vec3);

void imm_set_transform (const mat4&);

#endif /* GL_IMMEDIATE_H */
