#ifndef YAMENU_H
#define YAMENU_H

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_timer.h"
#include "cglm/struct/vec3.h"
#include "cglm/vec3.h"
#include "ft2build.h"
#include "glad/glad.h"
#include <SDL3/SDL.h>
#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/struct.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freetype/freetype.h>
#include <dirent.h>
#include <unistd.h>
#include "stb_image.h"

typedef struct {
	const char *title;
	uint32_t width;
	uint32_t height;
	bool running;
	SDL_Window *sdl_window;
	SDL_GLContext gl_context;
	SDL_Event event;

} YM_Window;
typedef struct {
	unsigned int vertex;
	unsigned int fragment;
	unsigned int program;
    
} YM_Shader;
typedef struct {
	unsigned int vao, vbo, ebo;
	YM_Shader *shader;
	vec2s transform;
	vec2s scale;
	vec4s color;
	mat4 model;
	float last_glyph_x;
	float glyph_size;
	unsigned int texture;
} YM_Element;

typedef struct {
	float x, y;
	float screen_x, screen_y;
	bool left_button_down;
} YM_Mouse;

typedef struct {
	unsigned int texture_id;
	vec2s size;
	vec2s bearing;
	unsigned int advance;
	char ascii;
} YM_Glyph;

typedef struct {
	YM_Glyph characters[128];
	mat4 projection;
	YM_Window *window;
} YM_Context;

typedef struct {
	char label_text[255];
	float bg_pos_x, bg_pos_y;
	float text_pos_x, text_pos_y;
	YM_Element *bg_element;
	YM_Element *text_element;
	YM_Shader *bg_shader;
} YM_Label;

typedef struct {
	size_t size;
	char **list;
} YM_String_List;

typedef struct {
	size_t size;
	YM_Label list[6];
	float line_offset;
} YM_Label_List;

typedef struct {
	float r,g,b,a;
} YM_RGBA;

enum YM_Border_Style {
	YM_BORDER,
	YM_NO_BORDER,
};

YM_Window ym_create_window();

char *ym_read_shader_file(const char *path);

void ym_swap_buffers(YM_Window *window);

void ym_clean_up(YM_Window *window);

void ym_free_element(YM_Element *element);

void ym_destroy_labels(YM_Label_List *labels, size_t label_count);

YM_Shader *ym_create_shader(const char *vertex, const char *fragment);

void ym_use_shader(YM_Shader *shader);

YM_Element *ym_render_rectangle(bool is_textured, const char *texture_path);

void ym_draw_element(YM_Element *element, YM_Shader *shader,
                     YM_Context *context, enum YM_Border_Style border);

float ym_convert_mouse_y(YM_Mouse *mouse);

bool ym_check_mouse_intersection(YM_Mouse mouse, YM_Element element);

bool ym_check_mouse_click(YM_Mouse *mouse, YM_Element *element);

void ym_create_text_renderer(YM_Context *context);

void ym_draw_text(const char *text, YM_Context *context, YM_Element *element);

void ym_set_position(YM_Element *element, float x, float y);

void ym_set_color_rgba(YM_Element *element, YM_RGBA color);

void ym_set_scale(YM_Element *element, float x, float y);

void ym_destroy_list(YM_String_List *list);

YM_Element *ym_render_text(const char *text, float x, float y, YM_Context *context, YM_RGBA color);

YM_Label *ym_render_label(const char *label_txt, float x, float y,
                          YM_Context *context, YM_RGBA text_color, YM_RGBA bg_color);

void ym_draw_label(YM_Label *label, YM_Context *context);

YM_String_List ym_map_directory(YM_Context *context);

void ym_cursor_point_to(YM_Element *cursor, float x, float y);

bool ym_search(YM_Label* label, const char* input);

bool ym_match(YM_Label* label, const char* input);

void ym_draw_label_list(YM_String_List *str_list,YM_Label_List *list, YM_Context *context, size_t nof_labels) ;

void ym_execute_app(const char* name_str, char** envp);

#endif
