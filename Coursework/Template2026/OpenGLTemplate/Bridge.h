#pragma once

#include "Common.h"
#include "Texture.h"
#include <vector>

// Procedural bridge interior for the Perhonen.
// Cylinder room with comet-ice walls, angular furniture, wall panels,
// smartmatter mirror wall, and Sydän's jewel.
class CBridge
{
public:
    CBridge();
    ~CBridge();

    // texDir/texFile for wall panels
    void Create(const std::string& panelTexDir, const std::string& panelTexFile);
    void Render();
    void Release();

    // Smartmatter wall transparency (0 = mirror, 1 = fully transparent)
    float m_wallTransparency;

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;

    CTexture m_panelTexture;
    bool m_hasPanelTexture;

    // Vertex: pos(vec3) | uv(vec2) | normal(vec3) = 32 bytes (matches mainShader layout)
    struct BridgeVertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    std::vector<BridgeVertex> m_vertices;
    std::vector<unsigned int> m_indices;

    // Render ranges (triangle counts)
    unsigned int m_opaqueIndexCount;    // room shell + furniture
    unsigned int m_mirrorIndexStart;    // smartmatter wall start
    unsigned int m_mirrorIndexCount;    // smartmatter wall
    unsigned int m_emissiveIndexStart;  // jewel start
    unsigned int m_emissiveIndexCount;  // jewel

    unsigned int AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal);
    void AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2);
    void AddQuad(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3);

    void CreateCylinder(float length, float radius, int slices);
    void CreateFloor(float length, float radius);
    void CreateChair(glm::vec3 pos, float facing);
    void CreateTable(glm::vec3 pos);
    void CreateWallPanel(glm::vec3 center, glm::vec3 normal, float width, float height);
    void CreateSmartmatterWall(float length, float height, float z);
    void CreateJewel(glm::vec3 pos, float radius, int detail);
    void CreateBox(glm::vec3 center, glm::vec3 halfExtents, glm::vec3 normal);
};
