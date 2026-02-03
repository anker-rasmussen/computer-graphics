#include "Common.h"
#include "VertexBufferObject.h"


// Constructor -- initialise member variable m_bDataUploaded to false
CVertexBufferObject::CVertexBufferObject()
{
	m_bDataUploaded = false;
}


// Create a VBO and optionally reserve space for data
void CVertexBufferObject::Create(int iSize)
{
	glGenBuffers(1, &m_uiBuffer);
	m_data.reserve(iSize);
	m_iSize = iSize;
}

// Release the VBO and any associated data
void CVertexBufferObject::Release()
{
	glDeleteBuffers(1, &m_uiBuffer);
	m_bDataUploaded = false;
	m_data.clear();
}


// Maps the buffer to memory and returns a pointer to data.
// iUsageHint - GL_READ_ONLY, GL_WRITE_ONLY...
void* CVertexBufferObject::MapBufferToMemory(int iUsageHint)
{
	if(!m_bDataUploaded)return NULL;
	void* ptrRes = glMapBuffer(m_iBufferType, iUsageHint);
	return ptrRes;
}


// Maps specified part of a buffer to memory and returns a pointer to data.
// uiOffset is the data offset, and uiLength is the length of the data.
void* CVertexBufferObject::MapSubBufferToMemory(int iUsageHint, unsigned int uiOffset, unsigned int uiLength)
{
	if(!m_bDataUploaded)return NULL;
	void* ptrRes = glMapBufferRange(m_iBufferType, uiOffset, uiLength, iUsageHint);
	return ptrRes;
}

// Unmaps a previously mapped buffer
void CVertexBufferObject::UnmapBuffer()
{
	glUnmapBuffer(m_iBufferType);
}

// Binds a VBO.  iBufferType is the buffer type (e.g., GL_ARRAY_BUFFER, ...)
void CVertexBufferObject::Bind(int iBufferType)
{
	m_iBufferType = iBufferType;
	glBindBuffer(m_iBufferType, m_uiBuffer);
}


// Uploads the data to the GPU.  Afterwards, the data can be cleared.
// iUsageHint - GL_STATIC_DRAW, GL_DYNAMIC_DRAW...
void CVertexBufferObject::UploadDataToGPU(int iDrawingHint)
{
	glBufferData(m_iBufferType, m_data.size(), &m_data[0], iDrawingHint);
	m_bDataUploaded = true;
	m_data.clear();
}

// Adds data to the VBO.
void CVertexBufferObject::AddData(void* ptrData, unsigned int uiDataSize)
{
	m_data.insert(m_data.end(), (unsigned char*)ptrData, (unsigned char*)ptrData+uiDataSize);
}

// Gets a pointer to the data.  Note this is only valid before uploading, since the data is cleared on upload.
void* CVertexBufferObject::GetDataPointer()
{
	if (m_bDataUploaded)
		return NULL;
	return (void*) m_data[0];
}

// Returns the ID of the VBO
GLuint CVertexBufferObject::GetBuffer()
{
	return m_uiBuffer;
}
