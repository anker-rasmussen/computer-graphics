#version 400 core

in vec2 vTexCoord;
out vec4 vOutputColour;

uniform sampler2D sampler0;
uniform vec4 vColour;
uniform bool bFullColour;  // true = RGBA texture (portraits), false = single-channel (glyphs)

void main()
{
	vec4 vTexColour = texture(sampler0, vTexCoord);
	if (bFullColour)
		vOutputColour = vTexColour * vColour;
	else
		vOutputColour = vec4(vTexColour.r) * vColour;
}