#ifndef GL_IMMEDIATE_H
#define GL_IMMEDIATE_H

#include "gl.h"
#include "math.h"

/*
 * An imitation of the old OpenGL immediate mode rendering,
 * with the principle of "getting something to draw", though
 * not efficiently at all.
 */

namespace imm
{

void init ();
void deinit ();

void begin (GLenum render_mode);
void end ();

void vertex (vec3);
void normal (vec3);
void tex_coord (vec2);
void color (vec3);

namespace attrib_loc
{
static constexpr int POSITION = 0;
static constexpr int NORMAL = 1;
static constexpr int TEX_COORD = 2;
static constexpr int COLOR = 3;
}

} /* namespace imm */

#endif /* GL_IMMEDIATE_H */
