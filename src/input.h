#ifndef INPUT_H
#define INPUT_H

void input_parse_cmdline_option (const char* option);

void input_init ();
void input_handle_events ();

extern bool app_quit;

struct keybind_t {
	void (*callback) (void* user_data, bool pressed);
	void* user_data;
};

/*
 * The names of the arguments aren't macros, but make sure that
 * it's clear they are related to the macro the function was defined with
 */
#define KEY_FUNC(name) \
	void name ([[maybe_unused]] void* KEY_FUNC_USER_DATA, \
	           [[maybe_unused]] bool KEY_FUNC_PRESSED)

#endif /* INPUT_H */
