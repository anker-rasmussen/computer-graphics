#version 400 core

in vec2 vTexCoord;
in vec4 vColour;

out vec4 vOutputColour;

uniform sampler2D sampler0;

void main() {
	float a = texture(sampler0, vTexCoord).a;
	vOutputColour = vec4(vColour.rgb, vColour.a * a);
}
