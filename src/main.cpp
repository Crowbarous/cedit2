#include "util.h"
#include "gl.h"
#include "input.h"
#include "app.h"

int main ()
{
	render_init();
	input_init();
	app_init();

	while (!app_quit) {
		input_handle_events();
		app_update();
		render_frame();
	}

	return 0;
}

