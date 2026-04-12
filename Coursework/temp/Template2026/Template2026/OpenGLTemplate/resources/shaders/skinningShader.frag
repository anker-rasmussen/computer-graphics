#version 400 core

in vec3 vEyeNorm;
in vec3 vEyePos;
in vec2 vTexCoord;
in vec3 worldPosition;
in vec4 vLightSpacePos;
in vec4 vSpotLightSpacePos;

out vec4 vOutputColour;

uniform sampler2D sampler0;
uniform sampler2DShadow shadowMap;
uniform sampler2DShadow spotShadowMap;
uniform bool bUseTexture;

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

float ComputeShadowFrom(sampler2DShadow smap, vec4 lightSpacePos, float bias)
{
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0)
		return 1.0;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(smap, 0);
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias);
			shadow += texture(smap, sampleCoord);
		}
	}
	return shadow / 9.0;
}

vec3 PhongModelSingle(vec3 eyePos, vec3 eyeNorm, LightInfo light, float shadowFactor)
{
	vec3 s = normalize(vec3(light.position) - eyePos);
	vec3 v = normalize(-eyePos);
	vec3 r = reflect(-s, eyeNorm);
	vec3 ambient = light.La * material1.Ma;
	float sDotN = max(dot(s, eyeNorm), 0.0);
	vec3 diffuse = light.Ld * material1.Md * sDotN;
	vec3 specular = vec3(0.0);
	if (sDotN > 0.0)
		specular = light.Ls * material1.Ms * pow(max(dot(r, v), 0.0), material1.shininess + 0.000001);
	return ambient + shadowFactor * (diffuse + specular);
}

void main()
{
	vec3 N = normalize(vEyeNorm);
	float shadow = ComputeShadowFrom(shadowMap, vLightSpacePos, 0.002);
	float spotShadow = ComputeShadowFrom(spotShadowMap, vSpotLightSpacePos, 0.005);

	// Sun light with shadow
	vec3 colour = PhongModelSingle(vEyePos, N, light1, shadow);

	// Console point lights with viewport spotlight shadow
	if (numLights >= 2) colour += spotShadow * PhongModelSingle(vEyePos, N, light2, 1.0);
	if (numLights >= 3) colour += spotShadow * PhongModelSingle(vEyePos, N, light3, 1.0);
	if (numLights >= 4) colour += spotShadow * PhongModelSingle(vEyePos, N, light4, 1.0);
	if (numLights >= 5) colour += spotShadow * PhongModelSingle(vEyePos, N, light5, 1.0);

	vec4 vTexColour = texture(sampler0, vTexCoord);
	if (bUseTexture)
		vOutputColour = vTexColour * vec4(colour, 1.0);
	else
		vOutputColour = vec4(colour, 1.0);
}
