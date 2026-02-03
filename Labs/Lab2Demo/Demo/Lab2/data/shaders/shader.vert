#version 400 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;



layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inCoord;
layout (location = 2) in vec3 inNormal;

out vec3 vColour;
out vec2 vTexCoord;
out vec3 vNormal;

void main()
{
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	
	mat3 normalMatrix = transpose(inverse(mat3(viewMatrix*modelMatrix)));
	vNormal = normalMatrix*inNormal; 

	vTexCoord = inCoord; 

}