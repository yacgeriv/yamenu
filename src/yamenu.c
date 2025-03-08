#define STB_IMAGE_IMPLEMENTATION
#include "yamenu.h"
#include "shaders.h"

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

	tmp_window.sdl_window = SDL_CreateWindow(
											 tmp_window.title, tmp_window.width, tmp_window.height,
                                             SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
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
	fclose(shader_file);
	return dest;
}
void ym_swap_buffers(YM_Window *window) {
	SDL_GL_SwapWindow(window->sdl_window);
}
void ym_clean_up(YM_Window *window) {
	SDL_DestroyWindow(window->sdl_window);
	SDL_Quit();
}
void ym_free_element(YM_Element *element) {
    free(element);
}
void ym_destroy_labels(YM_Label_List *labels, size_t label_count) {
    for (uint32_t i = 0; i < label_count; i++) {
        free(labels->list[i].bg_shader);
        free(labels->list[i].text_element->shader);
    }
}
YM_Shader *ym_create_shader(const char *vertex,
                            const char *fragment) {

	YM_Shader *shader;
	shader = (YM_Shader *)malloc(sizeof(YM_Shader));

	shader->vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader->vertex, 1, &vertex, NULL);
	glCompileShader(shader->vertex);

	int success;
	char info_log[512];
	glGetShaderiv(shader->vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader->vertex, 512, NULL, info_log);
		printf("[ERROR] vertex shader : %s \n", info_log);
	}

	shader->fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader->fragment, 1, &fragment, NULL);
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

	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);

	return shader;
}
void ym_use_shader(YM_Shader *shader) {
    glUseProgram(shader->program);
}
YM_Element *ym_render_rectangle(bool is_textured, const char *texture_path) {
	YM_Element *element;
	element = (YM_Element *)malloc(sizeof(YM_Element));

	const float vertices[] = {
		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };

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

    if (is_textured && strlen(texture_path) > 0) {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(texture_path, &width, &height, &nrChannels, 0);
        
        glGenTextures(1, &element->texture);
        glBindTexture(GL_TEXTURE_2D, element->texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    
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

    if (element->texture != 0) {
        glBindTexture(GL_TEXTURE_2D, element->texture);
        glUniform1i(glGetUniformLocation(shader->program, "tex"), 0);
    }

	glBindVertexArray(element->vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}
float ym_convert_mouse_y(YM_Mouse *mouse) {
    return -mouse->y + 500;
}
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

	if (FT_New_Face(font, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0, &font_face)) {
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
void ym_set_color_rgba(YM_Element *element, YM_RGBA color) {
	element->color.x = color.r;
	element->color.y = color.g;
	element->color.z = color.b;
	element->color.w = color.a;
}
void ym_set_scale(YM_Element *element, float x, float y) {
	element->scale.x = x;
	element->scale.y = y;
}
void ym_destroy_list(YM_String_List *list) {
	for (uint32_t i = 0; i < list->size; i++) {
        free(list->list[i]);
	}
	free(list->list);
}
YM_Element *ym_render_text(const char *text, float x, float y,
                           YM_Context *context, YM_RGBA color) {
	YM_Element *element;
	element = (YM_Element *)malloc(sizeof(YM_Element));

	element->transform.x = x;
	element->transform.y = y;

	element->shader = ym_create_shader(GLYPH_V_SHADER, GLYPH_F_SHADER);

	ym_set_scale(element, 0.5, 0.5);
	ym_set_position(element, element->transform.x, element->transform.y);
	ym_set_color_rgba(element, color);

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
                          YM_Context *context, YM_RGBA text_color, YM_RGBA bg_color) {
	YM_Label *label;
	label = (YM_Label *)malloc(sizeof(YM_Label));

	float text_offset = 10.f;

	label->bg_pos_x = x;
	label->bg_pos_y = y;

	strncpy(label->label_text, label_txt, 254);

	label->bg_element = ym_render_rectangle(false, "");

	ym_set_scale(label->bg_element, context->window->width, 40.0f);
	ym_set_position(label->bg_element, x, y);
	ym_set_color_rgba(label->bg_element, bg_color);

	label->bg_shader =
		ym_create_shader(VERTEX_SHADER, FRAGMENT_SHADER);
	label->text_element =
		ym_render_text(label_txt, label->bg_element->transform.x + text_offset,
					   label->bg_element->transform.y, context, text_color);

	ym_set_position(label->text_element, label->text_element->transform.x,
					label->bg_element->transform.y -
					(label->text_element->glyph_size) / 2);

	return label;
}
void ym_draw_label(YM_Label *label, YM_Context *context) {
   	ym_draw_element(label->bg_element, label->bg_shader, context, YM_NO_BORDER);
	ym_draw_text(label->label_text, context, label->text_element);
}
YM_String_List ym_map_directory(YM_Context *context) {
	YM_String_List list;

    DIR *directory;

	struct dirent *dirent_pointer;

#ifdef __linux__
    directory = opendir("/usr/bin/");
#endif

#ifdef __MINGW32__
    directory = opendir("C:/ProgramData/Microsoft/Windows/Start Menu/Programs/");
#endif
    
	char **app_list = (char **)malloc(sizeof(char *));

	size_t size = 0;
	
	while ((dirent_pointer = readdir(directory))) {
        if (strlen(dirent_pointer->d_name) > 0 && strcmp(dirent_pointer->d_name, "..")
            && strcmp(dirent_pointer->d_name, ".")) {
			size++;
			app_list = (char **)realloc(app_list, sizeof(char *) * size);
			app_list[size - 1] = (char *)malloc(sizeof(char)* 254);
            char temp_str[255] = {'\0'};
            for (uint32_t i = 0; i < 254; i++) {
                if (dirent_pointer->d_name[i] == '.' && dirent_pointer->d_name[i + 1] == 'd') {
                    break;
                }else {
                    temp_str[i] = dirent_pointer->d_name[i];
                }
            }
			strcpy(app_list[size - 1], temp_str);
		}
	}
	free(dirent_pointer);
	closedir(directory);
	
	list.list = app_list;
	list.size = size;
	
	return list;
}
void ym_cursor_point_to(YM_Element *cursor, float x, float y) {
	vec3s diff = {cursor->transform.x - x, cursor->transform.y - y};
	glms_normalize(diff);

	cursor->transform.x -= diff.x * 0.2;
	cursor->transform.y -= diff.y * 0.2;
}
bool ym_search(YM_Label* label, const char* input) {
    if (strstr(label->label_text, input)) {
        return true;
    }
    return false;
}
bool ym_match(YM_Label* label, const char* input) {
    if (strcmp(label->label_text, input) == false) {
        return true;
    }
    return false;
}
void ym_draw_label_list(YM_String_List *str_list,YM_Label_List *list, YM_Context *context, size_t nof_labels) {
    for (uint32_t i = 0; i < nof_labels; i++) {
        ym_draw_label(&list->list[i], context);
    }
}
void ym_execute_app(const char* name_str) {
    char user_binary_path[255] = "/usr/bin/";    
    char *arg[] = {(char*)name_str, 0};
    strcat(user_binary_path, name_str);
    
    execvp(user_binary_path, arg);
}
