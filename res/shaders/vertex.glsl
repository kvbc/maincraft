#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aAlpha;

out float alpha;
out vec2 TexCoord;
uniform mat4 proj, view;

void main()
{
   gl_Position = proj * view * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
   TexCoord = aTexCoord;
   alpha = aAlpha;
}