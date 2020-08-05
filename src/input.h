#ifndef INPUT_H
#define INPUT_H

void input_parse_cmdline_options (int argc, char** argv);

void input_init ();
void input_handle_events ();

extern bool app_quit;

struct keybind_t {
	void (*callback) (void* user_data, bool pressed);
	void* user_data;
};


#define KEY_FUNC(name) \
	void name ([[maybe_unused]] void* user_data, \
	           [[maybe_unused]] bool pressed)

#endif // INPUT_H
