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
#include FT_FREETYPE_H
#include <dirent.h>

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
	YM_Shader bg_shader, text_shader;
} YM_Label;
typedef struct {
	size_t size;
	YM_Label **list;

} YM_Label_List;

enum YM_Border_Style { YM_BORDER, YM_NO_BORDER };

char input[528];
uint32_t input_cursor = 0;

float cursor_target_x, cursor_target_y = 0;
int cursor_index = 0;

YM_Window ym_create_window() {
	YM_Window tmp_window;
	tmp_window.title = "yamenu";
	tmp_window.width = 800;
	tmp_window.height = 500;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// sdl_SetHint(SDL_HINT_X11_WINDOW_TYPE, "_NET_WM_WINDOW_TYPE_DESKTOP");
	tmp_window.sdl_window = SDL_CreateWindow(
											 tmp_window.title, tmp_window.width, tmp_window.height, SDL_WINDOW_OPENGL);
	if (!tmp_window.sdl_window) {
		fprintf(stderr, "SDL window failed to initialise: %s\n", SDL_GetError());
	}
	tmp_window.gl_context = SDL_GL_CreateContext(tmp_window.sdl_window);
	if (!tmp_window.gl_context) {
		perror("failed to create opengl context");
	}
	SDL_GL_MakeCurrent(tmp_window.sdl_window, tmp_window.gl_context);

	if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) == 0) {
		perror("failed to load glad");
	}

	// glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return tmp_window;
}
char *ym_read_shader_file(const char *path) {
	FILE *shader_file;
	uint32_t shader_size = 0;
	char *dest;
	uint32_t i = 0;
	char c;

	shader_file = fopen(path, "r");
	if (!shader_file) {
		perror("could not find/open shader file\n");
		exit(1);
	}

	fseek(shader_file, 0, SEEK_END);

	shader_size = ftell(shader_file);

	fseek(shader_file, 0, SEEK_SET);

	dest = malloc(sizeof(char) * (shader_size) + 1);

	while ((c = fgetc(shader_file)) != EOF) {
		dest[i] = c;
		i++;
	}

	dest[i] = '\0';
	return dest;
}
void ym_swap_buffers(YM_Window *window) {
	SDL_GL_SwapWindow(window->sdl_window);
}
void ym_clean_up(YM_Window *window) {
	SDL_DestroyWindow(window->sdl_window);
	SDL_Quit();
}
typedef enum { YM_QUIT, YM_MOUSE_MOTION, YM_NONE } YM_Event;
YM_Event ym_handle_events(SDL_EventType event_type) {
	switch (event_type) {
	case SDL_EVENT_QUIT:
		return YM_QUIT;
	case SDL_EVENT_MOUSE_MOTION:
		return YM_MOUSE_MOTION;

	default:
		return YM_NONE;
	}
	return YM_NONE;
}
YM_Shader *ym_create_shader(const char *vertex_path,
                            const char *fragment_path) {
	const char *vertex_shader_data = ym_read_shader_file(vertex_path);
	const char *fragment_shader_data = ym_read_shader_file(fragment_path);

	YM_Shader *shader;
	shader = (YM_Shader *)malloc(sizeof(YM_Shader));

	shader->vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader->vertex, 1, &vertex_shader_data, NULL);
	glCompileShader(shader->vertex);

	int success;
	char info_log[512];
	glGetShaderiv(shader->vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader->vertex, 512, NULL, info_log);
		printf("[ERROR] vertex shader : %s \n", info_log);
	}

	shader->fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader->fragment, 1, &fragment_shader_data, NULL);
	glCompileShader(shader->fragment);

	glGetShaderiv(shader->fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader->fragment, 512, NULL, info_log);
		printf("[ERROR] fragment shader : %s\n", info_log);
	}

	shader->program = glCreateProgram();
	glAttachShader(shader->program, shader->vertex);
	glAttachShader(shader->program, shader->fragment);
	glLinkProgram(shader->program);

	glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader->program, 512, NULL, info_log);
		printf("[ERROR] program shader : %s\n", info_log);
	}

	free((char *)fragment_shader_data);
	free((char *)vertex_shader_data);

	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);

	return shader;
}
void ym_use_shader(YM_Shader *shader) { glUseProgram(shader->program); }
YM_Element *ym_render_rectangle() {
	YM_Element *element;
	element = (YM_Element *)malloc(sizeof(YM_Element));

	const float vertices[] = {
		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f};

	glGenVertexArrays(1, &element->vao);

	glGenBuffers(1, &element->vbo);
	glBindVertexArray(element->vao);

	glBindBuffer(GL_ARRAY_BUFFER, element->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	element->transform.x = 1;
	element->transform.y = 1;

	element->scale.x = 20.0f;
	element->scale.y = 20.0f;
	element->color.x = 0.0f;
	element->color.y = 1.0f;
	element->color.z = 0.0f;
	element->color.w = 1.0f;

	return element;
}
void ym_draw_element(YM_Element *element, YM_Shader *shader,
                     YM_Context *context, enum YM_Border_Style border) {
	glm_mat4_identity(element->model);
	glm_translate_x(element->model, element->transform.x);
	glm_translate_y(element->model, element->transform.y);
	glm_scale(element->model, element->scale.raw);

	ym_use_shader(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader->program, "model"), 1,
					   GL_FALSE, element->model[0]);
	glUniformMatrix4fv(glGetUniformLocation(shader->program, "projection"), 1,
					   GL_FALSE, context->projection[0]);
	glUniform4fv(glGetUniformLocation(shader->program, "color"), 1,
				 element->color.raw);
	glUniform1f(glGetUniformLocation(shader->program, "time"),
				SDL_GetTicks() * 0.0002);
	if (border == YM_BORDER) {
		// TODO :: add border
	}

	glBindVertexArray(element->vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}
float ym_convert_mouse_y(YM_Mouse *mouse) { return -mouse->y + 500; }
bool ym_check_mouse_intersection(YM_Mouse mouse, YM_Element element) {
	float mouse_y = ym_convert_mouse_y(&mouse);

	if (mouse.x > element.transform.x &&
		mouse.x < element.transform.x + (element.scale.x)) {
		if (mouse_y > element.transform.y &&
			mouse_y < element.transform.y + (element.scale.y)) {
			return true;
		}
	}
	return false;
}
bool ym_check_mouse_click(YM_Mouse *mouse, YM_Element *element) {
	if (ym_check_mouse_intersection(*mouse, *element) &&
		mouse->left_button_down) {
		printf("mouse clicked on element\n");
		mouse->left_button_down = false;
		return true;
	}
	return false;
}
void ym_create_text_renderer(YM_Context *context) {
	FT_Library font;
	if (FT_Init_FreeType(&font)) {
		perror("failed to init font\n");
	}

	FT_Face font_face;

	if (FT_New_Face(font, "fonts/OpenSans-Regular.ttf", 0, &font_face)) {
		perror("couldn't load the fonts\n");
	}

	FT_Set_Pixel_Sizes(font_face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (unsigned char c; c < 128; c++) {
		if (FT_Load_Char(font_face, c, FT_LOAD_RENDER)) {
			perror("failed to load character\n");
		}

		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font_face->glyph->bitmap.width,
					 font_face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
					 font_face->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		YM_Glyph glyph;
		glyph.texture_id = texture;
		glyph.size.x = font_face->glyph->bitmap.width;
		glyph.size.y = font_face->glyph->bitmap.rows;
		glyph.bearing.x = font_face->glyph->bitmap_left;
		glyph.bearing.y = font_face->glyph->bitmap_top;
		glyph.advance = font_face->glyph->advance.x;
		glyph.ascii = c;

		context->characters[c] = glyph;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(font_face);
	FT_Done_FreeType(font);
}
void ym_draw_text(const char *text, YM_Context *context, YM_Element *element) {
	int x = element->transform.x;
	int y = element->transform.y;
	element->last_glyph_x = 0;

	glUseProgram(element->shader->program);

	glUniform4fv(glGetUniformLocation(element->shader->program, "color"), 1,
				 element->color.raw);
	glUniformMatrix4fv(
					   glGetUniformLocation(element->shader->program, "projection"), 1, GL_FALSE,
					   context->projection[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(element->vao);

	for (uint32_t i = 0; i < strlen(text); i++) {
		YM_Glyph current_glyph;
		for (uint32_t j = 0; sizeof(context->characters); j++) {
			if (context->characters[j].ascii == text[i]) {

				current_glyph = context->characters[j];
				break;
			}
		}

		float xpos = x + current_glyph.bearing.x * element->scale.x;
		float ypos =
			y - (current_glyph.size.y - current_glyph.bearing.y) * element->scale.x;

		float w = current_glyph.size.x * element->scale.x;
		float h = current_glyph.size.y * element->scale.x;

		element->last_glyph_x = xpos;
		element->glyph_size = h;

		float vertices[6][4] = {
			{xpos, ypos + h, 0.0f, 0.0f},    {xpos, ypos, 0.0f, 1.0f},
			{xpos + w, ypos, 1.0f, 1.0f},

			{xpos, ypos + h, 0.0f, 0.0f},    {xpos + w, ypos, 1.0f, 1.0f},
			{xpos + w, ypos + h, 1.0f, 0.0f}};

		glBindTexture(GL_TEXTURE_2D, current_glyph.texture_id);
		glBindBuffer(GL_ARRAY_BUFFER, element->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (current_glyph.advance >> 6) * element->scale.x;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void ym_set_position(YM_Element *element, float x, float y) {
	element->transform.x = x;
	element->transform.y = y;
}
void ym_set_color_rgb(YM_Element *element, float r, float g, float b) {
	element->color.x = r;
	element->color.y = g;
	element->color.z = b;
	element->color.w = 1.0f;
}
void ym_set_scale(YM_Element *element, float x, float y) {
	element->scale.x = x;
	element->scale.y = y;
}
void ym_destroy_list(YM_Label_List *list) {
	for (uint32_t i = 0; i < list->size; i++) {
		free(list->list[i]);
	}
	free(list->list);
}
YM_Element *ym_render_text(const char *text, float x, float y,
                           YM_Context *context) {
	YM_Element *element;
	element = (YM_Element *)malloc(sizeof(YM_Element));

	element->transform.x = x;
	element->transform.y = y;

	element->shader = ym_create_shader("glyph_v.glsl", "glyph_f.glsl");

	ym_set_scale(element, 0.5, 0.5);
	ym_set_position(element, element->transform.x, element->transform.y);
	ym_set_color_rgb(element, (float)0x39 / 255, (float)0x206 / 255,
					 (float)0x64 / 255);

	glGenVertexArrays(1, &element->vao);
	glGenBuffers(1, &element->vbo);
	glBindVertexArray(element->vao);
	glBindBuffer(GL_ARRAY_BUFFER, element->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return element;
}
YM_Label *ym_render_label(const char *label_txt, float x, float y,
                          YM_Context *context) {
	YM_Label *label;
	label = (YM_Label *)malloc(sizeof(YM_Label));

	float text_offset = 10.f;

	label->bg_pos_x = x;
	label->bg_pos_y = y;

	strncpy(label->label_text, label_txt, 254);

	YM_Element *label_bg = ym_render_rectangle();

	ym_set_scale(label_bg, context->window->width, 40.0f);
	ym_set_position(label_bg, x, y);
	ym_set_color_rgb(label_bg, (float)0x20 / 255, (float)0x20 / 255,
					 (float)0x20 / 255);

	label->bg_shader =
		*ym_create_shader("build/vertex.glsl", "build/fragment.glsl");
	label->text_element =
		ym_render_text(label_txt, label_bg->transform.x + text_offset,
					   label_bg->transform.y, context);
	label->bg_element = label_bg;

	ym_set_position(label->text_element, label->text_element->transform.x,
					label->bg_element->transform.y -
					(label->text_element->glyph_size) / 2);

	return label;
}
void ym_draw_label(YM_Label *label, YM_Context *context) {
	ym_draw_element(label->bg_element, &label->bg_shader, context, YM_NO_BORDER);
	ym_draw_text(label->label_text, context, label->text_element);
}
YM_Label_List ym_map_directory(YM_Context *context) {
#ifdef WIN32

	DIR *directory;

	struct dirent *dirent_pointer;
	directory = opendir("C:/ProgramData/Microsoft/Windows/Start Menu/Programs");

	YM_Label **app_list = (YM_Label **)malloc(sizeof(YM_Label *));

	size_t size = 0;
	float offset_y = 500;

	while ((dirent_pointer = readdir(directory))) {
		if (strstr(dirent_pointer->d_name, ".lnk")) {
			char temp_str[255];
			strncpy(temp_str, dirent_pointer->d_name, 254);
			offset_y -= 100;
			size++;
			app_list = (YM_Label **)realloc(app_list, sizeof(YM_Label *) * size);
			app_list[size - 1] = (YM_Label *)malloc(sizeof(YM_Label));
			app_list[size - 1] = ym_render_label(temp_str, 0, offset_y, context);
		}
	}
	free(dirent_pointer);

	YM_Label_List list;
	list.list = app_list;
	list.size = size;

	return list;

#endif

#ifdef LINUX
#endif
}
void ym_cursor_point_to(YM_Element *cursor, float x, float y) {
	vec3s diff = {cursor->transform.x - x, cursor->transform.y - y};
	glms_normalize(diff);

	cursor->transform.x -= diff.x * 0.2;
	cursor->transform.y -= diff.y * 0.2;
}
void ym_draw_label_list(YM_Label_List *list, YM_Context *context) {
	for (int32_t i = 0; i < list->size; i++) {
		ym_draw_label(list->list[i], context);
	}
}
int main(int argc, char **argv) {
	YM_Window ym_window;
	YM_Mouse mouse;
	YM_Context context;

	ym_window = ym_create_window();

	context.window = &ym_window;

	ym_create_text_renderer(&context);

	YM_Shader *rect_shader =
		ym_create_shader("build/vertex.glsl", "build/fragment.glsl");

	YM_Element *rect = ym_render_rectangle();

	ym_window.running = true;

	glm_ortho(0.0, ym_window.width, 0.0, ym_window.height, -2.0, 1.0f,
			  context.projection);

	SDL_Event event;

	// NOTE:------input area-------

	ym_set_scale(rect, ym_window.width, 50);
	ym_set_position(rect, 0, ym_window.height - rect->scale.y);
	ym_set_color_rgb(rect, (float)0x20 / 255, (float)0x20 / 255,
					 (float)0x20 / 255);

	YM_Element *input_text = ym_render_text(
											input, 10, ym_window.height - (rect->scale.y / 2), &context);

	YM_Label_List app_list;

	app_list = ym_map_directory(&context);

	YM_Shader *cursor_block_shader =
		ym_create_shader("build/vertex.glsl", "build/fragment.glsl");
	YM_Element *cursor_block = ym_render_rectangle();

	ym_set_scale(cursor_block, 15, 25);
	ym_set_position(cursor_block, (ym_window.width - cursor_block->scale.x) / 2.2,
					(ym_window.height - cursor_block->scale.y) / 2);
	ym_set_color_rgb(cursor_block, 0.0f, 0.87f, 1.0f);

	SDL_StartTextInput(ym_window.sdl_window);
	bool is_typing = false;

	while (ym_window.running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				ym_window.running = false;
				break;
			case SDL_EVENT_MOUSE_MOTION:
				SDL_GetMouseState(&mouse.x, &mouse.y);
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					mouse.left_button_down = true;
				}
				break;
			case SDL_EVENT_KEY_DOWN:
				if (event.key.key == SDLK_BACKSPACE) {
					if (input_cursor > 0) {
						is_typing = true;
						input[--input_cursor] = '\0';
						cursor_target_x = input_text->last_glyph_x;
						cursor_target_y = input_text->transform.y - 2.6;
						is_typing = false;
					}
				}
				if (event.key.key == SDLK_DOWN) {
					if (cursor_index < app_list.size) {
						cursor_target_x =
							app_list.list[cursor_index]->text_element->transform.x;
						cursor_target_y =
							app_list.list[cursor_index]->text_element->transform.y;
						cursor_index++;
					} else {
						cursor_index = 0;
					}
				}
				if (event.key.key == SDLK_UP) {
					if (cursor_index != 0) {
						cursor_index--;
						cursor_target_x =
							app_list.list[cursor_index]->text_element->transform.x;
						cursor_target_y =
							app_list.list[cursor_index]->text_element->transform.y;
					}
				}
				break;
			case SDL_EVENT_TEXT_INPUT:
				if (!is_typing && input_cursor < sizeof(input)) {
					input[input_cursor++] = event.text.text[0];
					cursor_target_x =
						input_text->last_glyph_x + cursor_block->scale.x * 2.1;
					cursor_target_y = input_text->transform.y - 2.6;
					break;
				}
			default:
				mouse.left_button_down = false;
				break;
			}
		}

		ym_cursor_point_to(cursor_block, cursor_target_x, cursor_target_y);

		glViewport(0, 0, ym_window.width, ym_window.height);
		glClearColor((float)0x20 / 255, (float)0x20 / 255, (float)0x20 / 255,
					 (float)0x20 / 255);
		glClear(GL_COLOR_BUFFER_BIT);

		ym_draw_element(rect, rect_shader, &context, YM_NO_BORDER);
		ym_draw_text(input, &context, input_text);
		ym_draw_label_list(&app_list, &context);
		ym_draw_element(cursor_block, cursor_block_shader, &context, YM_NO_BORDER);
		ym_swap_buffers(&ym_window);
	}

	ym_clean_up(&ym_window);
	ym_destroy_list(&app_list);
	return 0;
}
