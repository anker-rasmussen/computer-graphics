#pragma once

#include "Common.h"
#include <vector>
#include <map>

// Procedural asteroid mesh generated from a Perlin-noise-displaced icosphere.
// Each instance gets a unique seed for varied shapes.
class CAsteroid
{
public:
	CAsteroid();
	~CAsteroid();

	// Generate a mesh with given base radius, noise amplitude, and random seed.
	// subdivisions: 1-3 for icosphere detail level
	void Create(float baseRadius, float noiseAmplitude, float noiseFrequency,
	            int subdivisions, unsigned int seed);

	void Render();
	void Release();

	float GetRadius() const { return m_baseRadius; }

private:
	GLuint m_vao;
	GLuint m_vboVertices;
	GLuint m_vboIndices;
	int m_numTriangles;
	float m_baseRadius;

	// Icosphere generation
	struct IcoVertex { glm::vec3 pos; };
	std::vector<IcoVertex> m_icoVerts;
	std::vector<unsigned int> m_icoIndices;

	void MakeIcosahedron();
	void Subdivide(int levels);
	unsigned int GetMidpoint(unsigned int i0, unsigned int i1,
	                         std::map<uint64_t, unsigned int> &cache);

	// Fractal Brownian motion using glm::perlin
	float Fbm(glm::vec3 p, int octaves, float lacunarity, float gain) const;
};
