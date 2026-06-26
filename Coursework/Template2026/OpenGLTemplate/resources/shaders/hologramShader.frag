// Hologram fragment shader -- additive cyan rim glow + scanlines + scrolling wipe.

#version 400 core

in vec3 vEyeNorm;
in vec3 vEyePos;
in vec3 vModelPos;

out vec4 vOutputColour;

uniform float time;
uniform vec3 holoColour;
uniform float wipeAxisScale;  // model-space units per wipe band

void main()
{
	vec3 N = normalize(vEyeNorm);
	vec3 V = normalize(-vEyePos);
	float NdotV = max(dot(N, V), 0.0);

	float fresnel = pow(1.0 - NdotV, 2.5);
	float core = 0.18 + 0.55 * NdotV;

	float scan = 0.78 + 0.22 * sin(gl_FragCoord.y * 1.4 - time * 8.0);

	float wipePos = fract(vModelPos.z * wipeAxisScale - time * 0.35);
	float wipeBand = smoothstep(0.0, 0.04, wipePos) * smoothstep(0.10, 0.04, wipePos);
	float wipe = 1.0 + 1.6 * wipeBand;

	float flicker = 0.93 + 0.07 * sin(time * 47.0 + vModelPos.y * 6.3);

	vec3 colour = holoColour * (core + fresnel * 2.2) * scan * wipe * flicker;

	float alpha = clamp(0.32 + fresnel * 0.85 + wipeBand * 0.5, 0.0, 1.0);

	vOutputColour = vec4(colour, alpha);
}
