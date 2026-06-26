#pragma once

#include "Common.h"
#include <map>
#include <array>

class CCatmullRom;
class CShaderProgram;
class CCamera;

// -----------------------------------------------------------------------
// Grid cell types and data
// -----------------------------------------------------------------------
enum class CellType {
	Empty,
	Fuel,       // fuel bonus pickup
	Asteroid,   // destructible obstacle
	Danger      // enemy fire incoming
};

struct DangerInfo {
	int turnsUntilImpact = 0;
	int sourceEnemyId = -1;
};

struct GridCell {
	CellType type = CellType::Empty;
	DangerInfo danger;
	int fuelBonus = 0;
	bool destroyed = false;   // asteroid was mined
	int asteroidSeed = 0;     // for unique Perlin mesh per obstacle
};

// -----------------------------------------------------------------------
// Tactical grid overlaid on the CatmullRom spline
// -----------------------------------------------------------------------
class CTacticalGrid
{
public:
	static const int NUM_LANES = 10;
	static const int VISIBLE_ROWS = 8;
	static const int SEGMENT_LENGTH = 20; // arc-length units per row (matches lane width for square cells)
	static constexpr float HALF_WIDTH = 100.0f; // track half-width — 10 lanes of 20 units each

	CTacticalGrid();
	~CTacticalGrid();

	void Init(CCatmullRom *pSpline);

	// World-space queries
	glm::vec3 GetCellWorldPos(int absoluteRow, int lane) const;
	void GetCellTNB(int absoluteRow, int lane,
	                glm::vec3 &pos, glm::vec3 &T, glm::vec3 &N, glm::vec3 &B) const;

	// Grid cell access
	GridCell& GetCell(int absoluteRow, int lane);
	const GridCell& GetCellConst(int absoluteRow, int lane) const;
	bool HasCell(int absoluteRow, int lane) const;

	// Populate new rows as player advances
	void RevealRows(int fromRow, int toRow, unsigned int rngSeed);

	// Clear danger zones that have expired (countdown reached 0)
	void ClearExpiredDangers();
	// Decrement all danger countdowns by 1
	void TickDangers();

	// Total rows available in the spline
	int GetTotalRows() const { return m_totalRows; }

	// Rendering — caller must have main shader active with modelViewMatrix/normalMatrix/projMatrix set
	void CreateGL();
	void RenderGrid(int playerRow, int playerLane, int dangerRow,
	                const std::vector<std::pair<int,int>> &plannedPath,
	                CShaderProgram *pMainProgram, CCamera *pCamera,
	                glm::mat4 viewMatrix);

	static float LaneToOffset(int lane);

private:
	CCatmullRom *m_pSpline;
	int m_totalRows;

	// Sparse row storage: row -> array of 5 cells
	std::map<int, std::array<GridCell, NUM_LANES>> m_cells;

	// GL objects for rendering grid quads
	GLuint m_gridVAO, m_gridVBO;
};
