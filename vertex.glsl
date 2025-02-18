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
