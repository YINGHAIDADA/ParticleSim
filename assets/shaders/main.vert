#version 450

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texCoord;
uniform int u_flip_y;
layout (location = 0) out vec2 texCoord;
void main()
{
	gl_Position = vec4(a_pos, 0.0, 1.0);
	texCoord = vec2(a_texCoord.x, bool(u_flip_y) ? 1.0 - a_texCoord.y : a_texCoord.y);
}