#version 400 core

uniform mat4 projMatrix;
uniform mat4 viewMatrix;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inCoord;
layout (location = 2) in vec4 inColour;

out vec2 vTexCoord;
out vec4 vColour;

void main() {
	// Positions are in world space (billboarded on CPU), transform through view + proj
	gl_Position = projMatrix * viewMatrix * vec4(inPosition, 1.0);
	vTexCoord = inCoord;
	vColour = inColour;
}
