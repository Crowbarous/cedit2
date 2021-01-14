#ifndef APP_H
#define APP_H

#include "camera.h"
#include "math.h"

void app_init ();
void app_deinit ();
void app_update ();

struct viewport3d_t {
	void render () const;
	void set_dimension (vec2 pos_top_left, vec2 size);

	camera_t camera;

	vec2 pos;
	vec2 size;
};
extern viewport3d_t viewport;

struct move_flags_t {
	uint8_t forward;
	uint8_t backward;
	uint8_t left;
	uint8_t right;
	enum { NORMAL, FAST, SLOW } speed;
};
extern move_flags_t camera_move_flags;
bool camera_is_moving ();

#endif /* APP_H */
