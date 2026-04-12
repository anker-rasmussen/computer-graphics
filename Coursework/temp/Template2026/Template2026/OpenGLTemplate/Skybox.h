#pragma once

#include "Texture.h"
#include "VertexBufferObject.h"
#include "Cubemap.h"

// This is a class for creating and rendering a skybox
class CSkybox
{
public:
	CSkybox();
	~CSkybox();
	void Create(float size);
	void Create(float size, const string& dir, const string& prefix);
	void Render(int textureUnit);
	void Release();

private:
	UINT m_vao;
	CVertexBufferObject m_vbo;
	CCubemap m_cubemapTexture;
	
};