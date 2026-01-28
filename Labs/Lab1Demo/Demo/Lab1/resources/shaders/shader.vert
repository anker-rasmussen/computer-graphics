#version 400

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform float t;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColour;

out vec3 vColour;

void main()
{
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	vColour = inColour *(0.5+5*sin(t*10));
}