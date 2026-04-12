#include "Common.h"
#include "TacticalGrid.h"
#include "CatmullRom.h"
#include "Shaders.h"
#include "Camera.h"
#include <cstdlib>

CTacticalGrid::CTacticalGrid()
	: m_pSpline(NULL), m_totalRows(0), m_gridVAO(0), m_gridVBO(0)
{}

CTacticalGrid::~CTacticalGrid()
{
	if (m_gridVAO) glDeleteVertexArrays(1, &m_gridVAO);
	if (m_gridVBO) glDeleteBuffers(1, &m_gridVBO);
}

void CTacticalGrid::Init(CCatmullRom *pSpline)
{
	m_pSpline = pSpline;
	m_totalRows = (int)(pSpline->GetTotalLength() / SEGMENT_LENGTH);
}

float CTacticalGrid::LaneToOffset(int lane)
{
	// lane 0 = far left (-1.0), lane NUM_LANES-1 = far right (+1.0)
	float center = (NUM_LANES - 1) * 0.5f;
	return (lane - center) / center;
}

glm::vec3 CTacticalGrid::GetCellWorldPos(int absoluteRow, int lane) const
{
	float distance = absoluteRow * (float)SEGMENT_LENGTH;
	glm::vec3 pos, T, N, B;
	m_pSpline->SampleTNB(distance, pos, T, N, B);
	return pos + N * (LaneToOffset(lane) * HALF_WIDTH);
}

void CTacticalGrid::GetCellTNB(int absoluteRow, int lane,
                                glm::vec3 &pos, glm::vec3 &T, glm::vec3 &N, glm::vec3 &B) const
{
	float distance = absoluteRow * (float)SEGMENT_LENGTH;
	glm::vec3 centrePos;
	m_pSpline->SampleTNB(distance, centrePos, T, N, B);
	pos = centrePos + N * (LaneToOffset(lane) * HALF_WIDTH);
}

GridCell& CTacticalGrid::GetCell(int absoluteRow, int lane)
{
	return m_cells[absoluteRow][lane];
}

const GridCell& CTacticalGrid::GetCellConst(int absoluteRow, int lane) const
{
	static GridCell empty;
	auto it = m_cells.find(absoluteRow);
	if (it == m_cells.end()) return empty;
	return it->second[lane];
}

bool CTacticalGrid::HasCell(int absoluteRow, int lane) const
{
	return m_cells.find(absoluteRow) != m_cells.end()
	       && lane >= 0 && lane < NUM_LANES;
}

void CTacticalGrid::RevealRows(int fromRow, int toRow, unsigned int rngSeed)
{
	for (int row = fromRow; row <= toRow; row++) {
		if (m_cells.find(row) != m_cells.end()) continue;
		if (row < 0 || row >= m_totalRows) continue;

		std::array<GridCell, NUM_LANES> rowCells;

		// Deterministic RNG per row
		unsigned int hash = rngSeed ^ (row * 2654435761u);

		for (int lane = 0; lane < NUM_LANES; lane++) {
			unsigned int cellHash = hash ^ (lane * 1013904223u);
			float roll = (float)(cellHash % 1000) / 1000.0f;

			if (roll < 0.05f && row > 3) {
				// Asteroid obstacle (~5%)
				rowCells[lane].type = CellType::Asteroid;
				rowCells[lane].asteroidSeed = (int)(cellHash & 0xFFFF);
			} else if (roll < 0.15f) {
				// Fuel pickup (~10%)
				rowCells[lane].type = CellType::Fuel;
				rowCells[lane].fuelBonus = 1;
			} else {
				rowCells[lane].type = CellType::Empty;
			}
		}

		// Ensure at least one lane per row is passable (not asteroid)
		bool hasOpen = false;
		for (int lane = 0; lane < NUM_LANES; lane++) {
			if (rowCells[lane].type != CellType::Asteroid) { hasOpen = true; break; }
		}
		if (!hasOpen) {
			rowCells[2].type = CellType::Empty; // clear centre lane
		}

		m_cells[row] = rowCells;
	}
}

void CTacticalGrid::TickDangers()
{
	for (auto &pair : m_cells) {
		for (int lane = 0; lane < NUM_LANES; lane++) {
			if (pair.second[lane].type == CellType::Danger) {
				pair.second[lane].danger.turnsUntilImpact--;
			}
		}
	}
}

void CTacticalGrid::ClearExpiredDangers()
{
	for (auto &pair : m_cells) {
		for (int lane = 0; lane < NUM_LANES; lane++) {
			if (pair.second[lane].type == CellType::Danger &&
			    pair.second[lane].danger.turnsUntilImpact <= 0) {
				pair.second[lane].type = CellType::Empty;
			}
		}
	}
}

void CTacticalGrid::CreateGL()
{
	glGenVertexArrays(1, &m_gridVAO);
	glGenBuffers(1, &m_gridVBO);
}

// -----------------------------------------------------------------------
// Render the visible grid as coloured quads using the main shader
// -----------------------------------------------------------------------
void CTacticalGrid::RenderGrid(int playerRow, int playerLane, int dangerRow,
                                const std::vector<std::pair<int,int>> &plannedPath,
                                CShaderProgram *pMainProgram, CCamera *pCamera,
                                glm::mat4 viewMatrix)
{
	// Build a set of planned cells for quick lookup
	std::map<uint64_t, bool> plannedSet;
	for (auto &p : plannedPath) {
		uint64_t key = ((uint64_t)p.first << 32) | (uint64_t)(unsigned int)p.second;
		plannedSet[key] = true;
	}

	// Vertex layout matching main shader: pos(vec3) + uv(vec2) + normal(vec3)
	struct GridVert {
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", false);
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
	pMainProgram->SetUniform("material1.shininess", 1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	// Show from danger row (red) through visible rows ahead
	int startRow = glm::max(dangerRow, 0);
	int endRow = glm::min(playerRow + VISIBLE_ROWS, m_totalRows - 1);

	for (int row = startRow; row <= endRow; row++) {
		for (int lane = 0; lane < NUM_LANES; lane++) {
			glm::vec3 cellPos, T, N, B;
			GetCellTNB(row, lane, cellPos, T, N, B);

			// Determine cell colour
			glm::vec3 colour(0.1f, 0.15f, 0.25f);
			float alpha = 0.25f;

			// Consumed danger rows — solid red
			if (row <= dangerRow) {
				colour = glm::vec3(0.9f, 0.1f, 0.05f);
				alpha = 0.6f;
			} else {
				const GridCell &cell = GetCellConst(row, lane);

				if (cell.type == CellType::Danger) {
					colour = glm::vec3(0.9f, 0.15f, 0.1f);
					alpha = 0.55f;
				} else if (cell.type == CellType::Fuel) {
					colour = glm::vec3(0.1f, 0.7f, 0.2f);
					alpha = 0.4f;
				} else if (cell.type == CellType::Asteroid && !cell.destroyed) {
					colour = glm::vec3(0.5f, 0.35f, 0.15f);
					alpha = 0.45f;
				}

				// Player cell: bright white
				if (row == playerRow && lane == playerLane) {
					colour = glm::vec3(1.0f, 1.0f, 1.0f);
					alpha = 0.5f;
				}

				// Planned path: cyan
				uint64_t key = ((uint64_t)row << 32) | (uint64_t)(unsigned int)lane;
				if (plannedSet.count(key)) {
					colour = glm::vec3(0.0f, 0.85f, 0.95f);
					alpha = 0.5f;
				}
			}

			// Build quad in TNB space — square cells
			float halfCell = (float)SEGMENT_LENGTH * 0.45f;
			float lift = 0.5f;

			glm::vec3 c = cellPos + B * lift;
			glm::vec3 v0 = c - T * halfCell - N * halfCell;
			glm::vec3 v1 = c + T * halfCell - N * halfCell;
			glm::vec3 v2 = c + T * halfCell + N * halfCell;
			glm::vec3 v3 = c - T * halfCell + N * halfCell;

			glm::vec2 uv00(0,0), uv10(1,0), uv11(1,1), uv01(0,1);
			GridVert quad[6] = {
				{v0, uv00, B}, {v1, uv10, B}, {v2, uv11, B},
				{v0, uv00, B}, {v2, uv11, B}, {v3, uv01, B}
			};

			// Set material for this cell
			pMainProgram->SetUniform("material1.Ma", colour * 0.8f);
			pMainProgram->SetUniform("material1.Md", colour);
			pMainProgram->SetUniform("alpha", alpha);

			// Upload and draw
			glBindVertexArray(m_gridVAO);
			glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);

			GLsizei stride = sizeof(GridVert);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));

			glm::mat4 identity(1.0f);
			pMainProgram->SetUniform("matrices.modelViewMatrix", viewMatrix);
			pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(viewMatrix));

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	pMainProgram->SetUniform("alpha", 0.0f); // reset alpha
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}
