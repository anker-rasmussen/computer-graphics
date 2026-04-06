#version 400 core

in vec3 vEyeNorm;
in vec3 vEyePos;
in vec2 vTexCoord;
in vec3 worldPosition;
in vec4 vLightSpacePos;
in vec4 vSpotLightSpacePos;

out vec4 vOutputColour;

uniform sampler2D sampler0;
uniform samplerCube CubeMapTex;
uniform sampler2DShadow shadowMap;
uniform sampler2DShadow spotShadowMap;
uniform bool bUseTexture;
uniform bool renderSkybox;
uniform bool bMirror;
uniform float alpha;

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
uniform MaterialInfo material1;

const int MAX_POINT_LIGHTS = 4;
uniform int numPointLights;
uniform LightInfo pointLights[MAX_POINT_LIGHTS];

float ComputeShadowFrom(sampler2DShadow smap, vec4 lightSpacePos, float bias)
{
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0)
		return 1.0;

	// 3x3 PCF
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

vec3 PhongModel(vec3 eyePos, vec3 eyeNorm, float shadowFactor)
{
	vec3 s = normalize(vec3(light1.position) - eyePos);
	vec3 v = normalize(-eyePos);
	vec3 r = reflect(-s, eyeNorm);
	vec3 ambient = light1.La * material1.Ma;
	float sDotN = max(dot(s, eyeNorm), 0.0);
	vec3 diffuse = light1.Ld * material1.Md * sDotN;
	vec3 specular = vec3(0.0);
	if (sDotN > 0.0)
		specular = light1.Ls * material1.Ms * pow(max(dot(r, v), 0.0), material1.shininess + 0.000001);

	return ambient + shadowFactor * (diffuse + specular);
}

vec3 PointLightModel(vec3 eyePos, vec3 eyeNorm, LightInfo light)
{
	vec3 toLight = vec3(light.position) - eyePos;
	float dist = length(toLight);
	vec3 s = normalize(toLight);
	vec3 v = normalize(-eyePos);
	vec3 r = reflect(-s, eyeNorm);
	float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
	vec3 ambient = light.La * material1.Ma * atten;
	float sDotN = max(dot(s, eyeNorm), 0.0);
	vec3 diffuse = light.Ld * material1.Md * sDotN * atten;
	vec3 specular = vec3(0.0);
	if (sDotN > 0.0)
		specular = light.Ls * material1.Ms * pow(max(dot(r, v), 0.0), material1.shininess + 0.000001) * atten;
	return ambient + diffuse + specular;
}

void main()
{
	if (renderSkybox) {
		vOutputColour = texture(CubeMapTex, worldPosition);
		return;
	}

	vec3 N = normalize(vEyeNorm);
	float shadow = ComputeShadowFrom(shadowMap, vLightSpacePos, 0.002);
	float spotShadow = ComputeShadowFrom(spotShadowMap, vSpotLightSpacePos, 0.005);

	vec3 colour = PhongModel(vEyePos, N, shadow);

	// Point lights + viewport spotlight shadow
	for (int i = 0; i < numPointLights; i++)
		colour += spotShadow * PointLightModel(vEyePos, N, pointLights[i]);

	if (bMirror) {
		// Environment-mapped reflection using skybox cubemap
		vec3 viewDir = normalize(vEyePos);
		vec3 reflectDir = reflect(viewDir, N);
		vec4 envColour = texture(CubeMapTex, reflectDir);
		// Slight tint and specular highlight
		vec3 tint = vec3(0.85f, 0.9f, 1.0f);
		vOutputColour = vec4(envColour.rgb * tint + colour * 0.1, 1.0);
		return;
	}

	float a = (alpha > 0.0) ? alpha : 1.0;
	vec4 vTexColour = texture(sampler0, vTexCoord);
	if (bUseTexture)
		vOutputColour = vTexColour * vec4(colour, a);
	else
		vOutputColour = vec4(colour, a);
}
