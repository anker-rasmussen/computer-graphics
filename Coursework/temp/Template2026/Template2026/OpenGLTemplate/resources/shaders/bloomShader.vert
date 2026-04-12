#version 400 core

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

out vec2 vTexCoord;

void main()
{
	gl_Position = vec4(inPosition * 2.0 - 1.0, 0.0, 1.0);
	vTexCoord = inTexCoord;
}
