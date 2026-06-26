#version 400 core

in vec2 vTexCoord;
out vec4 vOutputColour;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform int mode; // 0 = brightness extract, 1 = blur, 2 = composite
uniform bool horizontal; // blur direction
uniform float bloomThreshold;
uniform float bloomIntensity;

void main()
{
	if (mode == 0) {
		// Brightness extraction — keep only pixels above threshold
		vec3 colour = texture(sceneTexture, vTexCoord).rgb;
		float brightness = dot(colour, vec3(0.2126, 0.7152, 0.0722));
		if (brightness > bloomThreshold)
			vOutputColour = vec4(colour, 1.0);
		else
			vOutputColour = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else if (mode == 1) {
		// 9-tap Gaussian blur
		vec2 texelSize = 1.0 / textureSize(sceneTexture, 0);
		float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

		vec3 result = texture(sceneTexture, vTexCoord).rgb * weights[0];
		vec2 dir = horizontal ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

		for (int i = 1; i < 5; i++) {
			vec2 offset = dir * texelSize * float(i);
			result += texture(sceneTexture, vTexCoord + offset).rgb * weights[i];
			result += texture(sceneTexture, vTexCoord - offset).rgb * weights[i];
		}
		vOutputColour = vec4(result, 1.0);
	}
	else if (mode == 2) {
		// Composite: scene + bloom
		vec3 scene = texture(sceneTexture, vTexCoord).rgb;
		vec3 bloom = texture(bloomTexture, vTexCoord).rgb;
		vOutputColour = vec4(scene + bloom * bloomIntensity, 1.0);
	}
}
