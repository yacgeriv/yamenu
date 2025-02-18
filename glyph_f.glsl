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
