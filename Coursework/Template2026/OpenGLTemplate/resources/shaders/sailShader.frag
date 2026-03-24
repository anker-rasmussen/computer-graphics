// Sail fragment shader -- Blinn-Phong, hyperreflective

#version 400 core

in vec3 vEyeNorm;
in vec3 vEyePos;
in vec2 vTexCoord;
in vec3 vColour;

out vec4 vOutputColour;

uniform sampler2D sampler0;
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
uniform MaterialInfo material1;

void main()
{
	vec3 N = normalize(vEyeNorm);
	vec3 V = normalize(-vEyePos);
	vec3 L = normalize(vec3(light1.position) - vEyePos);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 ambient  = light1.La * material1.Ma;
	vec3 diffuse  = light1.Ld * material1.Md * NdotL;
	vec3 specular = light1.Ls * material1.Ms * pow(NdotH, material1.shininess);

	// Broad bloom for solar glare
	float bloom = pow(NdotH, material1.shininess * 0.2) * 0.4;

	vec3 colour = (ambient + diffuse + specular + vec3(bloom)) * vColour;
	if (bUseTexture) {
		colour *= texture(sampler0, vTexCoord).rgb;
	}
	colour *= 1.5;

	vOutputColour = vec4(colour, 1.0);
}
