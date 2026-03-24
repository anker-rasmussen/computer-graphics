#pragma once

#include "Texture.h"
#include <vector>

// Solar sail shape
class CShip
{
public:
	CShip();
	~CShip();

	void Create(string directory, string filename);
	void Render();
	void Release();

private:
	GLuint m_vao;
	GLuint m_vboVertices;
	GLuint m_vboIndices;
	std::vector<BYTE> m_vertexData;
	std::vector<BYTE> m_indexData;

	CTexture m_texture;
	bool m_hasTexture;
	int m_numTriangles;
	unsigned int m_vertexCount;

	unsigned int AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal, glm::vec3 colour);
	void AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2);

	void CreateSolarSail(float sailHeight, float sailWidth, float curvature);
	glm::vec3 CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const;
};
