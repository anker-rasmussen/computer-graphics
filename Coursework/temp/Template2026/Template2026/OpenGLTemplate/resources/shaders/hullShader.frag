// Hull fragment shader -- neon circuit glow with charge-driven emissive blue lines

#version 400 core

in vec3 vEyeNorm;
in vec3 vEyePos;
in vec2 vTexCoord;
in vec3 vColour;

out vec4 vOutputColour;

uniform sampler2D sampler0;
uniform bool bUseTexture;
uniform float charge; // 0.0 = dormant, 1.0 = fully charged

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

	vec3 baseColour = (ambient + diffuse + specular) * vColour;

	if (bUseTexture) {
		vec4 texel = texture(sampler0, vTexCoord * 4.0);
		baseColour *= texel.rgb;

		// Detect dark navy circuit traces on light blue background
		// Traces are dark (low luminance), background is bright blue
		float luminance = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
		float lineMask = 1.0 - smoothstep(0.15, 0.45, luminance);

		// Emissive glow proportional to charge
		float baseGlow = 0.3 + 0.7 * charge;
		vec3 glowColour = mix(vec3(0.1, 0.3, 0.6), vec3(0.2, 0.7, 1.0), charge);

		// Dark traces light up with emissive blue when charged
		baseColour += lineMask * baseGlow * glowColour * 2.5;

		// Bloom on the traces when charged
		float bloom = lineMask * charge * 0.4;
		baseColour += vec3(bloom * 0.1, bloom * 0.4, bloom * 0.8);
	}

	vOutputColour = vec4(baseColour, 1.0);
}
