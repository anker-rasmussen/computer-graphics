#pragma once

#include <map>
#include <vector>
#include <string>
#include "include/gl/glew.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Common.h"
#include "Texture.h"

#define MAX_BONES_PER_VERTEX 4
#define MAX_BONES 128

struct AnimatedVertex
{
    glm::vec3 m_pos;
    glm::vec2 m_tex;
    glm::vec3 m_normal;
    int m_boneIDs[MAX_BONES_PER_VERTEX];
    float m_weights[MAX_BONES_PER_VERTEX];

    AnimatedVertex()
    {
        memset(m_boneIDs, 0, sizeof(m_boneIDs));
        memset(m_weights, 0, sizeof(m_weights));
    }

    void AddBone(int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONES_PER_VERTEX; i++) {
            if (m_weights[i] == 0.0f) {
                m_boneIDs[i] = boneID;
                m_weights[i] = weight;
                return;
            }
        }
    }
};

struct BoneInfo
{
    glm::mat4 offsetMatrix;    // transforms from mesh space to bone space
    glm::mat4 finalTransform;  // final transform applied to vertices
};

struct AnimChannel
{
    std::vector<std::pair<double, glm::vec3>> positionKeys;
    std::vector<std::pair<double, glm::quat>> rotationKeys;
    std::vector<std::pair<double, glm::vec3>> scalingKeys;
};

class CAnimatedMesh
{
public:
    CAnimatedMesh();
    ~CAnimatedMesh();

    bool Load(const std::string& filename, bool collapsePivots = false);
    void SetTexture(const std::string& texturePath);
    bool LoadAnimation(const std::string& filename, const std::string& name);
    void SetAnimation(const std::string& name);
    void Update(float deltaTimeSec);
    void Render();

    float GetAnimationDuration(const std::string& name) const;
    glm::mat4* GetBoneMatrices() { return m_boneMatrices; }
    unsigned int GetNumBones() const { return m_numBones < MAX_BONES ? m_numBones : MAX_BONES; }
    unsigned int GetNumEntries() const { return (unsigned int)m_entries.size(); }
    float GetCurrentTime() const { return m_currentTime; }
    void SetCurrentTime(float t) { m_currentTime = t; }
    void SetLooping(bool loop) { m_looping = loop; }
    void SetInPlace(bool inPlace) { m_inPlace = inPlace; }

private:
    struct MeshEntry {
        GLuint vbo;
        GLuint ibo;
        unsigned int numIndices;
        unsigned int materialIndex;
        unsigned int baseVertex;
        unsigned int indexOffset;  // byte offset into IBO
    };

    struct Animation {
        double duration;
        double ticksPerSecond;
        std::map<std::string, AnimChannel> channels;
    };

    bool InitFromScene(const aiScene* pScene, const std::string& filename);
    void InitMesh(unsigned int index, const aiMesh* paiMesh,
                  std::vector<AnimatedVertex>& vertices,
                  std::vector<unsigned int>& indices);
    void LoadBones(unsigned int meshIndex, const aiMesh* paiMesh,
                   std::vector<AnimatedVertex>& vertices);
    bool InitMaterials(const aiScene* pScene, const std::string& filename);

    void ExtractAnimation(const aiScene* pScene, const std::string& name);
    void ReadNodeHierarchy(float animTime, const Animation& anim,
                           const aiNode* pNode, const glm::mat4& parentTransform);

    glm::vec3 InterpolatePosition(float animTime, const AnimChannel& channel);
    glm::quat InterpolateRotation(float animTime, const AnimChannel& channel);
    glm::vec3 InterpolateScaling(float animTime, const AnimChannel& channel);

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    std::vector<MeshEntry> m_entries;
    std::vector<CTexture*> m_textures;

    // Bone data
    std::map<std::string, unsigned int> m_boneMapping;
    std::vector<BoneInfo> m_boneInfo;
    unsigned int m_numBones;
    glm::mat4 m_globalInverseTransform;

    // Node hierarchy (kept from the base mesh scene)
    Assimp::Importer m_importer;  // keeps the scene alive
    const aiScene* m_pScene;

    // Animations (can load multiple)
    std::map<std::string, Animation> m_animations;
    std::string m_currentAnimation;
    float m_currentTime;
    bool m_looping;
    bool m_inPlace;  // strip root XZ translation for in-place animation
    bool m_collapsePivots;

    // Final bone matrices sent to shader
    glm::mat4 m_boneMatrices[MAX_BONES];
    GLuint m_boneSSBO;

public:
    void UploadBoneMatrices(); // upload to SSBO binding 0
};
