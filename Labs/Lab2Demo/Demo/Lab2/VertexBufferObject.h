#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include "Common.h"

// This class provides a wrapper around an OpenGL Vertex Buffer Object
class CVertexBufferObject
{
public:
	CVertexBufferObject();

	void Create(int iSize = 0);						// Creates a VBO
	void Bind(int iBufferType = GL_ARRAY_BUFFER);	// Binds the VBO
	void Release();									// Releases the VBO

	void AddData(void* ptrData, unsigned int uiDataSize);	// Add data to the VBO
	void UploadDataToGPU(int iUsageHint);			// Upload the VBO to the GPU

	void *MapBufferToMemory(int iUsageHint);
	void *MapSubBufferToMemory(int iUsageHint, unsigned int uiOffset, unsigned int uiLength);
	void UnmapBuffer();

	void* GetDataPointer();
	GLuint GetBuffer();

private:
	GLuint m_uiBuffer;
	int m_iSize;
	int m_iBufferType;
	vector<unsigned char> m_data;

	bool m_bDataUploaded;
};
