#include "util.h"
#include "gl.h"
#include "input.h"
#include "app.h"

int main (int argc, char** argv)
{
	input_parse_cmdline_options(argc, argv);

	render_init();
	input_init();
	app_init();

	while (!app_quit) {
		input_handle_events();
		app_update();
		render_frame();
	}

	app_deinit();
	render_deinit();

	return 0;
}
