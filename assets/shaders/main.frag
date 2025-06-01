#version 450

layout (location = 0) in vec2 texCoord;
uniform sampler u_tex;
layout (location = 0) out vec4 frag_color;
void main()
{
    frag_color = textureLod(u_tex, texCoord, 0.0);
}
