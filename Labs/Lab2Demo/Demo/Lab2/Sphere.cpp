#include "Common.h"

#define _USE_MATH_DEFINES

#include "Sphere.h"
#include <math.h>

// Create a unit sphere 
void CSphere::Create(string a_sDirectory, string a_sFilename, int slicesIn, int stacksIn)
{
	// check if filename passed in -- if so, load texture

	m_tTexture.Load(a_sDirectory+a_sFilename);

	m_sDirectory = a_sDirectory;
	m_sFilename = a_sFilename;
	m_tTexture.SetSamplerParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	m_tTexture.SetSamplerParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_tTexture.SetSamplerParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
	m_tTexture.SetSamplerParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glGenVertexArrays(1, &m_uiVAO);
	glBindVertexArray(m_uiVAO);

	m_vboData.Create();
	m_vboData.Bind();

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> textureCoords;
	std::vector<glm::vec3> normals;

	for (int stacks = 0; stacks < stacksIn; stacks++) {
		float phi = (stacks / (float) stacksIn) * (float) M_PI;
		float nextPhi;
		if (phi != stacksIn - 1)
			nextPhi = ((stacks + 1) / (float) stacksIn) * (float) M_PI;
		else
			nextPhi = (float) M_PI;

		for (int slices = 0; slices < slicesIn; slices++) {
			float theta = (slices / (float) slicesIn) * 2 * (float) M_PI;
			float nextTheta = (((slices + 1) % slicesIn) / (float) slicesIn) * 2 * (float) M_PI;
			
			glm::vec3 v1 = glm::vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
			glm::vec3 v2 = glm::vec3(cos(nextTheta) * sin(phi), sin(nextTheta) * sin(phi), cos(phi));
			glm::vec3 v3 = glm::vec3(cos(theta) * sin(nextPhi), sin(theta) * sin(nextPhi), cos(nextPhi));
			glm::vec3 v4 = glm::vec3(cos(nextTheta) * sin(nextPhi), sin(nextTheta) * sin(nextPhi), cos(nextPhi));
			
			glm::vec2 t1 = glm::vec2(slices / (float) slicesIn, stacks / (float) stacksIn);
			glm::vec2 t2 = glm::vec2((slices + 1) / (float) slicesIn, stacks / (float) stacksIn);
			glm::vec2 t3 = glm::vec2(slices / (float) slicesIn, (stacks + 1) / (float) stacksIn);
			glm::vec2 t4 = glm::vec2((slices + 1) / (float) slicesIn, (stacks + 1) / (float) stacksIn);

			vertices.push_back(v1);
			vertices.push_back(v4);
			vertices.push_back(v2);

			vertices.push_back(v1);
			vertices.push_back(v3);
			vertices.push_back(v4);

			normals.push_back(v1);
			normals.push_back(v4);
			normals.push_back(v2);

			normals.push_back(v1);
			normals.push_back(v3);
			normals.push_back(v4);

			textureCoords.push_back(t1);
			textureCoords.push_back(t4);
			textureCoords.push_back(t2);

			textureCoords.push_back(t1);
			textureCoords.push_back(t3);
			textureCoords.push_back(t4);
		}
	}


	for (int i = 0; i < (int) vertices.size(); i++)
	{
		m_vboData.AddData(&vertices[i], sizeof(glm::vec3));
		m_vboData.AddData(&textureCoords[i], sizeof(glm::vec2));
		m_vboData.AddData(&normals[i], sizeof(glm::vec3));
	}
	m_iNumTriangles = vertices.size() /  3;
	m_vboData.UploadDataToGPU(GL_STATIC_DRAW);

	GLsizei stride = 2*sizeof(glm::vec3)+sizeof(glm::vec2);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	// Texture coordinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
	// Normal vectors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)+sizeof(glm::vec2)));
	
}

// Render the sphere as a set of triangles
void CSphere::Render()
{
	glBindVertexArray(m_uiVAO);


	m_tTexture.Bind();
	glDrawArrays(GL_TRIANGLES, 0, m_iNumTriangles*3);

}

// Release memory on the GPU 
void CSphere::Release()
{
	m_tTexture.Release();
	glDeleteVertexArrays(1, &m_uiVAO);
	m_vboData.Release();
}