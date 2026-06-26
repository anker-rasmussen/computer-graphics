// Hologram vertex shader -- used for the bridge viewport schematic display.

#version 400 core

uniform struct Matrices
{
	mat4 projMatrix;
	mat4 modelViewMatrix;
	mat3 normalMatrix;
} matrices;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inCoord;

out vec3 vEyeNorm;
out vec3 vEyePos;
out vec3 vModelPos;

void main()
{
	vec4 eyePos4 = matrices.modelViewMatrix * vec4(inPosition, 1.0);
	vEyePos = eyePos4.xyz;
	vEyeNorm = normalize(matrices.normalMatrix * inNormal);
	vModelPos = inPosition;
	gl_Position = matrices.projMatrix * eyePos4;
}
