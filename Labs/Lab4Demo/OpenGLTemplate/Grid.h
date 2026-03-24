#pragma once

#include "VertexBufferObject.h"

// Class for rendering a grid-like terrain
class CGrid
{
public:
	CGrid();
	~CGrid();
	void Create(float fWidth, float fHeight, int iLines);
	void Render();
	void Release();
private:
	UINT m_vao;
	CVertexBufferObject m_vbo;
	glm::vec4 m_vColour;	
	int m_iVertices;
};