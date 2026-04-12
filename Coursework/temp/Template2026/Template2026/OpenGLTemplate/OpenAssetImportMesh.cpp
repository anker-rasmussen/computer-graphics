/*

	Copyright 2011 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <cstdio>
#include <FreeImage.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include "OpenAssetImportMesh.h"

COpenAssetImportMesh::MeshEntry::MeshEntry()
{
    vbo = INVALID_OGL_VALUE;
    ibo = INVALID_OGL_VALUE;
    NumIndices  = 0;
    MaterialIndex = INVALID_MATERIAL;
};

COpenAssetImportMesh::MeshEntry::~MeshEntry()
{
    if (vbo != INVALID_OGL_VALUE)
        glDeleteBuffers(1, &vbo);

    if (ibo != INVALID_OGL_VALUE)
        glDeleteBuffers(1, &ibo);
}

void COpenAssetImportMesh::MeshEntry::Init(const std::vector<Vertex>& Vertices,
                          const std::vector<unsigned int>& Indices)
{
    NumIndices = int(Indices.size());
    if (Vertices.empty() || Indices.empty()) {
        NumIndices = 0;
        return;
    }

	glGenBuffers(1, &vbo);
  	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NumIndices, &Indices[0], GL_STATIC_DRAW);
}

COpenAssetImportMesh::COpenAssetImportMesh()
{
}


COpenAssetImportMesh::~COpenAssetImportMesh()
{
    Clear();
}


void COpenAssetImportMesh::Clear()
{
    for (unsigned int i = 0 ; i < m_Textures.size() ; i++) {
        SAFE_DELETE(m_Textures[i]);
    }
	glDeleteVertexArrays(1, &m_vao);
}


bool COpenAssetImportMesh::Load(const std::string& Filename)
{
    // Release the previously loaded mesh (if it exists)
    Clear();

    bool Ret = false;
    Assimp::Importer Importer;

    const aiScene* pScene = Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_PreTransformVertices);

    if (pScene) {
        Ret = InitFromScene(pScene, Filename);
    }
    else {
        fprintf(stderr, "Error loading mesh model: %s\n", Importer.GetErrorString());
    }

    return Ret;
}

bool COpenAssetImportMesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);


    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh);
    }

    return InitMaterials(pScene, Filename);
}

void COpenAssetImportMesh::InitMesh(unsigned int Index, const aiMesh* paiMesh)
{
    m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;

    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
        const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal   = paiMesh->HasNormals() ? &(paiMesh->mNormals[i]) : &Zero3D;
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Vertex v(glm::vec3(pPos->x, pPos->y, pPos->z),
                 glm::vec2(pTexCoord->x, 1.0f-pTexCoord->y),
                 glm::vec3(pNormal->x, pNormal->y, pNormal->z));

        Vertices.push_back(v);
    }

    for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        if (Face.mNumIndices != 3) continue; // skip non-triangle faces
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries[Index].Init(Vertices, Indices);
}

bool COpenAssetImportMesh::InitMaterials(const aiScene* pScene, const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/\\");
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Textures[i] = NULL;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

			if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                // Check for embedded texture (GLB files)
                const aiTexture* embTex = pScene->GetEmbeddedTexture(Path.C_Str());
                if (embTex && embTex->mHeight == 0) {
                    // Compressed embedded texture — decode via FreeImage
                    FIMEMORY* fiMem = FreeImage_OpenMemory(
                        (BYTE*)embTex->pcData, embTex->mWidth);
                    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(fiMem, 0);
                    FIBITMAP* dib = FreeImage_LoadFromMemory(fif, fiMem, 0);
                    FreeImage_CloseMemory(fiMem);
                    if (dib) {
                        BYTE* pData = FreeImage_GetBits(dib);
                        int w = FreeImage_GetWidth(dib);
                        int h = FreeImage_GetHeight(dib);
                        int bpp = FreeImage_GetBPP(dib);
                        GLenum fmt = (bpp == 32) ? GL_BGRA :
                                     (bpp == 24) ? GL_BGR : GL_LUMINANCE;
                        m_Textures[i] = new CTexture();
                        m_Textures[i]->CreateFromData(pData, w, h, bpp, fmt, true);
                        m_Textures[i]->SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        m_Textures[i]->SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        m_Textures[i]->SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                        m_Textures[i]->SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                        FreeImage_Unload(dib);
                        printf("Loaded embedded texture (%dx%d)\n", w, h);
                    }
                } else {
                    std::string FullPath = Dir + "/" + Path.data;
                    m_Textures[i] = new CTexture();
                    if (!m_Textures[i]->Load(FullPath, true)) {
                        fprintf(stderr, "Error loading mesh texture: %s\n", FullPath.c_str());
                        delete m_Textures[i];
                        m_Textures[i] = NULL;
                        Ret = false;
                    }
                    else {
                        printf("Loaded texture '%s'\n", FullPath.c_str());
                    }
                }
            }
        }

        // Try PBR BaseColor texture by material name convention (e.g. "bow_main" -> "*_bow_main_BaseColor.png")
        if (!m_Textures[i]) {
            aiString matName;
            pMaterial->Get(AI_MATKEY_NAME, matName);
            std::string name(matName.C_Str());

            // Search for a file in the model directory matching *_<matname>_BaseColor.png
            std::string searchSuffix = "_" + name + "_BaseColor.png";
            // List files in Dir (use platform directory listing)
#ifdef _WIN32
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA((Dir + "/*").c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::string fname(fd.cFileName);
                    if (fname.size() >= searchSuffix.size() &&
                        fname.compare(fname.size() - searchSuffix.size(), searchSuffix.size(), searchSuffix) == 0) {
                        std::string fullPath = Dir + "/" + fname;
                        m_Textures[i] = new CTexture();
                        if (m_Textures[i]->Load(fullPath, true)) {
                            printf("Loaded PBR BaseColor: %s\n", fullPath.c_str());
                        } else {
                            delete m_Textures[i];
                            m_Textures[i] = NULL;
                        }
                        break;
                    }
                } while (FindNextFileA(hFind, &fd));
                FindClose(hFind);
            }
#else
            DIR* dir = opendir(Dir.c_str());
            if (dir) {
                struct dirent* ent;
                while ((ent = readdir(dir)) != NULL) {
                    std::string fname(ent->d_name);
                    if (fname.size() >= searchSuffix.size() &&
                        fname.compare(fname.size() - searchSuffix.size(), searchSuffix.size(), searchSuffix) == 0) {
                        std::string fullPath = Dir + "/" + fname;
                        m_Textures[i] = new CTexture();
                        if (m_Textures[i]->Load(fullPath, true)) {
                            printf("Loaded PBR BaseColor: %s\n", fullPath.c_str());
                        } else {
                            delete m_Textures[i];
                            m_Textures[i] = NULL;
                        }
                        break;
                    }
                }
                closedir(dir);
            }
#endif
        }

        // Load a single colour texture matching the diffuse colour if no texture added
        if (!m_Textures[i]) {

			aiColor3D color (0.f,0.f,0.f);
			pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE,color);

			m_Textures[i] = new CTexture();
			BYTE data[3];
			data[0] = (BYTE) (color[2]*255);
			data[1] = (BYTE) (color[1]*255);
			data[2] = (BYTE) (color[0]*255);
			m_Textures[i]->CreateFromData(data, 1, 1, 24, GL_BGR, false);

        }
    }

    return Ret;
}

void COpenAssetImportMesh::Render()
{
	glBindVertexArray(m_vao);

    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].ibo);


        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        if (MaterialIndex < m_Textures.size() && m_Textures[MaterialIndex]) {
            m_Textures[MaterialIndex]->Bind(0);
        }


        glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
    }



}
