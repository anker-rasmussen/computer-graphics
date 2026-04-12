#include "Common.h"
#include "TacticalGame.h"
#include "CatmullRom.h"
#include "Camera.h"
#include "Shaders.h"
#include "FreeTypeFont.h"
#include "Ship.h"
#include "OpenAssetImportMesh.h"
#include "Asteroid.h"
#include "ParticleSystem.h"
#include "Audio.h"
#include "Texture.h"
#include "MatrixStack.h"

#include <cstdio>
#include <algorithm>

CTacticalGame::CTacticalGame()
	: m_pSpline(NULL), m_pShip(NULL), m_pCruiserMesh(NULL), m_pWarmindMesh(NULL),
	  m_pParticles(NULL), m_pAudio(NULL),
	  m_phase(TurnPhase::Planning), m_turnNumber(0),
	  m_playerRow(0), m_playerLane(5), m_fuel(5), m_health(3), m_fuelCollected(0),
	  m_planCursorRow(0), m_planCursorLane(5),
	  m_animTimer(0), m_animStepDuration(0.3f), m_animStep(0),
	  m_dangerRow(-1),
	  m_gameOver(false), m_escaped(false),
	  m_sceneryGeneratedUpTo(-1),
	  m_shipCharge(0.5f), m_sailUnfurl(0.0f),
	  m_dangerPopupTimer(0.0f), m_screenShake(0.0f),
	  m_pPortraitPerhonen(NULL), m_dialogueVAO(0),
	  m_pMissileMesh(NULL)
{}

CTacticalGame::~CTacticalGame()
{
	for (auto *a : m_asteroidMeshPool) { a->Release(); delete a; }
	for (auto &pair : m_obstacleAsteroidMeshes) { pair.second->Release(); delete pair.second; }
}

void CTacticalGame::Init(CCatmullRom *pSpline, CShip *pShip,
                          COpenAssetImportMesh *pCruiserMesh, COpenAssetImportMesh *pWarmindMesh,
                          COpenAssetImportMesh *pMissileMesh,
                          CParticleSystem *pParticles, CAudio *pAudio,
                          CTexture *pPortraitPerhonen, GLuint dialogueVAO)
{
	m_pSpline = pSpline;
	m_pShip = pShip;
	m_pCruiserMesh = pCruiserMesh;
	m_pWarmindMesh = pWarmindMesh;
	m_pMissileMesh = pMissileMesh;
	m_pParticles = pParticles;
	m_pAudio = pAudio;
	m_pPortraitPerhonen = pPortraitPerhonen;
	m_dialogueVAO = dialogueVAO;

	m_grid.Init(pSpline);
	m_grid.CreateGL();

	// Create shared scenery asteroid meshes
	for (int i = 0; i < ASTEROID_MESH_POOL_SIZE; i++) {
		CAsteroid *a = new CAsteroid();
		a->Create(1.0f, 0.4f, 1.5f, 2, (unsigned int)(i * 137 + 42));
		m_asteroidMeshPool.push_back(a);
	}
}

void CTacticalGame::StartEscape()
{
	m_playerRow = 0;
	m_playerLane = 5; // centre of 10 lanes
	m_fuel = 5;
	m_health = MAX_HEALTH;
	m_fuelCollected = 0;
	m_turnNumber = 0;
	m_dangerRow = -1; // no rows consumed yet
	m_gameOver = false;
	m_escaped = false;
	m_phase = TurnPhase::Planning;
	m_shipCharge = 0.5f;
	m_sailUnfurl = 0.0f;

	// Reveal initial rows
	m_grid.RevealRows(0, CTacticalGrid::VISIBLE_ROWS + 2, 12345);
	GenerateSceneryAsteroids(0, CTacticalGrid::VISIBLE_ROWS + 2);

	BeginPlanningPhase();
}

// -----------------------------------------------------------------------
// Planning phase
// -----------------------------------------------------------------------
void CTacticalGame::BeginPlanningPhase()
{
	m_phase = TurnPhase::Planning;
	m_plannedMoves.clear();
	m_confirmedPath.clear();
	m_planCursorRow = m_playerRow;
	m_planCursorLane = m_playerLane;

	// Award fuel
	if (m_turnNumber > 0) {
		m_fuel = glm::min(m_fuel + FUEL_PER_TURN, MAX_FUEL);
	}

	PlaceDangerCells();
}

void CTacticalGame::OnKeyPress(int key)
{
	if (m_gameOver) return;

	if (m_phase == TurnPhase::Planning) {
		int dRow = 0, dLane = 0;

		if (key == VK_UP) {
			dRow = 1;
		} else if (key == VK_DOWN) {
			dRow = -1;
		} else if (key == VK_LEFT) {
			dLane = -1;
		} else if (key == VK_RIGHT) {
			dLane = 1;
		} else if (key == VK_BACK) {
			if (!m_plannedMoves.empty()) {
				PlannedMove last = m_plannedMoves.back();
				m_plannedMoves.pop_back();
				m_planCursorRow -= last.dRow;
				m_planCursorLane -= last.dLane;
				m_fuel++;
			}
			return;
		} else if (key == VK_RETURN || key == VK_SPACE) {
			if (!m_plannedMoves.empty()) {
				m_pAudio->PlaySound("sensor", 0.5f);
				ExecutePlan();
			}
			return;
		} else {
			return;
		}

		int newRow = m_planCursorRow + dRow;
		int newLane = m_planCursorLane + dLane;

		// Bounds check
		if (newLane < 0 || newLane >= CTacticalGrid::NUM_LANES) return;
		if (newRow < 0 || newRow >= m_grid.GetTotalRows()) return;
		// Can't move back into consumed danger rows
		if (newRow <= m_dangerRow) return;
		if (m_fuel <= 0) return;

		// Check passability
		const GridCell &destCell = m_grid.GetCellConst(newRow, newLane);
		if (destCell.type == CellType::Asteroid && !destCell.destroyed) {
			if (m_fuel >= 2) {
				PlannedMove pm = {dRow, dLane};
				m_plannedMoves.push_back(pm);
				m_planCursorRow = newRow;
				m_planCursorLane = newLane;
				m_fuel -= 2;
			}
		} else {
			PlannedMove pm = {dRow, dLane};
			m_plannedMoves.push_back(pm);
			m_planCursorRow = newRow;
			m_planCursorLane = newLane;
			m_fuel--;
		}
	}
}

std::vector<std::pair<int,int>> CTacticalGame::GetPlannedPathCells() const
{
	std::vector<std::pair<int,int>> path;
	int r = m_playerRow;
	int l = m_playerLane;
	path.push_back({r, l});

	for (auto &pm : m_plannedMoves) {
		r += pm.dRow;
		l += pm.dLane;
		path.push_back({r, l});
	}
	return path;
}

// Catmull-Rom interpolation along the confirmed path world positions.
// globalT ranges from 0 (start of path) to numMoves (end of path).
glm::vec3 CTacticalGame::SamplePathCatmullRom(float globalT) const
{
	int n = (int)m_confirmedPathWorldPos.size();
	if (n == 0) return glm::vec3(0);
	if (n == 1) return m_confirmedPathWorldPos[0];

	// Clamp
	globalT = glm::clamp(globalT, 0.0f, (float)(n - 1));

	int seg = (int)globalT;
	float t = globalT - seg;

	// Catmull-Rom needs p0, p1, p2, p3 around the segment
	int i0 = glm::clamp(seg - 1, 0, n - 1);
	int i1 = glm::clamp(seg,     0, n - 1);
	int i2 = glm::clamp(seg + 1, 0, n - 1);
	int i3 = glm::clamp(seg + 2, 0, n - 1);

	glm::vec3 p0 = m_confirmedPathWorldPos[i0];
	glm::vec3 p1 = m_confirmedPathWorldPos[i1];
	glm::vec3 p2 = m_confirmedPathWorldPos[i2];
	glm::vec3 p3 = m_confirmedPathWorldPos[i3];

	// Standard Catmull-Rom formula
	float t2 = t * t;
	float t3 = t2 * t;
	return 0.5f * ((-p0 + 3.0f*p1 - 3.0f*p2 + p3) * t3
	             + (2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3) * t2
	             + (-p0 + p2) * t
	             + 2.0f*p1);
}

// -----------------------------------------------------------------------
// Animation phase
// -----------------------------------------------------------------------
void CTacticalGame::ExecutePlan()
{
	m_phase = TurnPhase::Animating;
	m_animStep = 0;
	m_animTimer = 0.0f;

	// Snapshot the full planned path and precompute world positions
	m_confirmedPath = GetPlannedPathCells();
	m_confirmedPathWorldPos.clear();
	for (auto &cell : m_confirmedPath)
		m_confirmedPathWorldPos.push_back(m_grid.GetCellWorldPos(cell.first, cell.second));

	// Create projectiles aimed at pre-placed danger cells
	CreateProjectiles();
}

void CTacticalGame::AnimateStep(float dt)
{
	int numMoves = (int)m_plannedMoves.size();
	float totalDuration = numMoves * m_animStepDuration;

	m_animTimer += dt;
	float globalT = (m_animTimer / totalDuration) * (float)numMoves;

	// Check which cells the ship has passed through since last frame
	int currentStep = glm::min((int)globalT, numMoves - 1);
	while (m_animStep <= currentStep && m_animStep < numMoves) {
		// Apply this move
		m_playerRow += m_plannedMoves[m_animStep].dRow;
		m_playerLane += m_plannedMoves[m_animStep].dLane;

		// Check cell effects
		GridCell &cell = m_grid.GetCell(m_playerRow, m_playerLane);
		if (cell.type == CellType::Fuel) {
			m_fuel = glm::min(m_fuel + 1, MAX_FUEL);
			m_fuelCollected++;
			if (m_fuelCollected >= 3 && m_health < MAX_HEALTH) {
				m_fuelCollected = 0;
				m_health++;
			}
			cell.type = CellType::Empty;
			m_pAudio->PlaySound("sensor", 0.4f);
		} else if (cell.type == CellType::Asteroid && !cell.destroyed) {
			cell.destroyed = true;
			m_pAudio->PlaySound("hit", 0.6f);
		} else if (cell.type == CellType::Danger) {
			m_health--;
			m_pAudio->PlaySound("warpin", 0.9f);
			m_screenShake = 1.0f;
			if (m_pParticles) {
				glm::vec3 hitPos = m_grid.GetCellWorldPos(m_playerRow, m_playerLane);
				glm::vec3 bloodRed(0.8f, 0.05f, 0.02f);
				glm::vec3 darkRed(0.5f, 0.0f, 0.0f);
				glm::vec3 orange(1.0f, 0.4f, 0.05f);
				// Core burst
				m_pParticles->Spawn(hitPos, 120, bloodRed, 4.0f, 12.0f, 10.0f, 35.0f, 0.8f, 2.0f);
				// Outer shrapnel
				m_pParticles->Spawn(hitPos, 80, darkRed, 2.0f, 8.0f, 20.0f, 60.0f, 0.4f, 1.2f);
				// Hot core flash
				m_pParticles->Spawn(hitPos, 40, orange, 6.0f, 15.0f, 5.0f, 15.0f, 0.2f, 0.6f);
			}
			cell.type = CellType::Empty;
			if (m_health <= 0) {
				m_gameOver = true;
				m_escaped = false;
				m_pAudio->PlaySound("loss", 0.8f);
				return;
			}
		}

		if (m_playerRow <= m_dangerRow) {
			m_gameOver = true;
			m_escaped = false;
			m_pAudio->PlaySound("loss", 0.8f);
			return;
		}
		if (m_playerRow >= m_grid.GetTotalRows() - 1) {
			m_gameOver = true;
			m_escaped = true;
			return;
		}

		m_animStep++;
	}

	// Check if any projectiles just landed — spawn explosions
	for (auto &proj : m_activeProjectiles) {
		if (proj.landed) continue;
		float elapsed = m_animTimer - proj.fireTime;
		if (elapsed >= proj.flightDuration) {
			proj.landed = true;
			if (m_pParticles) {
				glm::vec3 orange(1.0f, 0.4f, 0.05f);
				glm::vec3 darkRed(0.5f, 0.0f, 0.0f);
				if (proj.isMissile) {
					// Big missile explosion
					glm::vec3 bloodRed(0.8f, 0.05f, 0.02f);
					m_pParticles->Spawn(proj.to, 100, bloodRed, 4.0f, 12.0f, 10.0f, 35.0f, 0.8f, 2.0f);
					m_pParticles->Spawn(proj.to, 60, darkRed, 2.0f, 8.0f, 20.0f, 60.0f, 0.4f, 1.2f);
					m_pParticles->Spawn(proj.to, 30, orange, 6.0f, 15.0f, 5.0f, 15.0f, 0.2f, 0.6f);
					m_screenShake = 0.5f;
				} else {
					// Laser impact
					m_pParticles->Spawn(proj.to, 40, orange, 2.0f, 6.0f, 8.0f, 25.0f, 0.3f, 0.8f);
					m_pParticles->Spawn(proj.to, 20, darkRed, 1.0f, 4.0f, 12.0f, 35.0f, 0.2f, 0.6f);
				}
			}
			m_pAudio->PlaySound("hit", 0.5f);
		}
	}

	// Animation complete — wait for all projectiles to land too
	bool allLanded = true;
	for (auto &proj : m_activeProjectiles) {
		if (!proj.landed) { allLanded = false; break; }
	}
	if (m_animTimer >= totalDuration && allLanded) {
		m_phase = TurnPhase::EnemyTurn;
		m_animTimer = 0.0f;
	}
}

// -----------------------------------------------------------------------
// Place danger cells on the grid at start of planning (visible as red squares)
// -----------------------------------------------------------------------
void CTacticalGame::PlaceDangerCells()
{
	m_dangerTargets.clear();

	unsigned int hash = m_turnNumber * 2654435761u;

	// Warmind fires a + pattern around a predicted position ahead of the player
	{
		int targetRow = m_playerRow + 2 + (int)(hash % 4);
		int targetLane = glm::clamp(m_playerLane + (int)(hash % 3) - 1,
		                            1, CTacticalGrid::NUM_LANES - 2);
		targetRow = glm::clamp(targetRow, m_dangerRow + 1, m_grid.GetTotalRows() - 1);

		int offsets[][2] = {{0,0}, {1,0}, {0,1}, {-1,0}, {0,-1}};
		for (auto &off : offsets) {
			int r = targetRow + off[0];
			int l = targetLane + off[1];
			if (r <= m_dangerRow || r >= m_grid.GetTotalRows()) continue;
			if (l < 0 || l >= CTacticalGrid::NUM_LANES) continue;
			GridCell &cell = m_grid.GetCell(r, l);
			if (cell.type == CellType::Empty || cell.type == CellType::Fuel) {
				cell.type = CellType::Danger;
				cell.danger.turnsUntilImpact = 1;
			}
		}
		m_dangerTargets.push_back({targetRow, targetLane, true});
	}

	// 3 guberniya cruisers each fire 1 cell
	for (int i = 0; i < 3; i++) {
		unsigned int h = hash ^ ((i + 1) * 1013904223u);
		int targetRow = m_playerRow + 2 + (int)(h % 4);
		int targetLane = glm::clamp(m_playerLane + (int)((h >> 8) % 5) - 2,
		                            0, CTacticalGrid::NUM_LANES - 1);
		targetRow = glm::clamp(targetRow, m_dangerRow + 1, m_grid.GetTotalRows() - 1);

		GridCell &cell = m_grid.GetCell(targetRow, targetLane);
		if (cell.type == CellType::Empty || cell.type == CellType::Fuel) {
			cell.type = CellType::Danger;
			cell.danger.turnsUntilImpact = 1;
		}
		m_dangerTargets.push_back({targetRow, targetLane, false});
	}
}

// Create projectiles aimed at the pre-placed danger cells (called on confirm)
// -----------------------------------------------------------------------
void CTacticalGame::CreateProjectiles()
{
	m_activeProjectiles.clear();

	glm::vec3 behindPos, bT, bN, bB;
	float behindDist = glm::max((m_playerRow - 8) * (float)CTacticalGrid::SEGMENT_LENGTH, 0.0f);
	m_pSpline->SampleTNB(behindDist, behindPos, bT, bN, bB);

	int numMoves = (int)m_plannedMoves.size();
	float timePerTile = m_animStepDuration;

	int cruiserIndex = 0;
	for (auto &dt : m_dangerTargets) {
		int tileDist = glm::abs(dt.row - m_playerRow) + glm::abs(dt.lane - m_playerLane);
		float fireTime = (float)glm::min(tileDist, numMoves) * timePerTile;

		glm::vec3 target = m_grid.GetCellWorldPos(dt.row, dt.lane);
		glm::vec3 from;
		float flight;
		if (dt.isMissile) {
			from = behindPos + bB * 5.0f;
			flight = 0.5f;
		} else {
			from = behindPos + bN * (float)(cruiserIndex - 1) * 15.0f + bB * 3.0f;
			flight = 0.3f;
			cruiserIndex++;
		}
		m_activeProjectiles.push_back({from, target, fireTime, flight, dt.isMissile, false});
	}
}

void CTacticalGame::AdvanceDangerRow()
{
	int dist = m_playerRow - m_dangerRow;
	m_dangerRow += (dist > 5) ? 2 : 1;
	m_dangerPopupTimer = 3.0f; // show Perhonen popup for 3 seconds
	m_pAudio->PlaySound("fire1", 0.5f);

	// If player is on or below the new danger row, game over
	if (m_playerRow <= m_dangerRow) {
		m_gameOver = true;
		m_escaped = false;
		m_pAudio->PlaySound("loss", 0.8f);
	}
}

// -----------------------------------------------------------------------
// Next turn
// -----------------------------------------------------------------------
void CTacticalGame::AdvanceToNextTurn(float dt)
{
	m_animTimer += dt;

	if (m_animTimer >= 0.3f) {
		m_turnNumber++;

		// Reveal new rows ahead
		int revealEnd = m_playerRow + CTacticalGrid::VISIBLE_ROWS + 2;
		m_grid.RevealRows(m_playerRow, revealEnd, 12345 + m_turnNumber);
		GenerateSceneryAsteroids(m_playerRow, revealEnd);

		BeginPlanningPhase();
	}
}

// -----------------------------------------------------------------------
// Scenery asteroids
// -----------------------------------------------------------------------
void CTacticalGame::GenerateSceneryAsteroids(int fromRow, int toRow)
{
	if (fromRow <= m_sceneryGeneratedUpTo)
		fromRow = m_sceneryGeneratedUpTo + 1;

	for (int row = fromRow; row <= toRow; row++) {
		unsigned int hash = row * 2654435761u + 999;
		int count = 3 + (hash % 5); // 3-7 asteroids per row

		for (int i = 0; i < count; i++) {
			unsigned int h = hash ^ (i * 1013904223u);
			SceneryAsteroid sa;

			// Position: offset from track centre, at larger distances
			float distance = row * (float)CTacticalGrid::SEGMENT_LENGTH;
			glm::vec3 pos, T, N, B;
			m_pSpline->SampleTNB(distance, pos, T, N, B);

			float lateralDist = 40.0f + (float)(h % 120);
			float side = (h & 1) ? 1.0f : -1.0f;
			float vertOffset = -10.0f + (float)(h % 20);

			sa.position = pos + N * (lateralDist * side) + B * vertOffset;
			sa.radius = 2.0f + (float)(h % 8);
			sa.seed = h;
			sa.meshIndex = (int)(h % ASTEROID_MESH_POOL_SIZE);
			m_sceneryAsteroids.push_back(sa);
		}
	}

	m_sceneryGeneratedUpTo = glm::max(m_sceneryGeneratedUpTo, toRow);
}

CAsteroid* CTacticalGame::GetOrCreateObstacleMesh(int seed)
{
	auto it = m_obstacleAsteroidMeshes.find(seed);
	if (it != m_obstacleAsteroidMeshes.end()) return it->second;

	CAsteroid *a = new CAsteroid();
	a->Create(1.0f, 0.4f, 1.8f, 2, (unsigned int)seed);
	m_obstacleAsteroidMeshes[seed] = a;
	return a;
}

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void CTacticalGame::Update(float dt)
{
	if (m_gameOver) return;

	switch (m_phase) {
	case TurnPhase::Planning:
		// Input handled via OnKeyPress
		// Retract sails during planning (cruise mode visual)
		m_sailUnfurl = glm::min(m_sailUnfurl + 0.5f * dt, 1.0f);
		m_shipCharge = glm::min(m_shipCharge + 0.3f * dt, 0.8f);
		break;

	case TurnPhase::Animating:
		AnimateStep(dt);
		m_sailUnfurl = glm::max(m_sailUnfurl - 0.8f * dt, 0.0f);
		m_shipCharge = glm::min(m_shipCharge + 0.3f * dt, 0.8f);
		break;

	case TurnPhase::EnemyTurn:
		// Check if player ended on a danger cell, advance danger row, then next turn
		m_animTimer += dt;
		if (m_animTimer >= 0.3f) {
			// Player takes damage if they ended their move on a danger cell
			{
				const GridCell &pc = m_grid.GetCellConst(m_playerRow, m_playerLane);
				if (pc.type == CellType::Danger) {
					m_health--;
					m_pAudio->PlaySound("warpin", 0.9f);
					m_screenShake = 1.0f;
					if (m_pParticles) {
						glm::vec3 hitPos = m_grid.GetCellWorldPos(m_playerRow, m_playerLane);
						glm::vec3 bloodRed(0.8f, 0.05f, 0.02f);
						glm::vec3 darkRed(0.5f, 0.0f, 0.0f);
						glm::vec3 orange(1.0f, 0.4f, 0.05f);
						m_pParticles->Spawn(hitPos, 120, bloodRed, 4.0f, 12.0f, 10.0f, 35.0f, 0.8f, 2.0f);
						m_pParticles->Spawn(hitPos, 80, darkRed, 2.0f, 8.0f, 20.0f, 60.0f, 0.4f, 1.2f);
						m_pParticles->Spawn(hitPos, 40, orange, 6.0f, 15.0f, 5.0f, 15.0f, 0.2f, 0.6f);
					}
					m_grid.GetCell(m_playerRow, m_playerLane).type = CellType::Empty;
					if (m_health <= 0) {
						m_gameOver = true;
						m_escaped = false;
						m_pAudio->PlaySound("loss", 0.8f);
						break;
					}
				}
			}
			// Clear old danger cells now that animation is done
			m_grid.TickDangers();
			m_grid.ClearExpiredDangers();
			AdvanceDangerRow();
			m_phase = TurnPhase::NextTurn;
			m_animTimer = 0.0f;
		}
		break;

	case TurnPhase::NextTurn:
		AdvanceToNextTurn(dt);
		break;
	}

	if (m_dangerPopupTimer > 0.0f)
		m_dangerPopupTimer -= dt;
	if (m_screenShake > 0.0f)
		m_screenShake = glm::max(m_screenShake - dt * 1.2f, 0.0f);

	if (m_pParticles)
		m_pParticles->Update(dt);
}

// -----------------------------------------------------------------------
// Camera
// -----------------------------------------------------------------------
void CTacticalGame::UpdateCamera(CCamera *pCamera)
{
	glm::vec3 T, N, B, centrePos;
	float dist = m_playerRow * (float)CTacticalGrid::SEGMENT_LENGTH;
	m_pSpline->SampleTNB(dist, centrePos, T, N, B);

	glm::vec3 playerPos = m_grid.GetCellWorldPos(m_playerRow, m_playerLane);

	if (m_phase == TurnPhase::Planning) {
		// God's eye top-down view during planning. Ship is rendered at
		// playerPos + B * 20.0f (see Render()), so anchor the framing to that.
		glm::vec3 shipPos = playerPos + B * 20.0f;
		glm::vec3 lookAhead = m_grid.GetCellWorldPos(
			glm::min(m_playerRow + CTacticalGrid::VISIBLE_ROWS / 2, m_grid.GetTotalRows() - 1),
			CTacticalGrid::NUM_LANES / 2) + B * 20.0f;
		glm::vec3 gridCentre = (shipPos + lookAhead) * 0.5f;
		glm::vec3 camPos = gridCentre + B * 350.0f + T * 20.0f;
		pCamera->Set(camPos, gridCentre, T);
	} else if (m_phase == TurnPhase::Animating && !m_plannedMoves.empty()) {
		// Follow behind during animation — smooth Catmull-Rom path
		int numMoves = (int)m_plannedMoves.size();
		float totalDuration = numMoves * m_animStepDuration;
		float globalT = (m_animTimer / totalDuration) * (float)numMoves;
		glm::vec3 shipPos = SamplePathCatmullRom(globalT) + B * 20.0f;
		glm::vec3 camPos = shipPos - T * 60.0f + B * 25.0f;
		glm::vec3 camLookAt = shipPos + T * 30.0f;
		pCamera->Set(camPos, camLookAt, B);
	} else {
		// Default third-person
		glm::vec3 shipPos = playerPos + B * 20.0f;
		glm::vec3 camPos = shipPos - T * 45.0f + B * 20.0f;
		glm::vec3 camLookAt = shipPos + T * 30.0f;
		pCamera->Set(camPos, camLookAt, B);
	}

	// Apply screen shake
	if (m_screenShake > 0.01f) {
		float shakeAmount = m_screenShake * 12.0f;
		float time = (float)GetTickCount() / 1000.0f * 50.0f;
		glm::vec3 shakeOffset = glm::vec3(
			sinf(time * 1.7f) * shakeAmount,
			cosf(time * 2.3f) * shakeAmount,
			sinf(time * 1.1f) * shakeAmount * 0.5f);
		glm::vec3 pos = pCamera->GetPosition() + shakeOffset;
		pCamera->Set(pos, pCamera->GetView() + shakeOffset, pCamera->GetUpVector());
	}
}

// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void CTacticalGame::Render(CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
                            CFreeTypeFont *pFont, int screenWidth, int screenHeight)
{
	CShaderProgram *pMainProgram = (*pShaderPrograms)[0];
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", true);
	pMainProgram->SetUniform("sampler0", 0);
	int cubeMapUnit = 10;
	pMainProgram->SetUniform("CubeMapTex", cubeMapUnit);
	pMainProgram->SetUniform("bMirror", false);
	pMainProgram->SetUniform("alpha", 0.0f);
	pMainProgram->SetUniform("numPointLights", 0);

	pMainProgram->SetUniform("matrices.projMatrix", *pCamera->GetPerspectiveProjectionMatrix());

	glutil::MatrixStack ms;
	ms.SetIdentity();
	ms.LookAt(pCamera->GetPosition(), pCamera->GetView(), pCamera->GetUpVector());
	glm::mat4 viewMatrix = ms.Top();

	glm::vec4 lightPos(-100, 100, -100, 1);
	pMainProgram->SetUniform("light1.position", viewMatrix * lightPos);
	pMainProgram->SetUniform("light1.La", glm::vec3(0.6f));
	pMainProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
	pMainProgram->SetUniform("light1.Ls", glm::vec3(0.5f));

	glm::mat4 identBias(1.0f);

	// Track
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.3f));
	pMainProgram->SetUniform("material1.shininess", 15.0f);
	glDisable(GL_CULL_FACE);
	ms.Push();
		pMainProgram->SetUniform("matrices.modelViewMatrix", ms.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(ms.Top()));
		m_pSpline->RenderTrack();
	ms.Pop();
	glEnable(GL_CULL_FACE);

	// Grid overlay — use confirmed path during animation, live path during planning
	std::vector<std::pair<int,int>> pathCells;
	if (m_phase == TurnPhase::Animating || m_phase == TurnPhase::EnemyTurn)
		pathCells = m_confirmedPath;
	else
		pathCells = GetPlannedPathCells();
	m_grid.RenderGrid(m_playerRow, m_playerLane, m_dangerRow, pathCells,
	                  pMainProgram, pCamera, viewMatrix);

	// Ship — smooth Catmull-Rom path during animation
	glm::vec3 shipPos, T, N, B;
	if (m_phase == TurnPhase::Animating && !m_plannedMoves.empty()) {
		int numMoves = (int)m_plannedMoves.size();
		float totalDuration = numMoves * m_animStepDuration;
		float globalT = (m_animTimer / totalDuration) * (float)numMoves;
		shipPos = SamplePathCatmullRom(globalT);
		// Get TNB from a point slightly ahead on the path for orientation
		glm::vec3 ahead = SamplePathCatmullRom(globalT + 0.1f);
		T = glm::normalize(ahead - shipPos);
		if (glm::length(T) < 0.001f) T = glm::vec3(0, 0, 1);
		N = glm::normalize(glm::cross(T, glm::vec3(0, 1, 0)));
		B = glm::normalize(glm::cross(N, T));
	} else {
		m_grid.GetCellTNB(m_playerRow, m_playerLane, shipPos, T, N, B);
	}
	shipPos += B * 20.0f;
	RenderShipAt(shipPos, T, N, B, pCamera, pShaderPrograms, viewMatrix);

	// Scenery asteroids
	RenderAsteroids(pCamera, pMainProgram, viewMatrix);

	// Projectiles (lasers + missile) — visible while ship is moving
	if (m_phase == TurnPhase::Animating && !m_activeProjectiles.empty()) {
		RenderProjectiles(pCamera, pMainProgram, viewMatrix);
	}

	// Grid obstacle asteroids
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", false);
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.25f, 0.2f, 0.15f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f, 0.4f, 0.3f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.2f));
	pMainProgram->SetUniform("material1.shininess", 10.0f);

	int startRow = m_playerRow;
	int endRow = glm::min(m_playerRow + CTacticalGrid::VISIBLE_ROWS, m_grid.GetTotalRows() - 1);
	for (int row = startRow; row <= endRow; row++) {
		for (int lane = 0; lane < CTacticalGrid::NUM_LANES; lane++) {
			const GridCell &cell = m_grid.GetCellConst(row, lane);
			if (cell.type != CellType::Asteroid || cell.destroyed) continue;

			CAsteroid *mesh = const_cast<CTacticalGame*>(this)->GetOrCreateObstacleMesh(cell.asteroidSeed);
			glm::vec3 aPos, aT, aN, aB;
			m_grid.GetCellTNB(row, lane, aPos, aT, aN, aB);

			ms.Push();
				glm::mat4 model = ms.Top()
					* glm::translate(glm::mat4(1.0f), aPos)
					* glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
				pMainProgram->SetUniform("matrices.modelViewMatrix", model);
				pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(model));
				mesh->Render();
			ms.Pop();
		}
	}



	// Particles
	if (m_pParticles)
		m_pParticles->Render(viewMatrix, *pCamera->GetPerspectiveProjectionMatrix());

	// HUD (always last)
	RenderHUD(pCamera, pShaderPrograms, pFont, screenWidth, screenHeight);
}

// -----------------------------------------------------------------------
// Render the player's ship at a given position/orientation
// -----------------------------------------------------------------------
void CTacticalGame::RenderShipAt(glm::vec3 pos, glm::vec3 T, glm::vec3 N, glm::vec3 B,
                                  CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
                                  glm::mat4 viewMatrix)
{
	glutil::MatrixStack ms;
	ms.SetIdentity();
	ms.SetMatrix(viewMatrix);

	glm::mat4 orient(1.0f);
	orient[0] = glm::vec4(N, 0.0f);
	orient[1] = glm::vec4(B, 0.0f);
	orient[2] = glm::vec4(T, 0.0f);
	orient[3] = glm::vec4(pos, 1.0f);

	glm::mat4 shipMV = ms.Top() * orient * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
	glm::mat3 shipNM = pCamera->ComputeNormalMatrix(shipMV);

	glm::vec4 lightPos(-100, 100, -100, 1);

	// Hull
	CShaderProgram *pHullProgram = (*pShaderPrograms)[3];
	pHullProgram->UseProgram();
	pHullProgram->SetUniform("matrices.projMatrix", *pCamera->GetPerspectiveProjectionMatrix());
	pHullProgram->SetUniform("sampler0", 0);
	pHullProgram->SetUniform("charge", m_shipCharge);
	pHullProgram->SetUniform("light1.position", viewMatrix * lightPos);
	pHullProgram->SetUniform("light1.La", glm::vec3(0.6f));
	pHullProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
	pHullProgram->SetUniform("light1.Ls", glm::vec3(0.5f));
	pHullProgram->SetUniform("material1.Ma", glm::vec3(0.15f, 0.15f, 0.2f));
	pHullProgram->SetUniform("material1.Md", glm::vec3(0.6f, 0.6f, 0.7f));
	pHullProgram->SetUniform("material1.Ms", glm::vec3(1.0f, 1.0f, 1.2f));
	pHullProgram->SetUniform("material1.shininess", 80.0f);
	pHullProgram->SetUniform("bUseTexture", true);
	pHullProgram->SetUniform("matrices.modelViewMatrix", shipMV);
	pHullProgram->SetUniform("matrices.normalMatrix", shipNM);
	m_pShip->RenderHull();

	// Thrust
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	float exhaustZ = -7.35f;
	float thrustScale = 0.1f + 0.9f * m_shipCharge;
	glm::mat4 thrustMV = shipMV
		* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, exhaustZ))
		* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, thrustScale))
		* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -exhaustZ));
	pHullProgram->SetUniform("bUseTexture", false);
	pHullProgram->SetUniform("material1.Ma", glm::vec3(0.0f));
	float intensity = 0.15f + 0.85f * m_shipCharge;
	pHullProgram->SetUniform("material1.Md", intensity * glm::vec3(0.5f, 0.8f, 1.0f));
	pHullProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
	pHullProgram->SetUniform("matrices.modelViewMatrix", thrustMV);
	pHullProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(thrustMV));
	m_pShip->RenderThrust();

	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Sails
	if (m_sailUnfurl > 0.01f) {
		CShaderProgram *pSailProgram = (*pShaderPrograms)[2];
		pSailProgram->UseProgram();
		pSailProgram->SetUniform("matrices.projMatrix", *pCamera->GetPerspectiveProjectionMatrix());
		pSailProgram->SetUniform("sampler0", 0);
		pSailProgram->SetUniform("unfurl", m_sailUnfurl);
		pSailProgram->SetUniform("light1.position", viewMatrix * lightPos);
		pSailProgram->SetUniform("light1.La", glm::vec3(0.6f));
		pSailProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
		pSailProgram->SetUniform("light1.Ls", glm::vec3(0.5f));
		pSailProgram->SetUniform("material1.Ma", glm::vec3(0.3f));
		pSailProgram->SetUniform("material1.Md", glm::vec3(1.0f));
		pSailProgram->SetUniform("material1.Ms", glm::vec3(1.5f));
		pSailProgram->SetUniform("material1.shininess", 200.0f);
		pSailProgram->SetUniform("bUseTexture", true);
		pSailProgram->SetUniform("matrices.modelViewMatrix", shipMV);
		pSailProgram->SetUniform("matrices.normalMatrix", shipNM);
		m_pShip->RenderSails();
	}
}


// -----------------------------------------------------------------------
// Render projectiles (lasers + missile) during enemy turn
// -----------------------------------------------------------------------
void CTacticalGame::RenderProjectiles(CCamera *pCamera, CShaderProgram *pMainProgram,
                                       glm::mat4 viewMatrix)
{
	float animTime = m_animTimer; // current time into animation

	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", false);
	pMainProgram->SetUniform("alpha", 0.0f);

	struct LineVert { glm::vec3 pos; glm::vec2 uv; glm::vec3 normal; };
	std::vector<LineVert> laserVerts;
	glm::vec3 up(0, 1, 0);

	for (auto &proj : m_activeProjectiles) {
		// Has this projectile fired yet?
		float elapsed = animTime - proj.fireTime;
		if (elapsed < 0.0f) continue; // not yet
		float t = glm::clamp(elapsed / proj.flightDuration, 0.0f, 1.0f);
		if (t >= 1.0f) continue; // already arrived

		if (proj.isMissile && m_pMissileMesh) {
			// Missile mesh
			glm::vec3 missilePos = glm::mix(proj.from, proj.to, t);
			glm::vec3 missileDir = glm::normalize(proj.to - proj.from);
			glm::vec3 right = glm::normalize(glm::cross(missileDir, up));
			glm::vec3 realUp = glm::cross(right, missileDir);

			glm::mat4 orient(1.0f);
			orient[0] = glm::vec4(right, 0.0f);
			orient[1] = glm::vec4(realUp, 0.0f);
			orient[2] = glm::vec4(missileDir, 0.0f);
			orient[3] = glm::vec4(missilePos, 1.0f);

			glm::mat4 missileMV = viewMatrix * orient * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			pMainProgram->SetUniform("material1.Ma", glm::vec3(0.3f, 0.3f, 0.35f));
			pMainProgram->SetUniform("material1.Md", glm::vec3(0.6f, 0.6f, 0.65f));
			pMainProgram->SetUniform("material1.Ms", glm::vec3(0.5f));
			pMainProgram->SetUniform("material1.shininess", 40.0f);
			pMainProgram->SetUniform("matrices.modelViewMatrix", missileMV);
			pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(missileMV));
			m_pMissileMesh->Render();
		} else {
			// Laser line
			glm::vec3 tip = glm::mix(proj.from, proj.to, t);
			glm::vec3 tail = glm::mix(proj.from, proj.to, glm::max(t - 0.3f, 0.0f));
			laserVerts.push_back({tail, glm::vec2(0), up});
			laserVerts.push_back({tip, glm::vec2(0), up});
		}
	}

	// Draw all laser lines in one batch
	if (!laserVerts.empty()) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glLineWidth(5.0f);

		pMainProgram->SetUniform("material1.Ma", glm::vec3(3.0f, 0.5f, 0.2f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(4.0f, 0.8f, 0.3f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
		pMainProgram->SetUniform("matrices.modelViewMatrix", viewMatrix);
		pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(viewMatrix));

		GLuint tmpVAO, tmpVBO;
		glGenVertexArrays(1, &tmpVAO);
		glBindVertexArray(tmpVAO);
		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, laserVerts.size() * sizeof(LineVert), laserVerts.data(), GL_DYNAMIC_DRAW);

		GLsizei stride = sizeof(LineVert);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));

		glDrawArrays(GL_LINES, 0, (int)laserVerts.size());
		glDeleteBuffers(1, &tmpVBO);
		glDeleteVertexArrays(1, &tmpVAO);

		glLineWidth(1.0f);
		glDisable(GL_BLEND);
	}
}

// -----------------------------------------------------------------------
// Render scenery asteroids
// -----------------------------------------------------------------------
void CTacticalGame::RenderAsteroids(CCamera *pCamera, CShaderProgram *pMainProgram,
                                     glm::mat4 viewMatrix)
{
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", false);
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.2f, 0.18f, 0.15f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.4f, 0.35f, 0.3f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.15f));
	pMainProgram->SetUniform("material1.shininess", 8.0f);

	glutil::MatrixStack ms;
	ms.SetIdentity();
	ms.SetMatrix(viewMatrix);

	glm::vec3 camPos = pCamera->GetPosition();

	for (auto &sa : m_sceneryAsteroids) {
		float d = glm::length(sa.position - camPos);
		if (d > 800.0f) continue; // cull distant

		ms.Push();
			glm::mat4 model = ms.Top()
				* glm::translate(glm::mat4(1.0f), sa.position)
				* glm::scale(glm::mat4(1.0f), glm::vec3(sa.radius));
			pMainProgram->SetUniform("matrices.modelViewMatrix", model);
			pMainProgram->SetUniform("matrices.normalMatrix", pCamera->ComputeNormalMatrix(model));
			m_asteroidMeshPool[sa.meshIndex]->Render();
		ms.Pop();
	}
}

// -----------------------------------------------------------------------
// HUD
// -----------------------------------------------------------------------
void CTacticalGame::RenderHUD(CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
                               CFreeTypeFont *pFont, int screenWidth, int screenHeight)
{
	CShaderProgram *fontProgram = (*pShaderPrograms)[1];
	pCamera->SetOrthographicProjectionMatrix(screenWidth, screenHeight);

	fontProgram->UseProgram();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	fontProgram->SetUniform("matrices.projMatrix", *pCamera->GetOrthographicProjectionMatrix());
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));

	char buf[128];

	// Fuel bar — top left
	fontProgram->SetUniform("vColour", glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));
	sprintf(buf, "FUEL: %d/%d  [%d/3]", m_fuel, MAX_FUEL, m_fuelCollected);
	pFont->Render(20, screenHeight - 40, 24, buf);

	// Health — below fuel
	glm::vec4 hpColour = (m_health == 1)
		? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)
		: glm::vec4(0.9f, 0.9f, 0.2f, 1.0f);
	fontProgram->SetUniform("vColour", hpColour);
	// Render hearts/pips
	string hpStr = "HP: ";
	for (int i = 0; i < MAX_HEALTH; i++)
		hpStr += (i < m_health) ? "# " : "- ";
	pFont->Render(20, screenHeight - 70, 24, hpStr.c_str());

	// Turn counter — top centre
	fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	sprintf(buf, "TURN %d", m_turnNumber);
	pFont->Render(screenWidth / 2 - 40, screenHeight - 40, 24, buf);

	// Danger proximity — top right
	int dangerDist = m_playerRow - m_dangerRow;
	glm::vec4 dangerColour = (dangerDist <= 2)
		? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)
		: glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
	fontProgram->SetUniform("vColour", dangerColour);
	sprintf(buf, "SAFE: %d ROWS", dangerDist);
	pFont->Render(screenWidth - 220, screenHeight - 40, 24, buf);

	// Planning instructions
	if (m_phase == TurnPhase::Planning) {
		fontProgram->SetUniform("vColour", glm::vec4(0.7f, 0.9f, 1.0f, 1.0f));
		sprintf(buf, "MOVES: %d | ENTER confirm | BACKSPACE undo", (int)m_plannedMoves.size());
		int tw = pFont->GetTextWidth(buf, 20);
		pFont->Render(screenWidth / 2 - tw / 2, 40, 20, buf);
	}

	// Perhonen popup when danger row advances
	if (m_dangerPopupTimer > 0.0f && m_pPortraitPerhonen) {
		float popupAlpha = glm::clamp(m_dangerPopupTimer, 0.0f, 1.0f);

		// Portrait in top-right corner
		int portraitSize = 80;
		int px = screenWidth - portraitSize - 20;
		int py = screenHeight - portraitSize - 70;

		fontProgram->SetUniform("bFullColour", true);
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, popupAlpha));
		glBindVertexArray(m_dialogueVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindSampler(0, 0);
		m_pPortraitPerhonen->Bind(0);
		fontProgram->SetUniform("sampler0", 0);
		glm::mat4 portraitMV = glm::translate(glm::mat4(1.0f), glm::vec3((float)px, (float)py, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3((float)portraitSize, (float)portraitSize, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", portraitMV);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		fontProgram->SetUniform("bFullColour", false);

		// Speech bubble text
		fontProgram->SetUniform("vColour", glm::vec4(0.9f, 0.8f, 0.3f, popupAlpha));
		pFont->Render(px - 280, py + portraitSize / 2 + 8, 18,
		              "Fire control patterns calculated.");
	}

	// Game over overlay
	if (m_gameOver) {
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
		if (m_escaped)
			pFont->Render(screenWidth / 2 - 80, screenHeight / 2, 48, "ESCAPED!");
		else
			pFont->Render(screenWidth / 2 - 100, screenHeight / 2, 48, "CAPTURED");
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}
