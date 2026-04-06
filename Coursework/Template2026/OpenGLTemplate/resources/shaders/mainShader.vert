#version 400 core

// Structure for matrices
uniform struct Matrices
{
	mat4 projMatrix;
	mat4 modelViewMatrix;
	mat3 normalMatrix;
} matrices;

// Light and material structs (used in fragment shader, declared here for interface)
struct LightInfo
{
	vec4 position;
	vec3 La;
	vec3 Ld;
	vec3 Ls;
};

struct MaterialInfo
{
	vec3 Ma;
	vec3 Md;
	vec3 Ms;
	float shininess;
};

// Shadow mapping
uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;

// Layout of vertex attributes in VBO
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inCoord;
layout (location = 2) in vec3 inNormal;

// Outputs to fragment shader
out vec3 vEyeNorm;
out vec3 vEyePos;
out vec2 vTexCoord;
out vec3 worldPosition;
out vec4 vLightSpacePos;
out vec4 vSpotLightSpacePos;

void main()
{
	// Save the world position for rendering the skybox
	worldPosition = inPosition;

	// Transform the vertex spatial position
	gl_Position = matrices.projMatrix * matrices.modelViewMatrix * vec4(inPosition, 1.0);

	// Eye-space normal and position for per-fragment lighting
	vEyeNorm = normalize(matrices.normalMatrix * inNormal);
	vec4 eyePos4 = matrices.modelViewMatrix * vec4(inPosition, 1.0);
	vEyePos = eyePos4.xyz;

	// Light-space positions for shadow mapping
	vLightSpacePos = lightSpaceMatrix * vec4(inPosition, 1.0);
	vSpotLightSpacePos = spotLightSpaceMatrix * vec4(inPosition, 1.0);

	// Pass through the texture coordinate
	vTexCoord = inCoord;
}
