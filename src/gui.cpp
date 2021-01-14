#include "util.h"
#include "input.h"
#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

/* Font is in the app's root dir */
static constexpr const char* FONT_PATH = "JetBrainsMono-Medium.ttf";

static ImFont* im_font;
static ImGuiContext* im_context;

float app_font_scale = 16.0;
static int gui_frame_number = 0;

static bool show_imgui_demo_window = false;

/* Forward declarations for actual gui code down below */
static void gui_generate_top_bar ();
static void gui_generate_bottom_window ();
static void gui_generate_viewport3d ();
static void gui_generate_viewport2d ();

/* Window positions and sizes. The layout is generally fixed */
vec2 gui_bottom_window_pos, gui_bottom_window_size;
vec2 gui_viewport3d_pos, gui_viewport3d_size;
vec2 gui_viewport2d_pos, gui_viewport2d_size;
static float gui_top_bar_height;

void gui_init ()
{
	assert(render_context.is_initialized);

	IMGUI_CHECKVERSION();
	im_context = ImGui::CreateContext();
	ImGui::SetCurrentContext(im_context);
	ImGui_ImplSDL2_InitForOpenGL(render_context.sdl_window,
	                             render_context.sdl_gl_context);
	ImGui_ImplOpenGL3_Init();

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0.0;

	ImFontAtlas* atlas = ImGui::GetIO().Fonts;
	im_font = atlas->AddFontFromFileTTF(FONT_PATH,
	                                    app_font_scale,
	                                    nullptr,
	                                    atlas->GetGlyphRangesCyrillic());
	assert(im_font != nullptr);
}

void gui_deinit ()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	assert(ImGui::GetCurrentContext() == im_context);
	ImGui::DestroyContext();
}

void gui_generate_frame ()
{
	gui_frame_number++;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(render_context.sdl_window);
	ImGui::NewFrame();

	const float half_res_x = render_context.resolution_x * 0.5;
	const float half_res_y = render_context.resolution_y * 0.5;

	gui_top_bar_height = ImGui::GetTextLineHeightWithSpacing() + 3.0;
	gui_generate_top_bar();

	// 3d viewport is top-left
	gui_viewport3d_pos = { 0, gui_top_bar_height };
	gui_viewport3d_size = { half_res_x, half_res_y };
	gui_generate_viewport3d();

	// 2d viewport is top-right
	gui_viewport2d_pos = { half_res_x, gui_top_bar_height };
	gui_viewport2d_size = { half_res_x, half_res_y };
	gui_generate_viewport2d();

	// main window takes up the bottom
	gui_bottom_window_pos = { 0, gui_top_bar_height + half_res_y };
	gui_bottom_window_size = { (float) render_context.resolution_x,
	                           half_res_y - gui_top_bar_height };
	gui_generate_bottom_window();

	if (show_imgui_demo_window)
		ImGui::ShowDemoWindow(&show_imgui_demo_window);

	ImGui::Render();
}

void gui_render_frame ()
{
	assert(render_context.is_rendering);

	glViewport(0, 0, render_context.resolution_x, render_context.resolution_y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gui_handle_event (SDL_Event& e)
{
	ImGui_ImplSDL2_ProcessEvent(&e);
}

/* ================ ACTUAL GUI CODE ================== */
using namespace ImGui;

constexpr static ImGuiWindowFlags RIGID_WINDOW_FLAGS
		= ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoBringToFrontOnFocus;

constexpr static ImGuiWindowFlags VIEWPORT_WINDOW_FLAGS
		= RIGID_WINDOW_FLAGS
		| ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoDecoration;

static void gui_generate_top_bar ()
{
	BeginMainMenuBar();

	if (BeginMenu("App")) {
		if (show_imgui_demo_window)
			show_imgui_demo_window = !MenuItem("Hide demo");
		else
			show_imgui_demo_window = MenuItem("Show demo");

		Separator();

		if (MenuItem("Exit##menu-exit"))
			app_quit = true;
		EndMenu();
	}

	EndMainMenuBar();
}

static void draw_viewport_border (vec2 pos, vec2 size)
{
	ImDrawList* dl = GetForegroundDrawList();
	auto index = IsWindowFocused() ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg;
	dl->AddRect(pos, pos + size, GetColorU32(index));
}

/*
 * The viewport windows are phony, they only serve to receive focus.
 * The actual rendering of either viewport is done behind them, before ImGui draws
 */

static void gui_generate_viewport3d ()
{
	SetNextWindowPos(gui_viewport3d_pos);
	SetNextWindowSize(gui_viewport3d_size);
	Begin("#3d", nullptr, VIEWPORT_WINDOW_FLAGS);
	draw_viewport_border(gui_viewport3d_pos, gui_viewport3d_size);
	End();
}

static void gui_generate_viewport2d ()
{
	SetNextWindowPos(gui_viewport2d_pos);
	SetNextWindowSize(gui_viewport2d_size);
	Begin("#2d", nullptr, VIEWPORT_WINDOW_FLAGS);

	draw_viewport_border(gui_viewport2d_pos, gui_viewport2d_size);
	End();
}

static void gui_generate_bottom_window ()
{
	SetNextWindowPos(gui_bottom_window_pos);
	SetNextWindowSize(gui_bottom_window_size);
	Begin("##bottom", nullptr, RIGID_WINDOW_FLAGS);

	End();
}
