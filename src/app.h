#ifndef APP_H
#define APP_H

#include "camera.h"

void app_init ();
void app_deinit ();
void app_update ();

struct mesh_t {  };

struct viewport3d_t {
	camera_t camera;

	/* Position of the viewport on screen */
	int dim_x_low;
	int dim_y_low;
	int dim_x_high;
	int dim_y_high;

	mesh_t* mesh;

	void render () const;
	void set_size (int xl, int yl, int xh, int yh);
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
