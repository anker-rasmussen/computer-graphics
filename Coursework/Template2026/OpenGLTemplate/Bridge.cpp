#include "Common.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "Bridge.h"

CBridge::CBridge()
    : m_vao(0), m_vbo(0), m_ibo(0),
      m_floorIndexCount(0), m_wallIndexStart(0),
      m_roomIndexCount(0), m_mirrorIndexStart(0), m_mirrorIndexCount(0),
      m_wallTransparency(0.0f)
{}

CBridge::~CBridge()
{}

unsigned int CBridge::AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal)
{
    BridgeVertex v;
    v.pos = pos;
    v.uv = uv;
    v.normal = normal;
    m_vertices.push_back(v);
    return (unsigned int)(m_vertices.size() - 1);
}

void CBridge::AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2)
{
    m_indices.push_back(i0);
    m_indices.push_back(i1);
    m_indices.push_back(i2);
}

void CBridge::AddQuad(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3)
{
    AddTriangle(i0, i1, i2);
    AddTriangle(i0, i2, i3);
}

void CBridge::Create(float width, float height, float depth)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    // Floor (y = 0)
    {
        glm::vec3 n(0, 1, 0);
        float tile = 1.5f; // tiles per unit — higher = smaller tiles
        unsigned int v0 = AddVertex(glm::vec3(-hw, 0, -hd), glm::vec2(0, 0), n);
        unsigned int v1 = AddVertex(glm::vec3( hw, 0, -hd), glm::vec2(width * tile, 0), n);
        unsigned int v2 = AddVertex(glm::vec3( hw, 0,  hd), glm::vec2(width * tile, depth * tile), n);
        unsigned int v3 = AddVertex(glm::vec3(-hw, 0,  hd), glm::vec2(0, depth * tile), n);
        AddQuad(v3, v2, v1, v0);
    }

    m_floorIndexCount = (unsigned int)m_indices.size();
    m_wallIndexStart = m_floorIndexCount;

    // Curved walls + ceiling as a half-cylinder arch (replaces flat walls and ceiling)
    // Arc from left floor edge, up over the ceiling, down to right floor edge
    // This gives curved side walls that merge into a curved ceiling
    {
        int arcSegments = 16;
        float arcRadius = hw; // radius matches half-width
        float centerY = 0.0f; // arc center at floor level

        // Arc goes from angle PI (left floor) to 0 (right floor)
        // through PI/2 (ceiling top)
        for (int j = 0; j < arcSegments; j++) {
            float a0 = (float)M_PI * (1.0f - (float)j / arcSegments);
            float a1 = (float)M_PI * (1.0f - (float)(j + 1) / arcSegments);

            float x0 = arcRadius * cosf(a0);
            float y0 = centerY + arcRadius * sinf(a0);
            float x1 = arcRadius * cosf(a1);
            float y1 = centerY + arcRadius * sinf(a1);

            // Inward-pointing normals
            glm::vec3 n0(-cosf(a0), -sinf(a0), 0.0f);
            glm::vec3 n1(-cosf(a1), -sinf(a1), 0.0f);

            float wallTile = 3.0f;
            float u0 = (float)j / arcSegments * wallTile;
            float u1 = (float)(j + 1) / arcSegments * wallTile;
            float vTile = depth * 1.5f;

            // Front edge (z = +hd) and back edge (z = -hd)
            unsigned int v0 = AddVertex(glm::vec3(x0, y0,  hd), glm::vec2(u0, 0), n0);
            unsigned int v1 = AddVertex(glm::vec3(x0, y0, -hd), glm::vec2(u0, vTile), n0);
            unsigned int v2 = AddVertex(glm::vec3(x1, y1, -hd), glm::vec2(u1, vTile), n1);
            unsigned int v3 = AddVertex(glm::vec3(x1, y1,  hd), glm::vec2(u1, 0), n1);
            AddQuad(v0, v1, v2, v3);
        }
    }

    // Back wall (z = -hd, normal points inward +z) — solid arch, no door
    {
        glm::vec3 n(0, 0, 1);
        int arcSegments = 16;
        float arcRadius = hw;
        float wallTile = 2.0f;

        // Build arch outline: right floor -> arc -> left floor
        std::vector<unsigned int> outline;
        outline.push_back(AddVertex(glm::vec3(hw, 0, -hd), glm::vec2(wallTile, 0), n));
        for (int i = 0; i <= arcSegments; i++) {
            float a = (float)M_PI * (float)i / arcSegments;
            float x = arcRadius * cosf(a);
            float y = arcRadius * sinf(a);
            float u = (1.0f - (float)i / arcSegments) * wallTile;
            outline.push_back(AddVertex(glm::vec3(x, y, -hd), glm::vec2(u, y / arcRadius * wallTile), n));
        }
        outline.push_back(AddVertex(glm::vec3(-hw, 0, -hd), glm::vec2(0, 0), n));

        // Fan triangulation from first vertex
        for (unsigned int i = 1; i < outline.size() - 1; i++) {
            AddTriangle(outline[0], outline[i], outline[i + 1]);
        }
    }

    m_roomIndexCount = (unsigned int)m_indices.size();

    // Front wall — viewport (z = +hd, normal points inward -z)
    // Simple square for clean texture mapping
    m_mirrorIndexStart = (unsigned int)m_indices.size();
    {
        glm::vec3 n(0, 0, -1);
        unsigned int v0 = AddVertex(glm::vec3(-hw, 0, hd), glm::vec2(0, 0), n);
        unsigned int v1 = AddVertex(glm::vec3( hw, 0, hd), glm::vec2(1, 0), n);
        unsigned int v2 = AddVertex(glm::vec3( hw, hw, hd), glm::vec2(1, 1), n);
        unsigned int v3 = AddVertex(glm::vec3(-hw, hw, hd), glm::vec2(0, 1), n);
        AddQuad(v3, v2, v1, v0);
    }
    m_mirrorIndexCount = (unsigned int)m_indices.size() - m_mirrorIndexStart;

    // Upload to GPU
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(BridgeVertex),
                 m_vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
                 m_indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, normal));

    glBindVertexArray(0);
}

void CBridge::RenderRoom()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_roomIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CBridge::RenderFloor()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CBridge::RenderWalls()
{
    glBindVertexArray(m_vao);
    unsigned int wallCount = m_roomIndexCount - m_wallIndexStart;
    glDrawElements(GL_TRIANGLES, wallCount, GL_UNSIGNED_INT,
                   (void*)(m_wallIndexStart * sizeof(unsigned int)));
    glBindVertexArray(0);
}

void CBridge::RenderMirrorWall()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_mirrorIndexCount, GL_UNSIGNED_INT,
                   (void*)(m_mirrorIndexStart * sizeof(unsigned int)));
    glBindVertexArray(0);
}

void CBridge::Release()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    m_vao = m_vbo = m_ibo = 0;
}
