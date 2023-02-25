#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in float alpha;
uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, TexCoord);
    FragColor.a = alpha;
}