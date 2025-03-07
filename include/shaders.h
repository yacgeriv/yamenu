#ifndef SHADERS_INC_H
#define SHADERS_INC_H

extern const char *VERTEX_SHADER;
extern const char *FRAGMENT_SHADER;
extern const char *TEXTURE_F_SHADER;
extern const char *GLYPH_V_SHADER;
extern const char *GLYPH_F_SHADER;

#ifdef SHADERS_IMPL
const char *VERTEX_SHADER = R"glsl(
#version 430 core

layout (location = 0) in vec4 vertex;

uniform mat4 model;
uniform mat4 projection;

out vec2 textureCoords;

void main()
{
   gl_Position = projection * model * vec4(vertex.xy, 0.0f, 1.0);
   textureCoords = vertex.zw;
}
)glsl";

const char *FRAGMENT_SHADER = R"glsl(
#version 430 core
out vec4 FragColor;

uniform vec4 color;
uniform float noise;

in vec2 textureCoords;

uniform float time;

void main()
{
	FragColor = color;
}
)glsl";

const char *TEXTURE_F_SHADER = R"glsl(
#version 430 core
                                  
out vec4 FragColor;

uniform vec4 color;
uniform float noise;
uniform sampler2D tex;

in vec2 textureCoords;

uniform float time;

void main()
{
	FragColor = texture(tex, textureCoords);
}
)glsl";

const char *GLYPH_V_SHADER = R"glsl(
#version 430 core

layout (location = 0) in vec4 vertex;

out vec2 textureCoords;

uniform mat4 projection;

void main()
{
	gl_Position = projection * vec4(vertex.xy, -1.0f, 1.0f);
	textureCoords = vertex.zw;	
}
)glsl";

const char *GLYPH_F_SHADER = R"glsl(
#version 430 core

out vec4 FragColor;
in vec2 textureCoords;

uniform sampler2D glyph_texture;
uniform vec4 color;

void main()
{
	vec4 sampler = vec4(1.0f, 1.0f, 1.0f , texture(glyph_texture, textureCoords).r);
	FragColor = color * sampler;
}
)glsl";

#endif

#endif
