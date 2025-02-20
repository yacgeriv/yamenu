#version 430 core

out vec4 FragColor;

uniform float border_width;
uniform float aspect;

uniform vec4 color;
in vec2 textureCoords;

void main()
{
   float maxX = 1.0 - border_width;
   float minX = border_width;
   float maxY = maxX / aspect;
   float minY = minX / aspect;

   if (textureCoords.x < maxX && textureCoords.x > minX &&
       textureCoords.y < maxY && textureCoords.y > minY)
   {
       FragColor = color;
   } else
   {
       FragColor = vec4(0.02, 0.64, 0.11, 1.0);
   }
}