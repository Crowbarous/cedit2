#include "util.h"
#include "input.h"
#include "gl.h"
#include "app.h"
#include <map>

bool app_quit;
std::map<SDL_Scancode, keybind_t> keybinds;

KEY_FUNC (keybind_quit)
{
	app_quit = true;
}

KEY_FUNC (keybind_camera_move)
{
	switch ((char) ((uintptr_t) user_data & 0xFF)) {
	case 'f':
		camera_move_flags.forward = pressed;
		break;
	case 'b':
		camera_move_flags.backward = pressed;
		break;
	case 'l':
		camera_move_flags.left = pressed;
		break;
	case 'r':
		camera_move_flags.right = pressed;
		break;
	case 'F':
		camera_move_flags.speed = pressed ? move_flags_t::FAST
		                                  : move_flags_t::NORMAL;
		break;
	case 'S':
		camera_move_flags.speed = pressed ? move_flags_t::SLOW
		                                  : move_flags_t::NORMAL;
		break;
	}
}

KEY_FUNC (keybind_change_mesh)
{
	if (!pressed)
		return;
	static float z = -5.0;
	mesh_add_cuboid(*viewport.mesh, vec3(-2.0, -2.0, z), vec3(1.3));
	z += 2.0;
}

KEY_FUNC (keybind_print_mesh)
{
	if (!pressed)
		return;
	viewport.mesh->debug_dump_info();
}

void mouse_bind (int x, int y, int dx, int dy)
{
	const float sens = 0.3;
	vec3& ang = viewport.camera.angles;
	ang += sens * vec3(-dy, -dx, 0.0);

	constexpr float epsilon = 1e-3;
	if (ang.x < -90.0)
		ang.x = -90.0 + epsilon;
	if (ang.x > 90.0)
		ang.x = 90.0 - epsilon;
}

void input_init ()
{
	app_quit = false;

	keybinds[SDL_SCANCODE_Q] = { keybind_quit, nullptr };
	keybinds[SDL_SCANCODE_P] = { keybind_change_mesh, nullptr };
	keybinds[SDL_SCANCODE_O] = { keybind_print_mesh, nullptr };

	keybinds[SDL_SCANCODE_W] = { keybind_camera_move, (void*) 'f' };
	keybinds[SDL_SCANCODE_A] = { keybind_camera_move, (void*) 'l' };
	keybinds[SDL_SCANCODE_S] = { keybind_camera_move, (void*) 'b' };
	keybinds[SDL_SCANCODE_D] = { keybind_camera_move, (void*) 'r' };

	keybinds[SDL_SCANCODE_LCTRL] = { keybind_camera_move, (void*) 'S' };
	keybinds[SDL_SCANCODE_LSHIFT] = { keybind_camera_move, (void*) 'F' };

	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void input_handle_events ()
{
	SDL_Event e;

	if (camera_is_moving()) {
		// When the camera is moving, we need to generate frames
		// constantly and not just on events, so don't wait here
		if (!SDL_PollEvent(&e))
			return;
	} else {
		SDL_WaitEvent(&e);
	}

	auto run_keybind = [] (SDL_Scancode scan, bool pressed) {
		auto iter = keybinds.find(scan);
		if (iter == keybinds.end())
			return;
		const keybind_t& kb = iter->second;
		if (kb.callback != nullptr)
			kb.callback(kb.user_data, pressed);
	};

	do {
		switch (e.type) {
		case SDL_QUIT:
			app_quit = true;
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
				int w = e.window.data1;
				int h = e.window.data2;
				render_resize_window(w, h);
				viewport.set_size(0, 0, w, h);
			}
			break;
		case SDL_KEYDOWN:
			if (!e.key.repeat)
				run_keybind(e.key.keysym.scancode, true);
			break;
		case SDL_KEYUP:
			run_keybind(e.key.keysym.scancode, false);
			break;
		case SDL_MOUSEMOTION:
			mouse_bind(e.motion.x, e.motion.y,
					e.motion.xrel, e.motion.yrel);
			break;
		}
	} while (SDL_PollEvent(&e));
}
