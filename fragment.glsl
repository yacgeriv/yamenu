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
