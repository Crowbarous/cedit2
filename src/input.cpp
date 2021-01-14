#include "app.h"
#include "gl.h"
#include "gui.h"
#include "imgui/imgui.h"
#include "input.h"
#include "util.h"
#include <map>

bool app_quit;
static bool app_3d_mousegrab;

/* 
 * App will wait on next OS event and not
 * render a frame unless this flag is set
 */
static bool app_another_frame = false;

std::map<SDL_Scancode, keybind_t> keybinds;

KEY_FUNC (keybind_quit)
{
	app_quit = true;
}

KEY_FUNC (keybind_camera_move)
{
	switch ((char) ((uintptr_t) KEY_FUNC_USER_DATA & 0xFF)) {
	case 'f':
		camera_move_flags.forward = KEY_FUNC_PRESSED;
		break;
	case 'b':
		camera_move_flags.backward = KEY_FUNC_PRESSED;
		break;
	case 'l':
		camera_move_flags.left = KEY_FUNC_PRESSED;
		break;
	case 'r':
		camera_move_flags.right = KEY_FUNC_PRESSED;
		break;
	case 'F':
		camera_move_flags.speed
			= KEY_FUNC_PRESSED ? move_flags_t::FAST
			                   : move_flags_t::NORMAL;
		break;
	case 'S':
		camera_move_flags.speed
			= KEY_FUNC_PRESSED ? move_flags_t::SLOW
			                   : move_flags_t::NORMAL;
		break;
	}
}

void mouse_bind (int x, int y, int dx, int dy)
{
	if (!app_3d_mousegrab)
		return;

	const float sens = 0.3;
	vec3& ang = viewport.camera.angles;
	ang += sens * vec3(-dy, -dx, 0.0);

	constexpr float epsilon = 1e-3;
	if (ang.x < -90.0)
		ang.x = -90.0 + epsilon;
	if (ang.x > 90.0)
		ang.x = 90.0 - epsilon;
}

KEY_FUNC (keybind_toggle_mousegrab)
{
	if (!KEY_FUNC_PRESSED)
		return;
	app_3d_mousegrab ^= true;
	SDL_SetRelativeMouseMode(app_3d_mousegrab ? SDL_TRUE : SDL_FALSE);
}

void input_init ()
{
	app_quit = false;
	app_3d_mousegrab = false;

	keybinds[SDL_SCANCODE_Q] = { keybind_quit, nullptr };

	keybinds[SDL_SCANCODE_W] = { keybind_camera_move, (void*) 'f' };
	keybinds[SDL_SCANCODE_A] = { keybind_camera_move, (void*) 'l' };
	keybinds[SDL_SCANCODE_S] = { keybind_camera_move, (void*) 'b' };
	keybinds[SDL_SCANCODE_D] = { keybind_camera_move, (void*) 'r' };

	keybinds[SDL_SCANCODE_LCTRL] = { keybind_camera_move, (void*) 'S' };
	keybinds[SDL_SCANCODE_LSHIFT] = { keybind_camera_move, (void*) 'F' };

	keybinds[SDL_SCANCODE_Z] = { keybind_toggle_mousegrab, nullptr };
}


static void run_keybind (SDL_Scancode scan, bool pressed)
{
	auto iter = keybinds.find(scan);
	if (iter == keybinds.end())
		return;

	const keybind_t& kb = iter->second;
	if (kb.callback != nullptr)
		kb.callback(kb.user_data, pressed);
}

void input_handle_events ()
{
	SDL_Event e;

	bool can_wait = !camera_is_moving();
	if (app_another_frame) {
		can_wait = false;
		app_another_frame = false;
	}

	if (can_wait) {
		SDL_WaitEvent(&e);
	} else {
		if (!SDL_PollEvent(&e))
			return;
	}

	const auto& io = ImGui::GetIO();
	const bool imgui_takes_mouse = io.WantCaptureMouse && !app_3d_mousegrab;
	const bool imgui_takes_kb = io.WantCaptureKeyboard;
	app_another_frame = (imgui_takes_mouse || imgui_takes_kb);

	do {
		if (!app_3d_mousegrab)
			gui_handle_event(e);

		switch (e.type) {
		case SDL_QUIT:
			app_quit = true;
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				render_resize_window(e.window.data1, e.window.data2);
			break;
		case SDL_KEYDOWN:
			if (!imgui_takes_kb)
				run_keybind(e.key.keysym.scancode, true);
			break;
		case SDL_KEYUP:
			if (!imgui_takes_kb)
				run_keybind(e.key.keysym.scancode, false);
			break;
		case SDL_MOUSEMOTION:
			if (!imgui_takes_mouse) {
				mouse_bind(e.motion.x, e.motion.y,
				           e.motion.xrel, e.motion.yrel);
			}
			break;
		}

	} while (SDL_PollEvent(&e));
}


enum cmdline_flag_type_t {
	BOOL_TRUE,
	BOOL_FALSE,
	INT_VAL,
	FLOAT_VAL,
};

struct cmdline_flag_t {
	const char* name;
	cmdline_flag_type_t type;
	void* variable;
};

const static cmdline_flag_t cmdline_flags[] = {
	{ "opengl-debug", BOOL_TRUE, &app_opengl_debug },
	{ "opengl-msaa", INT_VAL, &app_opengl_msaa },
	{ "font-scale", FLOAT_VAL, &app_font_scale },
};

constexpr int cmdline_flag_nr = sizeof(cmdline_flags) / sizeof(cmdline_flag_t);

static void set_cmdline_flag (const cmdline_flag_t& f, const char* value)
{
	bool bool_val = true;
	switch (f.type) {
	case BOOL_FALSE:
		bool_val = false;
		// Fallthrough
	case BOOL_TRUE:
		if (value != nullptr) {
			if (str_any_of(value, { "true", "yes", "1" }))
				bool_val = true;
			else if (str_any_of(value, { "false", "no", "0" }))
				bool_val = false;
			else
				fatal("Option --%s is boolean", f.name);
		}
		*((bool*) f.variable) = bool_val;
		break;
	case INT_VAL:
		if (value == nullptr) {
			fatal("Option --%s requires an integer argument",
					f.name);
		}
		*((int*) f.variable) = atoi(value);
		break;
	case FLOAT_VAL:
		if (value == nullptr) {
			fatal("Option --%s requires a floating point argument",
					f.name);
		}
		*((float*) f.variable) = atoi(value);
	}
}

void input_parse_cmdline_option (const char* str)
{
	if (*str != '-')
		return;

	// skip - or --
	str++;
	if (*str == '-')
		str++;

	for (int j = 0; j < cmdline_flag_nr; j++) {
		const cmdline_flag_t& f = cmdline_flags[j];

		for (int k = 0; ; k++) {
			if (f.name[k] == '\0') {
				if (str[k] == '\0')
					set_cmdline_flag(f, nullptr);
				else if (str[k] == '=')
					set_cmdline_flag(f, str + k + 1);
				else
					continue;
				return;
			} else if (f.name[k] != str[k]) {
				break;
			}
		}
	}
}
