#include "SDL3/SDL_power.h"
#include "yamenu.h"
#include <stdio.h>
#include <unistd.h>
#define SHADERS_IMPL
#include "shaders.h"
#include "config.h"

const YM_RGBA BACKGROUND_COLOR = {(float) 0x20 / 255, (float) 0x20 / 255, (float) 0x20 / 255, (float) 0x20 / 255};
const YM_RGBA LABELTEXT_NORMAL_COLOR = {(float)0x39 / 255, (float)0x206 / 255, (float)0x64 / 255, 1.0f};
const YM_RGBA LABELBG_COLOR = {(float) 0x20 / 255, (float) 0x20 / 255, (float) 0x20 / 255, (float) 0x20 / 255};
const YM_RGBA CURSOR_COLOR = {0.0f, 0.87f, 1.0f, 1.0f};
const YM_RGBA LABELTEXT_HOVER_COLOR = {0.0f, 0.87f, 1.0f, 1.0};

char input[528];
uint32_t input_cursor = 0;

float cursor_target_x, cursor_target_y = 0;
uint32_t cursor_index = 0;

const uint32_t MAX_LABEL_COUNT = 6;

int main(int argc, char **argv, char** envp) {
	YM_Window ym_window;
	YM_Mouse mouse;
	YM_Context context;

	Settings user_settings;
	if(read_config_file(&user_settings)) {
	//	perror("failed to find config file under ~.config/yamenu/config.ini\n");
	}

	float labels_dynamic_loc[MAX_LABEL_COUNT];
	ym_window = ym_create_window();
    	
	context.window = &ym_window;
    
	ym_create_text_renderer(&context);
	YM_Shader *rect_shader =
		ym_create_shader(VERTEX_SHADER, FRAGMENT_SHADER);
   
	YM_Element *rect = ym_render_rectangle(false, "");
	ym_window.running = true;
 
	glm_ortho(0.0, ym_window.width, 0.0, ym_window.height, -2.0, 1.0f,
		  context.projection);

	SDL_Event event;

	ym_set_scale(rect, ym_window.width, 50);
	ym_set_position(rect, 0, ym_window.height - rect->scale.y);
	ym_set_color_rgba(rect, BACKGROUND_COLOR);

	YM_Element *input_text = ym_render_text(input, 10, ym_window.height - (rect->scale.y / 2), &context, LABELTEXT_NORMAL_COLOR);

	YM_String_List app_list;
	YM_Label_List labels;

	app_list = ym_map_directory(&context);

	YM_Shader *banner_shader =
		ym_create_shader(VERTEX_SHADER, TEXTURE_F_SHADER);
	YM_Element *banner = ym_render_rectangle(true, user_settings.background_path);
    
	ym_set_scale(banner, ym_window.width, 120);
	ym_set_position(banner, 0, ym_window.height - 160);
    
	YM_Shader *cursor_block_shader =
		ym_create_shader(VERTEX_SHADER, FRAGMENT_SHADER);
	YM_Element *cursor_block = ym_render_rectangle(false, "");

	ym_set_scale(cursor_block, 15, 25);
	ym_set_position(cursor_block, (ym_window.width - cursor_block->scale.x) / 2.2,
			(ym_window.height - cursor_block->scale.y) / 2);
	ym_set_color_rgba(cursor_block, CURSOR_COLOR);

	labels.line_offset = 340;
    
	for (uint32_t i = 0; i < MAX_LABEL_COUNT; i++) {
		labels.line_offset -= 50;
		labels.list[i] = *ym_render_label(app_list.list[i], 0, labels.line_offset,
						  &context, LABELTEXT_NORMAL_COLOR, LABELBG_COLOR);
	}
	labels.line_offset = 340;

	SDL_StartTextInput(ym_window.sdl_window);
	bool is_typing = false;
	cursor_target_y = 500 - cursor_block->scale.y;
	while (ym_window.running) {
		SDL_SetWindowKeyboardGrab(ym_window.sdl_window, true);
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
					if (cursor_index < MAX_LABEL_COUNT - 1) {
						cursor_index++;
						cursor_target_x =
							labels.list[cursor_index].text_element->last_glyph_x + cursor_block->scale.x;
						cursor_target_y =
							labels.list[cursor_index].text_element->transform.y;
					} else {
						cursor_index = 0;
						cursor_target_x =
							labels.list[cursor_index].text_element->last_glyph_x + cursor_block->scale.x;
						cursor_target_y =
							labels.list[cursor_index].text_element->transform.y;
					}
				}
				if (event.key.key == SDLK_UP) {
					if (cursor_index != 0) {
						cursor_index--;
						cursor_target_x =
							labels.list[cursor_index].text_element->last_glyph_x + cursor_block->scale.x;
						cursor_target_y =
							labels.list[cursor_index].text_element->transform.y;
					}
				}
				if (event.key.key == SDLK_RETURN) {
					if (cursor_index >= 0) {
						printf("%d\n", cursor_index);
                        ym_execute_app(labels.list[cursor_index].label_text, envp);
                        ym_destroy_list(&app_list);
						ym_clean_up(&ym_window);
					}
				}
				if (event.key.key == SDLK_ESCAPE) {
					ym_destroy_list(&app_list);
					ym_clean_up(&ym_window);
					return 0;
				}
				break;
			case SDL_EVENT_TEXT_INPUT:                
				if (!is_typing && input_cursor < sizeof(input)) {                    
					input[input_cursor++] = event.text.text[0];
					cursor_target_x =
						input_text->last_glyph_x + cursor_block->scale.x * 2.1;
					cursor_target_y = input_text->transform.y - 2.6;
					uint32_t match_count = 0;
					if (strlen(input) > 0 ) {
						labels.line_offset = 340;
						for (uint32_t i = 0; i < app_list.size; i++) {
							if (strstr(app_list.list[i] , input) && match_count < MAX_LABEL_COUNT) {
								labels.line_offset -= 50;
								labels.list[match_count] = *ym_render_label(app_list.list[i], 0, labels.line_offset, &context, LABELTEXT_NORMAL_COLOR, LABELBG_COLOR);
								match_count++;
							}
						}
					}
					cursor_index = 0;
					break;
				}
				break;
			default:
				mouse.left_button_down = false;
				break;
			}
		}
        
		for (uint32_t i = 0; i < MAX_LABEL_COUNT; i++) {
			if (ym_check_mouse_intersection(mouse, *labels.list[i].bg_element)) {
				ym_set_color_rgba(labels.list[i].text_element, LABELTEXT_HOVER_COLOR);
			}else{
				ym_set_color_rgba(labels.list[i].text_element, LABELTEXT_NORMAL_COLOR);
			}
			if (ym_check_mouse_click(&mouse, labels.list[i].bg_element)) {
				ym_execute_app(labels.list[i].label_text, envp);
                ym_clean_up(&ym_window);
				ym_destroy_list(&app_list);
				ym_destroy_labels(&labels, MAX_LABEL_COUNT);
				ym_free_element(rect);
				ym_free_element(banner);
				ym_free_element(cursor_block);
				free(rect_shader);
				free(banner_shader);
				free(cursor_block_shader);
			}
		}
        
		ym_cursor_point_to(cursor_block, cursor_target_x, cursor_target_y);

		glViewport(0, 0, ym_window.width, ym_window.height);
		glClearColor(BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
		glClear(GL_COLOR_BUFFER_BIT);

		ym_draw_element(rect, rect_shader, &context, YM_NO_BORDER);
		ym_draw_text(input, &context, input_text);
		ym_draw_label_list(&app_list, &labels, &context, MAX_LABEL_COUNT);
		ym_draw_element(banner, banner_shader, &context, YM_NO_BORDER);
		ym_draw_element(cursor_block, cursor_block_shader, &context, YM_NO_BORDER);
        
		ym_swap_buffers(&ym_window);
	}
    
	ym_clean_up(&ym_window);
	ym_destroy_list(&app_list);
	ym_destroy_labels(&labels, MAX_LABEL_COUNT);
	ym_free_element(rect);
	ym_free_element(banner);
	ym_free_element(cursor_block);
	free(rect_shader);
	free(banner_shader);
	free(cursor_block_shader);
    
	return 0;
}
