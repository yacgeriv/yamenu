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