#include "Common.h"

#define _USE_MATH_DEFINES

#include "Bridge.h"
#include <math.h>

CBridge::CBridge()
    : m_vao(0), m_vbo(0), m_ibo(0),
      m_hasPanelTexture(false),
      m_opaqueIndexCount(0), m_mirrorIndexStart(0), m_mirrorIndexCount(0),
      m_emissiveIndexStart(0), m_emissiveIndexCount(0),
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

// ---------------------------------------------------------------------------
// Cylinder shell — rendered from inside, normals point inward
// ---------------------------------------------------------------------------
void CBridge::CreateCylinder(float length, float radius, int slices)
{
    float halfLen = length * 0.5f;

    for (int i = 0; i <= slices; i++) {
        float theta = (float)i / (float)slices * 2.0f * (float)M_PI;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        // Inward-facing normal
        glm::vec3 normal = -glm::normalize(glm::vec3(x, y, 0.0f));
        float u = (float)i / (float)slices;

        // Front ring
        AddVertex(glm::vec3(x, y, halfLen), glm::vec2(u, 0.0f), normal);
        // Back ring
        AddVertex(glm::vec3(x, y, -halfLen), glm::vec2(u, 1.0f), normal);
    }

    for (int i = 0; i < slices; i++) {
        unsigned int base = i * 2;
        // Wind CW from inside
        AddQuad(base, base + 2, base + 3, base + 1);
    }
}

// ---------------------------------------------------------------------------
// Flat floor at bottom of cylinder
// ---------------------------------------------------------------------------
void CBridge::CreateFloor(float length, float radius)
{
    float halfLen = length * 0.5f;
    // Floor sits at bottom of cylinder
    float floorY = -radius * 0.85f;
    float floorHalfW = radius * 0.9f;
    glm::vec3 n(0, 1, 0);

    unsigned int v0 = AddVertex(glm::vec3(-floorHalfW, floorY, -halfLen), glm::vec2(0, 0), n);
    unsigned int v1 = AddVertex(glm::vec3( floorHalfW, floorY, -halfLen), glm::vec2(1, 0), n);
    unsigned int v2 = AddVertex(glm::vec3( floorHalfW, floorY,  halfLen), glm::vec2(1, 1), n);
    unsigned int v3 = AddVertex(glm::vec3(-floorHalfW, floorY,  halfLen), glm::vec2(0, 1), n);
    AddQuad(v0, v1, v2, v3);

    // Grid lines — subtle raised strips across the floor
    float stripH = 0.005f;
    int numStrips = 8;
    for (int i = 1; i < numStrips; i++) {
        float z = -halfLen + length * (float)i / (float)numStrips;
        float hw = 0.02f; // strip half-width in Z
        unsigned int s0 = AddVertex(glm::vec3(-floorHalfW, floorY + stripH, z - hw), glm::vec2(0, 0.5f), n);
        unsigned int s1 = AddVertex(glm::vec3( floorHalfW, floorY + stripH, z - hw), glm::vec2(1, 0.5f), n);
        unsigned int s2 = AddVertex(glm::vec3( floorHalfW, floorY + stripH, z + hw), glm::vec2(1, 0.5f), n);
        unsigned int s3 = AddVertex(glm::vec3(-floorHalfW, floorY + stripH, z + hw), glm::vec2(0, 0.5f), n);
        AddQuad(s0, s1, s2, s3);
    }
}

// ---------------------------------------------------------------------------
// Angular chair — a low-profile, clean geometric seat
// ---------------------------------------------------------------------------
void CBridge::CreateChair(glm::vec3 pos, float facingAngle)
{
    float rad = facingAngle * (float)M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad);

    auto rotate = [&](glm::vec3 p) -> glm::vec3 {
        return glm::vec3(p.x * c - p.z * s, p.y, p.x * s + p.z * c) + pos;
    };

    // Seat: flat slab
    float seatW = 0.4f, seatD = 0.4f, seatH = 0.02f;
    float seatY = 0.35f;
    glm::vec3 n_up(0, 1, 0);
    glm::vec3 n_front = glm::normalize(glm::vec3(s, 0, c));

    // Seat top
    glm::vec3 corners[4] = {
        rotate(glm::vec3(-seatW, seatY, -seatD)),
        rotate(glm::vec3( seatW, seatY, -seatD)),
        rotate(glm::vec3( seatW, seatY,  seatD)),
        rotate(glm::vec3(-seatW, seatY,  seatD))
    };
    unsigned int v0 = AddVertex(corners[0], glm::vec2(0,0), n_up);
    unsigned int v1 = AddVertex(corners[1], glm::vec2(1,0), n_up);
    unsigned int v2 = AddVertex(corners[2], glm::vec2(1,1), n_up);
    unsigned int v3 = AddVertex(corners[3], glm::vec2(0,1), n_up);
    AddQuad(v0, v1, v2, v3);

    // Seat bottom
    glm::vec3 n_down(0, -1, 0);
    glm::vec3 botCorners[4] = {
        rotate(glm::vec3(-seatW, seatY - seatH, -seatD)),
        rotate(glm::vec3( seatW, seatY - seatH, -seatD)),
        rotate(glm::vec3( seatW, seatY - seatH,  seatD)),
        rotate(glm::vec3(-seatW, seatY - seatH,  seatD))
    };
    unsigned int b0 = AddVertex(botCorners[0], glm::vec2(0,0), n_down);
    unsigned int b1 = AddVertex(botCorners[1], glm::vec2(1,0), n_down);
    unsigned int b2 = AddVertex(botCorners[2], glm::vec2(1,1), n_down);
    unsigned int b3 = AddVertex(botCorners[3], glm::vec2(0,1), n_down);
    AddQuad(b3, b2, b1, b0);

    // Seat sides (4 faces)
    // Front
    unsigned int f0 = AddVertex(corners[0], glm::vec2(0,0), -n_front);
    unsigned int f1 = AddVertex(corners[1], glm::vec2(1,0), -n_front);
    unsigned int f2 = AddVertex(botCorners[1], glm::vec2(1,1), -n_front);
    unsigned int f3 = AddVertex(botCorners[0], glm::vec2(0,1), -n_front);
    AddQuad(f0, f1, f2, f3);

    // Backrest: angled slab behind the seat
    float backH = 0.55f, backThick = 0.02f;
    float backAngle = 0.15f; // slight lean back
    glm::vec3 backCorners[4] = {
        rotate(glm::vec3(-seatW, seatY, seatD)),
        rotate(glm::vec3( seatW, seatY, seatD)),
        rotate(glm::vec3( seatW, seatY + backH, seatD + backAngle)),
        rotate(glm::vec3(-seatW, seatY + backH, seatD + backAngle))
    };
    unsigned int bk0 = AddVertex(backCorners[0], glm::vec2(0,0), n_front);
    unsigned int bk1 = AddVertex(backCorners[1], glm::vec2(1,0), n_front);
    unsigned int bk2 = AddVertex(backCorners[2], glm::vec2(1,1), n_front);
    unsigned int bk3 = AddVertex(backCorners[3], glm::vec2(0,1), n_front);
    AddQuad(bk0, bk1, bk2, bk3);

    // Two legs — thin angular supports
    float legW = 0.04f;
    for (int side = -1; side <= 1; side += 2) {
        float lx = side * (seatW - 0.1f);
        glm::vec3 legCorners[4] = {
            rotate(glm::vec3(lx - legW, 0.0f,  seatD * 0.5f)),
            rotate(glm::vec3(lx + legW, 0.0f,  seatD * 0.5f)),
            rotate(glm::vec3(lx + legW, seatY, seatD * 0.5f)),
            rotate(glm::vec3(lx - legW, seatY, seatD * 0.5f))
        };
        glm::vec3 legN = glm::normalize(glm::vec3((float)side, 0, 0));
        unsigned int l0 = AddVertex(legCorners[0], glm::vec2(0,0), legN);
        unsigned int l1 = AddVertex(legCorners[1], glm::vec2(1,0), legN);
        unsigned int l2 = AddVertex(legCorners[2], glm::vec2(1,1), legN);
        unsigned int l3 = AddVertex(legCorners[3], glm::vec2(0,1), legN);
        AddQuad(l0, l1, l2, l3);
    }
}

// ---------------------------------------------------------------------------
// Low central table
// ---------------------------------------------------------------------------
void CBridge::CreateTable(glm::vec3 pos)
{
    float halfW = 0.5f, halfD = 0.3f;
    float topY = 0.3f, thick = 0.015f;
    glm::vec3 n_up(0, 1, 0);

    // Table top
    unsigned int v0 = AddVertex(pos + glm::vec3(-halfW, topY, -halfD), glm::vec2(0,0), n_up);
    unsigned int v1 = AddVertex(pos + glm::vec3( halfW, topY, -halfD), glm::vec2(1,0), n_up);
    unsigned int v2 = AddVertex(pos + glm::vec3( halfW, topY,  halfD), glm::vec2(1,1), n_up);
    unsigned int v3 = AddVertex(pos + glm::vec3(-halfW, topY,  halfD), glm::vec2(0,1), n_up);
    AddQuad(v0, v1, v2, v3);

    // Table underside
    glm::vec3 n_dn(0, -1, 0);
    unsigned int u0 = AddVertex(pos + glm::vec3(-halfW, topY - thick, -halfD), glm::vec2(0,0), n_dn);
    unsigned int u1 = AddVertex(pos + glm::vec3( halfW, topY - thick, -halfD), glm::vec2(1,0), n_dn);
    unsigned int u2 = AddVertex(pos + glm::vec3( halfW, topY - thick,  halfD), glm::vec2(1,1), n_dn);
    unsigned int u3 = AddVertex(pos + glm::vec3(-halfW, topY - thick,  halfD), glm::vec2(0,1), n_dn);
    AddQuad(u3, u2, u1, u0);

    // Single center leg
    float legW = 0.04f;
    glm::vec3 legN(0, 0, 1);
    unsigned int l0 = AddVertex(pos + glm::vec3(-legW, 0.0f,  -legW), glm::vec2(0,0), legN);
    unsigned int l1 = AddVertex(pos + glm::vec3( legW, 0.0f,  -legW), glm::vec2(1,0), legN);
    unsigned int l2 = AddVertex(pos + glm::vec3( legW, topY - thick, -legW), glm::vec2(1,1), legN);
    unsigned int l3 = AddVertex(pos + glm::vec3(-legW, topY - thick, -legW), glm::vec2(0,1), legN);
    AddQuad(l0, l1, l2, l3);

    legN = glm::vec3(0, 0, -1);
    unsigned int l4 = AddVertex(pos + glm::vec3(-legW, 0.0f,   legW), glm::vec2(0,0), legN);
    unsigned int l5 = AddVertex(pos + glm::vec3( legW, 0.0f,   legW), glm::vec2(1,0), legN);
    unsigned int l6 = AddVertex(pos + glm::vec3( legW, topY - thick,  legW), glm::vec2(1,1), legN);
    unsigned int l7 = AddVertex(pos + glm::vec3(-legW, topY - thick,  legW), glm::vec2(0,1), legN);
    AddQuad(l7, l6, l5, l4);
}

// ---------------------------------------------------------------------------
// Wall display panel — flat textured quad mounted on the cylinder wall
// ---------------------------------------------------------------------------
void CBridge::CreateWallPanel(glm::vec3 center, glm::vec3 normal, float width, float height)
{
    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, normal));
    glm::vec3 actualUp = glm::normalize(glm::cross(normal, right));

    float hw = width * 0.5f, hh = height * 0.5f;

    unsigned int v0 = AddVertex(center - right * hw - actualUp * hh, glm::vec2(0, 0), normal);
    unsigned int v1 = AddVertex(center + right * hw - actualUp * hh, glm::vec2(1, 0), normal);
    unsigned int v2 = AddVertex(center + right * hw + actualUp * hh, glm::vec2(1, 1), normal);
    unsigned int v3 = AddVertex(center - right * hw + actualUp * hh, glm::vec2(0, 1), normal);
    AddQuad(v0, v1, v2, v3);
}

// ---------------------------------------------------------------------------
// Smartmatter wall — the large panel that goes transparent
// This is a separate render range so we can control its alpha
// ---------------------------------------------------------------------------
void CBridge::CreateSmartmatterWall(float length, float height, float z)
{
    m_mirrorIndexStart = (unsigned int)m_indices.size();

    float halfLen = length * 0.5f;
    float floorY = -2.5f * 0.85f;  // match floor position
    glm::vec3 n(0, 0, -1);  // faces into the room (wall at +Z end)

    unsigned int v0 = AddVertex(glm::vec3(-halfLen, floorY, z),           glm::vec2(0, 0), n);
    unsigned int v1 = AddVertex(glm::vec3( halfLen, floorY, z),           glm::vec2(1, 0), n);
    unsigned int v2 = AddVertex(glm::vec3( halfLen, floorY + height, z),  glm::vec2(1, 1), n);
    unsigned int v3 = AddVertex(glm::vec3(-halfLen, floorY + height, z),  glm::vec2(0, 1), n);
    AddQuad(v0, v1, v2, v3);

    m_mirrorIndexCount = (unsigned int)m_indices.size() - m_mirrorIndexStart;
}

// ---------------------------------------------------------------------------
// Sydän's jewel — a small glowing sphere
// ---------------------------------------------------------------------------
void CBridge::CreateJewel(glm::vec3 pos, float radius, int detail)
{
    m_emissiveIndexStart = (unsigned int)m_indices.size();

    unsigned int baseVert = (unsigned int)m_vertices.size();

    // Simple UV sphere
    for (int j = 0; j <= detail; j++) {
        float phi = (float)M_PI * (float)j / (float)detail;
        for (int i = 0; i <= detail; i++) {
            float theta = 2.0f * (float)M_PI * (float)i / (float)detail;
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);
            glm::vec3 n(x, y, z);
            float u = (float)i / (float)detail;
            float v = (float)j / (float)detail;
            AddVertex(pos + n * radius, glm::vec2(u, v), n);
        }
    }

    for (int j = 0; j < detail; j++) {
        for (int i = 0; i < detail; i++) {
            unsigned int row0 = baseVert + j * (detail + 1) + i;
            unsigned int row1 = row0 + detail + 1;
            AddTriangle(row0, row1, row0 + 1);
            AddTriangle(row0 + 1, row1, row1 + 1);
        }
    }

    m_emissiveIndexCount = (unsigned int)m_indices.size() - m_emissiveIndexStart;
}

// ---------------------------------------------------------------------------
// Create — builds all geometry and uploads to GPU
// ---------------------------------------------------------------------------
void CBridge::Create(const std::string& panelTexDir, const std::string& panelTexFile)
{
    float cylLength = 10.0f;
    float cylRadius = 2.5f;
    float floorY = -cylRadius * 0.85f;

    // --- Opaque geometry ---

    // Cylinder walls
    CreateCylinder(cylLength, cylRadius, 32);

    // Floor with grid lines
    CreateFloor(cylLength, cylRadius);

    // Two chairs facing each other across the table
    CreateChair(glm::vec3(-0.8f, floorY, -0.5f), 90.0f);   // left chair, facing right
    CreateChair(glm::vec3( 0.8f, floorY, -0.5f), -90.0f);  // right chair, facing left

    // Low table between them
    CreateTable(glm::vec3(0.0f, floorY, -0.5f));

    // Wall panels — mounted on the curved walls at various angles
    // Left wall panel
    CreateWallPanel(
        glm::vec3(-cylRadius * 0.95f, floorY + 1.5f, -2.0f),
        glm::vec3(1, 0, 0), 1.2f, 0.8f);

    // Right wall panel
    CreateWallPanel(
        glm::vec3(cylRadius * 0.95f, floorY + 1.5f, -2.0f),
        glm::vec3(-1, 0, 0), 1.2f, 0.8f);

    // Back wall panel (navigation display)
    CreateWallPanel(
        glm::vec3(0.0f, floorY + 1.5f, -cylLength * 0.5f + 0.05f),
        glm::vec3(0, 0, 1), 2.0f, 1.0f);

    m_opaqueIndexCount = (unsigned int)m_indices.size();

    // --- Smartmatter wall (separate render range for alpha control) ---
    CreateSmartmatterWall(cylRadius * 1.6f, cylRadius * 1.5f, cylLength * 0.5f - 0.01f);

    // --- Sydän's jewel (emissive, separate range) ---
    CreateJewel(glm::vec3(1.5f, floorY + 0.5f, 2.0f), 0.08f, 12);

    // --- Upload to GPU ---
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

    // pos: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, pos));
    // uv: location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, uv));
    // normal: location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BridgeVertex),
                          (void*)offsetof(BridgeVertex, normal));

    glBindVertexArray(0);

    // Load panel texture
    if (!panelTexFile.empty()) {
        std::string fullPath = panelTexDir + panelTexFile;
        if (m_panelTexture.Load(fullPath, true)) {
            m_hasPanelTexture = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Render — three passes: opaque room, smartmatter wall, jewel
// ---------------------------------------------------------------------------
void CBridge::Render()
{
    glBindVertexArray(m_vao);

    // Pass 1: opaque room geometry
    glDrawElements(GL_TRIANGLES, m_opaqueIndexCount, GL_UNSIGNED_INT, 0);

    // Pass 2: smartmatter wall (caller should set alpha uniform)
    if (m_mirrorIndexCount > 0) {
        glDrawElements(GL_TRIANGLES, m_mirrorIndexCount, GL_UNSIGNED_INT,
                       (void*)(m_mirrorIndexStart * sizeof(unsigned int)));
    }

    // Pass 3: jewel (caller should set emissive material)
    if (m_emissiveIndexCount > 0) {
        glDrawElements(GL_TRIANGLES, m_emissiveIndexCount, GL_UNSIGNED_INT,
                       (void*)(m_emissiveIndexStart * sizeof(unsigned int)));
    }

    glBindVertexArray(0);
}

void CBridge::Release()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    m_vao = m_vbo = m_ibo = 0;
    if (m_hasPanelTexture) m_panelTexture.Release();
}
