#pragma once

#include "Texture.h"
#include <vector>

// Procedural spaceship inspired by the Perhonen from "The Quantum Thief" by Hannu Rajaniemi.
// Hull (surface of revolution) + 4 nacelles + 4 solar sails + ion thrust cones.
// Split into three render ranges so Game can apply different shaders/blending per part.
class CShip
{
public:
	CShip();
	~CShip();

	void Create(string hullTexDir, string hullTexFile,
	            string sailTexDir, string sailTexFile);

	// Separate render calls — caller switches shaders between these
	void RenderHull();    // hull + nacelles (opaque, neon texture)
	void RenderThrust();  // ion thrust cones (additive blend)
	void RenderSails();   // 4 solar sails (iridescent texture)

	void Release();

private:
	GLuint m_vao;
	GLuint m_vboVertices;
	GLuint m_vboIndices;
	std::vector<BYTE> m_vertexData;
	std::vector<BYTE> m_indexData;

	CTexture m_hullTexture;
	bool m_hasHullTexture;
	CTexture m_sailTexture;
	bool m_hasSailTexture;

	int m_numTriangles;
	int m_hullTriangles;   // hull + nacelles
	int m_thrustTriangles; // ion thrust cones
	// sail triangles = m_numTriangles - m_hullTriangles - m_thrustTriangles
	unsigned int m_vertexCount;

	unsigned int AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal, glm::vec3 colour);
	void AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2);

	float HullProfile(float t) const;

	void CreateHull(float length, float maxRadius, int slices, int stacks);
	void CreateNacelles(float radius, float length, int count=4);
	void CreateIonThrust(int count=4);
	void CreateSolarSails(float sailHeight, float sailWidth, float curvature, int count=4);
	void CreateCylinder(glm::vec3 from, glm::vec3 to, float r0, float r1, int slices, glm::vec3 colour);

	glm::vec3 CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const;
};
