#version 400

in vec3 vColour;
out vec4 outputColour;

void main()
{
	outputColour = vec4(vColour, 1.0);
}