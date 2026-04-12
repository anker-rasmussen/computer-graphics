#include "Common.h"
#include "Asteroid.h"

#include <glm/gtc/noise.hpp>
#include <map>
#include <cmath>

CAsteroid::CAsteroid()
	: m_vao(0), m_vboVertices(0), m_vboIndices(0), m_numTriangles(0), m_baseRadius(1.0f)
{}

CAsteroid::~CAsteroid()
{}

// -----------------------------------------------------------------------
// Icosahedron base mesh
// -----------------------------------------------------------------------
void CAsteroid::MakeIcosahedron()
{
	float t = (1.0f + sqrtf(5.0f)) / 2.0f;

	m_icoVerts.clear();
	auto addV = [&](float x, float y, float z) {
		glm::vec3 p = glm::normalize(glm::vec3(x, y, z));
		m_icoVerts.push_back({p});
	};

	addV(-1,  t,  0); addV( 1,  t,  0); addV(-1, -t,  0); addV( 1, -t,  0);
	addV( 0, -1,  t); addV( 0,  1,  t); addV( 0, -1, -t); addV( 0,  1, -t);
	addV( t,  0, -1); addV( t,  0,  1); addV(-t,  0, -1); addV(-t,  0,  1);

	m_icoIndices = {
		0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
		1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
		3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
		4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1
	};
}

unsigned int CAsteroid::GetMidpoint(unsigned int i0, unsigned int i1,
                                    std::map<uint64_t, unsigned int> &cache)
{
	uint64_t key = (uint64_t)glm::min(i0, i1) << 32 | (uint64_t)glm::max(i0, i1);
	auto it = cache.find(key);
	if (it != cache.end()) return it->second;

	glm::vec3 mid = glm::normalize(
		(m_icoVerts[i0].pos + m_icoVerts[i1].pos) * 0.5f);
	unsigned int idx = (unsigned int)m_icoVerts.size();
	m_icoVerts.push_back({mid});
	cache[key] = idx;
	return idx;
}

void CAsteroid::Subdivide(int levels)
{
	for (int level = 0; level < levels; level++) {
		std::map<uint64_t, unsigned int> midCache;
		std::vector<unsigned int> newIndices;
		int numTris = (int)m_icoIndices.size() / 3;

		for (int i = 0; i < numTris; i++) {
			unsigned int v0 = m_icoIndices[i * 3 + 0];
			unsigned int v1 = m_icoIndices[i * 3 + 1];
			unsigned int v2 = m_icoIndices[i * 3 + 2];

			unsigned int a = GetMidpoint(v0, v1, midCache);
			unsigned int b = GetMidpoint(v1, v2, midCache);
			unsigned int c = GetMidpoint(v2, v0, midCache);

			newIndices.insert(newIndices.end(), {v0, a, c});
			newIndices.insert(newIndices.end(), {v1, b, a});
			newIndices.insert(newIndices.end(), {v2, c, b});
			newIndices.insert(newIndices.end(), {a, b, c});
		}

		m_icoIndices = newIndices;
	}
}

// -----------------------------------------------------------------------
// Fractal Brownian Motion
// -----------------------------------------------------------------------
float CAsteroid::Fbm(glm::vec3 p, int octaves, float lacunarity, float gain) const
{
	float sum = 0.0f;
	float amp = 1.0f;
	float freq = 1.0f;
	for (int i = 0; i < octaves; i++) {
		sum += amp * glm::perlin(p * freq);
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

// -----------------------------------------------------------------------
// Create — build mesh and upload to GPU
// -----------------------------------------------------------------------
void CAsteroid::Create(float baseRadius, float noiseAmplitude, float noiseFrequency,
                       int subdivisions, unsigned int seed)
{
	m_baseRadius = baseRadius;

	// Build icosphere
	MakeIcosahedron();
	Subdivide(subdivisions);

	// Displace vertices with Perlin noise
	glm::vec3 seedOffset(seed * 13.37f, seed * 7.13f, seed * 3.71f);

	for (auto &v : m_icoVerts) {
		float noise = Fbm(v.pos * noiseFrequency + seedOffset, 3, 2.0f, 0.5f);
		float r = baseRadius * (1.0f + noiseAmplitude * noise);
		v.pos = glm::normalize(v.pos) * r;
	}

	// Compute face normals and accumulate per-vertex normals
	int numVerts = (int)m_icoVerts.size();
	std::vector<glm::vec3> normals(numVerts, glm::vec3(0));

	m_numTriangles = (int)m_icoIndices.size() / 3;
	for (int i = 0; i < m_numTriangles; i++) {
		unsigned int i0 = m_icoIndices[i * 3 + 0];
		unsigned int i1 = m_icoIndices[i * 3 + 1];
		unsigned int i2 = m_icoIndices[i * 3 + 2];

		glm::vec3 e1 = m_icoVerts[i1].pos - m_icoVerts[i0].pos;
		glm::vec3 e2 = m_icoVerts[i2].pos - m_icoVerts[i0].pos;
		glm::vec3 fn = glm::cross(e1, e2);

		normals[i0] += fn;
		normals[i1] += fn;
		normals[i2] += fn;
	}

	for (auto &n : normals) {
		float len = glm::length(n);
		if (len > 0.0001f) n /= len;
	}

	// Build interleaved vertex data: pos(vec3) + normal(vec3) + uv(vec2) + colour(vec3)
	// Matches the main shader's expected vertex layout (44 bytes/vert like Ship)
	// But we only need pos + normal + uv for the main shader:
	// Layout: pos(vec3) | uv(vec2) | normal(vec3) = 32 bytes (matching CatmullRom/track layout)
	std::vector<float> vertData;
	vertData.reserve(numVerts * 8); // 3+2+3

	for (int i = 0; i < numVerts; i++) {
		// position
		vertData.push_back(m_icoVerts[i].pos.x);
		vertData.push_back(m_icoVerts[i].pos.y);
		vertData.push_back(m_icoVerts[i].pos.z);
		// uv (spherical mapping)
		glm::vec3 d = glm::normalize(m_icoVerts[i].pos);
		float u = 0.5f + atan2f(d.z, d.x) / (2.0f * (float)M_PI);
		float v = 0.5f - asinf(glm::clamp(d.y, -1.0f, 1.0f)) / (float)M_PI;
		vertData.push_back(u);
		vertData.push_back(v);
		// normal
		vertData.push_back(normals[i].x);
		vertData.push_back(normals[i].y);
		vertData.push_back(normals[i].z);
	}

	// Upload to GPU
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboVertices);
	glBufferData(GL_ARRAY_BUFFER, vertData.size() * sizeof(float), vertData.data(), GL_STATIC_DRAW);

	GLsizei stride = (3 + 2 + 3) * sizeof(float); // 32 bytes

	// pos
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	// uv
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	// normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

	glGenBuffers(1, &m_vboIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             m_icoIndices.size() * sizeof(unsigned int),
	             m_icoIndices.data(), GL_STATIC_DRAW);
}

void CAsteroid::Render()
{
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_numTriangles * 3, GL_UNSIGNED_INT, 0);
}

void CAsteroid::Release()
{
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vboVertices);
	glDeleteBuffers(1, &m_vboIndices);
}
