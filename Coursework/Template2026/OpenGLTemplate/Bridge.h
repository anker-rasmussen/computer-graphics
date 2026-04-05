#pragma once

#include "Common.h"
#include "Texture.h"
#include <vector>

// Procedural bridge room for the Perhonen.
// Rectangular room: walls, floor, ceiling, smartmatter mirror front wall.
// Furniture is loaded as separate meshes in Game.
class CBridge
{
public:
    CBridge();
    ~CBridge();

    void Create(float width, float height, float depth);
    void RenderRoom();          // walls, floor, ceiling (opaque)
    void RenderFloor();         // floor only (for separate texture)
    void RenderWalls();         // walls + ceiling only
    void RenderMirrorWall();    // front wall (separate for alpha control)
    void Release();

    float m_wallTransparency;   // 0 = mirror, 1 = fully transparent

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;

    struct BridgeVertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    std::vector<BridgeVertex> m_vertices;
    std::vector<unsigned int> m_indices;

    unsigned int m_floorIndexCount;
    unsigned int m_wallIndexStart;
    unsigned int m_roomIndexCount;
    unsigned int m_mirrorIndexStart;
    unsigned int m_mirrorIndexCount;

    unsigned int AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal);
    void AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2);
    void AddQuad(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3);
};
