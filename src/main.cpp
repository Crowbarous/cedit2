#include "util.h"
#include "gl.h"
#include "gui.h"
#include "input.h"
#include "app.h"

int main (int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
		input_parse_cmdline_option(argv[i]);

	render_init();
	gui_init();
	input_init();
	app_init();

	while (!app_quit) {
		gui_generate_frame();
		render_frame();
		input_handle_events();
		app_update();
	}

	app_deinit();
	gui_deinit();
	render_deinit();

	return 0;
}
