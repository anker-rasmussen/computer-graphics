#version 400 core

// Matrices
uniform struct Matrices
{
	mat4 projMatrix;
	mat4 modelViewMatrix;
	mat3 normalMatrix;
} matrices;

// Bone matrices
const int MAX_BONES = 128;
uniform mat4 boneMatrices[MAX_BONES];

// Shadow mapping
uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;

// Vertex attributes
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in ivec4 inBoneIDs;
layout (location = 4) in vec4 inWeights;

out vec3 vEyeNorm;
out vec3 vEyePos;
out vec2 vTexCoord;
out vec3 worldPosition;
out vec4 vLightSpacePos;
out vec4 vSpotLightSpacePos;

void main()
{
	// Compute skinned position and normal
	mat4 boneTransform = mat4(0.0);
	float totalWeight = inWeights[0] + inWeights[1] + inWeights[2] + inWeights[3];

	if (totalWeight > 0.001) {
		for (int i = 0; i < 4; i++) {
			int boneID = inBoneIDs[i];
			if (boneID >= 0 && boneID < MAX_BONES && inWeights[i] > 0.0) {
				boneTransform += boneMatrices[boneID] * inWeights[i];
			}
		}
	} else {
		boneTransform = mat4(1.0);
	}

	vec4 skinnedPos = boneTransform * vec4(inPosition, 1.0);
	vec3 skinnedNormal = mat3(boneTransform) * inNormal;
	worldPosition = skinnedPos.xyz;

	gl_Position = matrices.projMatrix * matrices.modelViewMatrix * skinnedPos;

	vEyeNorm = normalize(matrices.normalMatrix * skinnedNormal);
	vec4 eyePos4 = matrices.modelViewMatrix * skinnedPos;
	vEyePos = eyePos4.xyz;

	vLightSpacePos = lightSpaceMatrix * skinnedPos;
	vSpotLightSpacePos = spotLightSpaceMatrix * skinnedPos;

	vTexCoord = inCoord;
}
