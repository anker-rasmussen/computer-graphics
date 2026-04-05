// Sail vertex shader -- spar-driven unfurling with cloth drape physics

#version 400 core

uniform struct Matrices
{
	mat4 projMatrix;
	mat4 modelViewMatrix;
	mat3 normalMatrix;
} matrices;

uniform float unfurl; // 0 = furled, 1 = fully deployed

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inCoord;
layout (location = 3) in vec3 inColour;

out vec3 vEyeNorm;
out vec3 vEyePos;
out vec2 vTexCoord;
out vec3 vColour;

void main()
{
	// UV parameterisation: (0,0) = aft (hull attachment), (1,1) = forward tip
	float s = inCoord.x;
	float t = inCoord.y;

	// How far this vertex is from the aft anchor along the sail
	// Edge vertices (s or t near 0 or 1) are on spars; interior is membrane
	float edgeDist = min(min(s, 1.0 - s), min(t, 1.0 - t));
	bool onSpar = edgeDist < 0.08;

	// Spars extend progressively from root: root-end vertices lead, tips lag
	float sparProgress = max(s, t); // 0 at aft corner, 1 at far tips
	float sparExtend = clamp((unfurl * 1.3 - sparProgress) / 0.3, 0.0, 1.0);

	// Membrane between spars unfurls slower than the spars — cloth needs the
	// frame to be out before it can stretch taut. Interior lags behind edges.
	float membraneDelay = (1.0 - edgeDist * 4.0) * 0.15; // interior lags ~15%
	float membraneExtend = clamp((unfurl * 1.3 - sparProgress - membraneDelay) / 0.35, 0.0, 1.0);

	float localUnfurl = onSpar ? sparExtend : membraneExtend;

	// Furled position: collapsed along hull axis, bunched near the nose
	// Sails fold back along Z and collapse XY toward the axis
	vec3 furledPos = vec3(
		inPosition.x * 0.06,
		inPosition.y * 0.06,
		7.0 + sparProgress * 2.5  // stacked near hull nose, slight spread along Z
	);

	vec3 pos = mix(furledPos, inPosition, localUnfurl);

	// Extra droop on partially-deployed membrane — slack cloth sags under gravity
	// Maximum sag when membrane is half-extended, zero when furled or fully taut
	float sag = sin(localUnfurl * 3.14159) * (1.0 - float(onSpar)) * 0.8;
	pos.y -= sag * sin(3.14159 * s) * sin(3.14159 * t);

	vec4 eyePos4 = matrices.modelViewMatrix * vec4(pos, 1.0f);
	vEyePos = eyePos4.xyz;
	vEyeNorm = normalize(matrices.normalMatrix * inNormal);
	vTexCoord = inCoord;
	vColour = inColour;
	gl_Position = matrices.projMatrix * eyePos4;
}
