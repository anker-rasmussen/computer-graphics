#version 400 core

in vec3 vColour;
in vec2 vTexCoord;
in vec3 worldPosition;

out vec4 vOutputColour;

uniform sampler2D sampler0;
uniform bool bUseTexture;

void main()
{
	vec4 vTexColour = texture(sampler0, vTexCoord);

	if (bUseTexture)
		vOutputColour = vTexColour * vec4(vColour, 1.0f);
	else
		vOutputColour = vec4(vColour, 1.0f);
}
