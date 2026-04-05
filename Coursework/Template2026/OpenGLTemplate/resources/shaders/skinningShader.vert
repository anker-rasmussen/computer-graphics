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

// Light and material (same as mainShader)
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

uniform LightInfo light1;
uniform LightInfo light2;
uniform LightInfo light3;
uniform LightInfo light4;
uniform LightInfo light5;
uniform int numLights;
uniform MaterialInfo material1;

// Vertex attributes
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in ivec4 inBoneIDs;
layout (location = 4) in vec4 inWeights;

out vec3 vColour;
out vec2 vTexCoord;
out vec3 worldPosition;

vec3 PhongModelSingle(vec4 eyePosition, vec3 eyeNorm, LightInfo light)
{
	vec3 s = normalize(vec3(light.position - eyePosition));
	vec3 v = normalize(-eyePosition.xyz);
	vec3 r = reflect(-s, eyeNorm);
	vec3 n = eyeNorm;
	vec3 ambient = light.La * material1.Ma;
	float sDotN = max(dot(s, n), 0.0f);
	vec3 diffuse = light.Ld * material1.Md * sDotN;
	vec3 specular = vec3(0.0f);
	float eps = 0.000001f;
	if (sDotN > 0.0f)
		specular = light.Ls * material1.Ms * pow(max(dot(r, v), 0.0f), material1.shininess + eps);
	return ambient + diffuse + specular;
}

vec3 PhongModel(vec4 eyePosition, vec3 eyeNorm)
{
	vec3 colour = PhongModelSingle(eyePosition, eyeNorm, light1);
	if (numLights >= 2) colour += PhongModelSingle(eyePosition, eyeNorm, light2);
	if (numLights >= 3) colour += PhongModelSingle(eyePosition, eyeNorm, light3);
	if (numLights >= 4) colour += PhongModelSingle(eyePosition, eyeNorm, light4);
	if (numLights >= 5) colour += PhongModelSingle(eyePosition, eyeNorm, light5);
	return colour;
}

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

	vec3 vEyeNorm = normalize(matrices.normalMatrix * skinnedNormal);
	vec4 vEyePosition = matrices.modelViewMatrix * skinnedPos;

	vColour = PhongModel(vEyePosition, vEyeNorm);
	vTexCoord = inCoord;
}
