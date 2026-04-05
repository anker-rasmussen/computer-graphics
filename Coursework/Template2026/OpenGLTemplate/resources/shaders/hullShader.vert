// Hull vertex shader -- same layout as sailShader, passes data to fragment for neon glow

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
layout (location = 3) in vec3 inColour;

out vec3 vEyeNorm;
out vec3 vEyePos;
out vec2 vTexCoord;
out vec3 vColour;

void main()
{
	vec4 eyePos4 = matrices.modelViewMatrix * vec4(inPosition, 1.0f);
	vEyePos = eyePos4.xyz;
	vEyeNorm = normalize(matrices.normalMatrix * inNormal);
	vTexCoord = inCoord;
	vColour = inColour;
	gl_Position = matrices.projMatrix * eyePos4;
}
