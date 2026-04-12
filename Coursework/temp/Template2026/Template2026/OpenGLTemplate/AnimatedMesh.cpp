#include <assert.h>
#include <cstdio>
#ifndef _WIN32
#include <dirent.h>
#endif
#include "AnimatedMesh.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <FreeImage.h>

static glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}

CAnimatedMesh::CAnimatedMesh()
    : m_vao(0), m_vbo(0), m_ibo(0), m_numBones(0), m_boneSSBO(0),
      m_pScene(nullptr), m_currentTime(0.0f), m_looping(true), m_inPlace(false), m_collapsePivots(false)
{
    for (int i = 0; i < MAX_BONES; i++)
        m_boneMatrices[i] = glm::mat4(1.0f);
}

CAnimatedMesh::~CAnimatedMesh()
{
    for (unsigned int i = 0; i < m_textures.size(); i++)
        if (m_textures[i]) delete m_textures[i];

    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
}

bool CAnimatedMesh::Load(const std::string& filename, bool collapsePivots)
{
    m_collapsePivots = collapsePivots;
    if (collapsePivots)
        m_importer.SetPropertyBool("IMPORT_FBX_PRESERVE_PIVOTS", false);
    m_pScene = m_importer.ReadFile(filename.c_str(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs | aiProcess_LimitBoneWeights);

    if (!m_pScene || !m_pScene->mRootNode) {
        fprintf(stderr, "Error loading animated mesh: %s\n", m_importer.GetErrorString());
        return false;
    }

    m_globalInverseTransform = glm::inverse(AiToGlm(m_pScene->mRootNode->mTransformation));

    // Debug: print node hierarchy
    std::function<void(const aiNode*, int)> printNodes = [&](const aiNode* node, int depth) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("Node: '%s' (%d children)\n", node->mName.C_Str(), node->mNumChildren);
        for (unsigned int i = 0; i < node->mNumChildren && depth < 3; i++)
            printNodes(node->mChildren[i], depth + 1);
    };
    printf("=== Node hierarchy for %s ===\n", filename.c_str());
    printNodes(m_pScene->mRootNode, 0);

    return InitFromScene(m_pScene, filename);
}

bool CAnimatedMesh::InitFromScene(const aiScene* pScene, const std::string& filename)
{
    printf("AnimatedMesh: %d meshes, %d materials, %d animations\n",
           pScene->mNumMeshes, pScene->mNumMaterials, pScene->mNumAnimations);
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        printf("  mesh[%d]: %d verts, %d faces, %d bones\n", i,
               pScene->mMeshes[i]->mNumVertices, pScene->mMeshes[i]->mNumFaces,
               pScene->mMeshes[i]->mNumBones);
    }
    m_entries.resize(pScene->mNumMeshes);
    m_textures.resize(pScene->mNumMaterials);

    std::vector<AnimatedVertex> allVertices;
    std::vector<unsigned int> allIndices;

    unsigned int vertexOffset = 0;
    unsigned int indexOffset = 0;

    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];

        m_entries[i].materialIndex = paiMesh->mMaterialIndex;
        m_entries[i].baseVertex = vertexOffset;

        unsigned int numVerticesBefore = (unsigned int)allVertices.size();
        unsigned int numIndicesBefore = (unsigned int)allIndices.size();

        InitMesh(i, paiMesh, allVertices, allIndices);

        m_entries[i].numIndices = (unsigned int)allIndices.size() - numIndicesBefore;
        m_entries[i].indexOffset = numIndicesBefore;

        vertexOffset = (unsigned int)allVertices.size();
        indexOffset = (unsigned int)allIndices.size();
    }

    // Create single VAO/VBO/IBO for all meshes
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(AnimatedVertex) * allVertices.size(),
                 allVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * allIndices.size(),
                 allIndices.data(), GL_STATIC_DRAW);

    // Position: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
                          (void*)offsetof(AnimatedVertex, m_pos));

    // TexCoord: location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
                          (void*)offsetof(AnimatedVertex, m_tex));

    // Normal: location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
                          (void*)offsetof(AnimatedVertex, m_normal));

    // Bone IDs: location 3
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(AnimatedVertex),
                           (void*)offsetof(AnimatedVertex, m_boneIDs));

    // Bone Weights: location 4
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex),
                          (void*)offsetof(AnimatedVertex, m_weights));

    glBindVertexArray(0);

    // Create SSBO for bone matrices
    glGenBuffers(1, &m_boneSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_BONES * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Load any embedded animations from base file
    if (pScene->mNumAnimations > 0) {
        ExtractAnimation(pScene, "default");
        m_currentAnimation = "default";
    }

    InitMaterials(pScene, filename);
    return true;  // texture failures are non-fatal
}

void CAnimatedMesh::InitMesh(unsigned int index, const aiMesh* paiMesh,
                              std::vector<AnimatedVertex>& vertices,
                              std::vector<unsigned int>& indices)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    unsigned int baseVertex = (unsigned int)vertices.size();

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        AnimatedVertex v;
        const aiVector3D* pPos = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0)
            ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        v.m_pos = glm::vec3(pPos->x, pPos->y, pPos->z);
        v.m_tex = glm::vec2(pTexCoord->x, 1.0f - pTexCoord->y);
        v.m_normal = glm::vec3(pNormal->x, pNormal->y, pNormal->z);

        vertices.push_back(v);
    }

    LoadBones(index, paiMesh, vertices);

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& face = paiMesh->mFaces[i];
        assert(face.mNumIndices == 3);
        indices.push_back(baseVertex + face.mIndices[0]);
        indices.push_back(baseVertex + face.mIndices[1]);
        indices.push_back(baseVertex + face.mIndices[2]);
    }
}

void CAnimatedMesh::LoadBones(unsigned int meshIndex, const aiMesh* paiMesh,
                               std::vector<AnimatedVertex>& vertices)
{
    unsigned int baseVertex = m_entries[meshIndex].baseVertex;

    for (unsigned int i = 0; i < paiMesh->mNumBones; i++) {
        const aiBone* pBone = paiMesh->mBones[i];
        std::string boneName(pBone->mName.data);

        unsigned int boneIndex;
        if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
            boneIndex = m_numBones++;
            BoneInfo bi;
            bi.offsetMatrix = AiToGlm(pBone->mOffsetMatrix);
            bi.finalTransform = glm::mat4(1.0f);
            m_boneInfo.push_back(bi);
            m_boneMapping[boneName] = boneIndex;
        } else {
            boneIndex = m_boneMapping[boneName];
        }

        for (unsigned int j = 0; j < pBone->mNumWeights; j++) {
            unsigned int vertexID = baseVertex + pBone->mWeights[j].mVertexId;
            float weight = pBone->mWeights[j].mWeight;
            vertices[vertexID].AddBone(boneIndex, weight);
        }
    }
}

bool CAnimatedMesh::InitMaterials(const aiScene* pScene, const std::string& filename)
{
    std::string::size_type slashIndex = filename.find_last_of("/\\");
    std::string dir;
    if (slashIndex == std::string::npos) dir = ".";
    else if (slashIndex == 0) dir = "/";
    else dir = filename.substr(0, slashIndex);

    bool ret = true;

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];
        m_textures[i] = nullptr;

        aiString matName;
        pMaterial->Get(AI_MATKEY_NAME, matName);
        printf("  material[%d] '%s': diffuse=%d\n", i, matName.C_Str(),
               pMaterial->GetTextureCount(aiTextureType_DIFFUSE));

        // Try DIFFUSE texture type
        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString path;
            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
                // Check for embedded texture (path starts with '*')
                if (path.data[0] == '*' && pScene->mNumTextures > 0) {
                    int texIndex = atoi(path.C_Str() + 1);
                    if (texIndex >= 0 && (unsigned int)texIndex < pScene->mNumTextures) {
                        const aiTexture* embeddedTex = pScene->mTextures[texIndex];
                        if (embeddedTex && embeddedTex->mHeight == 0) {
                            FIMEMORY* fiMem = FreeImage_OpenMemory(
                                (BYTE*)embeddedTex->pcData, embeddedTex->mWidth);
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
                                m_textures[i] = new CTexture();
                                m_textures[i]->CreateFromData(pData, w, h, bpp, fmt, true);
                                FreeImage_Unload(dib);
                                printf("Loaded embedded texture (%dx%d)\n", w, h);
                            }
                        }
                    }
                } else {
                    std::string fullPath = dir + "/" + path.data;
                    m_textures[i] = new CTexture();
                    if (!m_textures[i]->Load(fullPath, true)) {
                        delete m_textures[i];
                        m_textures[i] = nullptr;
                        // Fallback: try embedded textures by scanning for matching filename
                        std::string pathStr(path.C_Str());
                        std::string baseName = pathStr.substr(pathStr.find_last_of("/\\") + 1);
                        for (unsigned int t = 0; t < pScene->mNumTextures && !m_textures[i]; t++) {
                            const aiTexture* embTex = pScene->mTextures[t];
                            if (embTex && embTex->mHeight == 0) {
                                if (t == i) {
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
                                        m_textures[i] = new CTexture();
                                        m_textures[i]->CreateFromData(pData, w, h, bpp, fmt, true);
                                        FreeImage_Unload(dib);
                                        printf("Loaded embedded texture for material %d (%dx%d)\n", i, w, h);
                                    }
                                }
                            }
                        }
                        if (!m_textures[i]) {
                            fprintf(stderr, "Error loading mesh texture: %s\n", fullPath.c_str());
                            ret = false;
                        }
                    } else {
                        printf("Loaded texture '%s'\n", fullPath.c_str());
                    }
                }
            }
        }

        // Try PBR BaseColor by material name (same as OpenAssetImportMesh)
        if (!m_textures[i]) {
            aiString matNameStr;
            pMaterial->Get(AI_MATKEY_NAME, matNameStr);
            std::string mname(matNameStr.C_Str());
            std::string searchSuffix = "_" + mname + "_BaseColor.png";

            DIR* dirp = opendir(dir.c_str());
            if (dirp) {
                struct dirent* ent;
                while ((ent = readdir(dirp)) != NULL) {
                    std::string fname(ent->d_name);
                    if (fname.size() >= searchSuffix.size() &&
                        fname.compare(fname.size() - searchSuffix.size(), searchSuffix.size(), searchSuffix) == 0) {
                        std::string fullPath = dir + "/" + fname;
                        m_textures[i] = new CTexture();
                        if (m_textures[i]->Load(fullPath, true)) {
                            printf("Loaded PBR BaseColor: %s\n", fullPath.c_str());
                        } else {
                            delete m_textures[i];
                            m_textures[i] = nullptr;
                        }
                        break;
                    }
                }
                closedir(dirp);
            }
        }

        // Try matching material name to texture/<name>_diff*.jpg in a texture/ subdirectory
        if (!m_textures[i]) {
            aiString matNameStr;
            pMaterial->Get(AI_MATKEY_NAME, matNameStr);
            std::string mname(matNameStr.C_Str());
            std::string texDir = dir + "/texture";
            DIR* dirp = opendir(texDir.c_str());
            if (dirp) {
                struct dirent* ent;
                while ((ent = readdir(dirp)) != NULL) {
                    std::string fname(ent->d_name);
                    // Match <materialname>_diff (e.g. body_diff_01.jpg for material "body")
                    if (fname.find(mname + "_diff") == 0) {
                        std::string fullPath = texDir + "/" + fname;
                        m_textures[i] = new CTexture();
                        if (m_textures[i]->Load(fullPath, true)) {
                            printf("Loaded texture by material name: %s\n", fullPath.c_str());
                        } else {
                            delete m_textures[i];
                            m_textures[i] = nullptr;
                        }
                        break;
                    }
                }
                closedir(dirp);
            }
        }

        if (!m_textures[i]) {
            aiColor3D color(0.8f, 0.8f, 0.8f);
            pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            // Blonde color for hair materials
            std::string mname2(matName.C_Str());
            if (mname2.find("hair") != std::string::npos) {
                color = aiColor3D(0.85f, 0.75f, 0.45f);
            }
            m_textures[i] = new CTexture();
            BYTE data[3];
            data[0] = (BYTE)(color[2] * 255);
            data[1] = (BYTE)(color[1] * 255);
            data[2] = (BYTE)(color[0] * 255);
            m_textures[i]->CreateFromData(data, 1, 1, 24, GL_BGR, false);
        }
    }

    return ret;
}

void CAnimatedMesh::SetTexture(const std::string& texturePath)
{
    CTexture* tex = new CTexture();
    if (tex->Load(texturePath, true)) {
        tex->SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        tex->SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Clear existing — just null them out, don't delete (may be shared/embedded)
        // This leaks the old textures but avoids double-free crashes
        m_textures.clear();

        // Fill all material slots
        unsigned int numSlots = 1;
        for (unsigned int i = 0; i < m_entries.size(); i++) {
            if (m_entries[i].materialIndex + 1 > numSlots)
                numSlots = m_entries[i].materialIndex + 1;
        }
        m_textures.resize(numSlots, tex);

        printf("Set texture '%s' for animated mesh (%d slots)\n", texturePath.c_str(), numSlots);
    } else {
        fprintf(stderr, "Failed to load texture '%s'\n", texturePath.c_str());
        delete tex;
    }
}

// --- Animation loading ---

bool CAnimatedMesh::LoadAnimation(const std::string& filename, const std::string& name)
{
    Assimp::Importer importer;
    if (m_collapsePivots)
        importer.SetPropertyBool("IMPORT_FBX_PRESERVE_PIVOTS", false);
    const aiScene* pScene = importer.ReadFile(filename.c_str(),
        aiProcess_Triangulate | aiProcess_LimitBoneWeights);

    if (!pScene || pScene->mNumAnimations == 0) {
        fprintf(stderr, "Error loading animation '%s': %s\n",
                filename.c_str(), importer.GetErrorString());
        return false;
    }

    ExtractAnimation(pScene, name);
    // Debug: print first few channel names
    const aiAnimation* pAnim = pScene->mAnimations[0];
    printf("Loaded animation '%s' from %s (%d channels):", name.c_str(), filename.c_str(), pAnim->mNumChannels);
    for (unsigned int i = 0; i < pAnim->mNumChannels && i < 5; i++)
        printf(" '%s'", pAnim->mChannels[i]->mNodeName.C_Str());
    printf("...\n");
    // Check how many channels match bones
    int matched = 0;
    for (unsigned int i = 0; i < pAnim->mNumChannels; i++) {
        if (m_boneMapping.find(pAnim->mChannels[i]->mNodeName.C_Str()) != m_boneMapping.end())
            matched++;
    }
    printf("  Channel-bone matches: %d/%d\n", matched, pAnim->mNumChannels);
    return true;
}

void CAnimatedMesh::ExtractAnimation(const aiScene* pScene, const std::string& name)
{
    const aiAnimation* pAnim = pScene->mAnimations[0];
    Animation anim;
    anim.duration = pAnim->mDuration;
    anim.ticksPerSecond = pAnim->mTicksPerSecond != 0 ? pAnim->mTicksPerSecond : 25.0;

    for (unsigned int i = 0; i < pAnim->mNumChannels; i++) {
        const aiNodeAnim* pChannel = pAnim->mChannels[i];
        std::string nodeName(pChannel->mNodeName.data);
        AnimChannel ch;

        for (unsigned int j = 0; j < pChannel->mNumPositionKeys; j++) {
            const auto& k = pChannel->mPositionKeys[j];
            ch.positionKeys.push_back({k.mTime,
                glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z)});
        }

        for (unsigned int j = 0; j < pChannel->mNumRotationKeys; j++) {
            const auto& k = pChannel->mRotationKeys[j];
            ch.rotationKeys.push_back({k.mTime,
                glm::quat(k.mValue.w, k.mValue.x, k.mValue.y, k.mValue.z)});
        }

        for (unsigned int j = 0; j < pChannel->mNumScalingKeys; j++) {
            const auto& k = pChannel->mScalingKeys[j];
            ch.scalingKeys.push_back({k.mTime,
                glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z)});
        }

        anim.channels[nodeName] = ch;
    }

    m_animations[name] = anim;
}

void CAnimatedMesh::SetAnimation(const std::string& name)
{
    if (m_animations.find(name) != m_animations.end()) {
        m_currentAnimation = name;
        m_currentTime = 0.0f;
    }
}

float CAnimatedMesh::GetAnimationDuration(const std::string& name) const
{
    auto it = m_animations.find(name);
    if (it == m_animations.end()) return 0.0f;
    double tps = it->second.ticksPerSecond;
    return (float)(it->second.duration / tps);
}

// --- Animation update ---

void CAnimatedMesh::Update(float deltaTimeSec)
{
    if (m_currentAnimation.empty()) return;

    auto it = m_animations.find(m_currentAnimation);
    if (it == m_animations.end()) return;

    const Animation& anim = it->second;
    double tps = anim.ticksPerSecond;

    m_currentTime += deltaTimeSec;
    float durationSec = (float)(anim.duration / tps);

    if (m_looping) {
        if (m_currentTime > durationSec)
            m_currentTime = fmod(m_currentTime, durationSec);
    } else {
        if (m_currentTime > durationSec)
            m_currentTime = durationSec;
    }

    float animTimeTicks = (float)(m_currentTime * tps);

    ReadNodeHierarchy(animTimeTicks, anim, m_pScene->mRootNode, glm::mat4(1.0f));

    for (unsigned int i = 0; i < m_numBones && i < MAX_BONES; i++) {
        m_boneMatrices[i] = m_boneInfo[i].finalTransform;
    }
}

void CAnimatedMesh::ReadNodeHierarchy(float animTime, const Animation& anim,
                                       const aiNode* pNode,
                                       const glm::mat4& parentTransform)
{
    std::string nodeName(pNode->mName.data);
    glm::mat4 nodeTransform = AiToGlm(pNode->mTransformation);

    auto it = anim.channels.find(nodeName);
    if (it != anim.channels.end()) {
        const AnimChannel& ch = it->second;

        glm::vec3 scaling = InterpolateScaling(animTime, ch);
        glm::quat rotation = InterpolateRotation(animTime, ch);
        glm::vec3 translation = InterpolatePosition(animTime, ch);

        // Strip root XZ movement for in-place animation (keep Y for height)
        std::string nodeNameLower(pNode->mName.C_Str());
        bool isRootMotionNode = (pNode == m_pScene->mRootNode ||
                                 pNode->mParent == m_pScene->mRootNode ||
                                 (pNode->mParent && pNode->mParent->mParent == m_pScene->mRootNode) ||
                                 nodeNameLower.find("Hips") != std::string::npos);
        if (m_inPlace && isRootMotionNode) {
            translation.x = 0.0f;
            translation.z = 0.0f;
        }

        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaling);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);

        nodeTransform = T * R * S;
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    if (m_boneMapping.find(nodeName) != m_boneMapping.end()) {
        unsigned int boneIndex = m_boneMapping[nodeName];
        m_boneInfo[boneIndex].finalTransform =
            m_globalInverseTransform * globalTransform * m_boneInfo[boneIndex].offsetMatrix;
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHierarchy(animTime, anim, pNode->mChildren[i], globalTransform);
    }
}

// --- Interpolation helpers ---

glm::vec3 CAnimatedMesh::InterpolatePosition(float animTime, const AnimChannel& ch)
{
    if (ch.positionKeys.size() == 1)
        return ch.positionKeys[0].second;

    for (unsigned int i = 0; i < ch.positionKeys.size() - 1; i++) {
        if (animTime < (float)ch.positionKeys[i + 1].first) {
            float dt = (float)(ch.positionKeys[i + 1].first - ch.positionKeys[i].first);
            float factor = (animTime - (float)ch.positionKeys[i].first) / dt;
            if (factor < 0.0f) factor = 0.0f; if (factor > 1.0f) factor = 1.0f;
            return glm::mix(ch.positionKeys[i].second, ch.positionKeys[i + 1].second, factor);
        }
    }
    return ch.positionKeys.back().second;
}

glm::quat CAnimatedMesh::InterpolateRotation(float animTime, const AnimChannel& ch)
{
    if (ch.rotationKeys.size() == 1)
        return ch.rotationKeys[0].second;

    for (unsigned int i = 0; i < ch.rotationKeys.size() - 1; i++) {
        if (animTime < (float)ch.rotationKeys[i + 1].first) {
            float dt = (float)(ch.rotationKeys[i + 1].first - ch.rotationKeys[i].first);
            float factor = (animTime - (float)ch.rotationKeys[i].first) / dt;
            if (factor < 0.0f) factor = 0.0f; if (factor > 1.0f) factor = 1.0f;
            const glm::quat& q0 = ch.rotationKeys[i].second;
            const glm::quat& q1 = ch.rotationKeys[i + 1].second;
            return glm::normalize(q0 * (1.0f - factor) + q1 * factor);
        }
    }
    return ch.rotationKeys.back().second;
}

glm::vec3 CAnimatedMesh::InterpolateScaling(float animTime, const AnimChannel& ch)
{
    if (ch.scalingKeys.size() == 1)
        return ch.scalingKeys[0].second;

    for (unsigned int i = 0; i < ch.scalingKeys.size() - 1; i++) {
        if (animTime < (float)ch.scalingKeys[i + 1].first) {
            float dt = (float)(ch.scalingKeys[i + 1].first - ch.scalingKeys[i].first);
            float factor = (animTime - (float)ch.scalingKeys[i].first) / dt;
            if (factor < 0.0f) factor = 0.0f; if (factor > 1.0f) factor = 1.0f;
            return glm::mix(ch.scalingKeys[i].second, ch.scalingKeys[i + 1].second, factor);
        }
    }
    return ch.scalingKeys.back().second;
}

void CAnimatedMesh::UploadBoneMatrices()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSSBO);
    unsigned int numToUpload = m_numBones < MAX_BONES ? m_numBones : MAX_BONES;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numToUpload * sizeof(glm::mat4), m_boneMatrices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_boneSSBO);
}

// --- Render ---

void CAnimatedMesh::Render()
{
    static int debugCount = 0;
    if (debugCount < 10) {
        printf("Render[%d]: %zu entries, %zu textures, vao=%u, bones=%u\n", debugCount, m_entries.size(), m_textures.size(), m_vao, m_numBones);
        debugCount++;
    }
    glBindVertexArray(m_vao);

    for (unsigned int i = 0; i < m_entries.size(); i++) {
        const unsigned int matIndex = m_entries[i].materialIndex;
        if (matIndex < m_textures.size() && m_textures[matIndex]) {
            m_textures[matIndex]->Bind(0);
        }

        glDrawElements(GL_TRIANGLES, m_entries[i].numIndices,
                       GL_UNSIGNED_INT,
                       (void*)(m_entries[i].indexOffset * sizeof(unsigned int)));
    }

    glBindVertexArray(0);
}
