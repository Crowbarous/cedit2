#ifndef GUI_H
#define GUI_H

#include "gl.h"

extern float app_font_scale;

void gui_init ();
void gui_deinit ();
void gui_handle_event (SDL_Event& ev);

extern vec2 gui_bottom_window_pos, gui_bottom_window_size;
extern vec2 gui_viewport3d_pos, gui_viewport3d_size;
extern vec2 gui_viewport2d_pos, gui_viewport2d_size;

/* 
 * Only calls ImGui functions to build the draw lists.
 * Also fills in the window dimensions declared above
 */
void gui_generate_frame ();

/* Makes the drawcalls for GUI */
void gui_render_frame ();

#endif /* GUI_H */
