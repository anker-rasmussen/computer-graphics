#pragma once

#include "Common.h"
#include "TacticalGrid.h"
#include <vector>

class CCatmullRom;
class CCamera;
class CShaderProgram;
class CFreeTypeFont;
class CShip;
class COpenAssetImportMesh;
class CAsteroid;
class CParticleSystem;
class CAudio;
class CTexture;

// -----------------------------------------------------------------------
// Turn phase state machine
// -----------------------------------------------------------------------
enum class TurnPhase {
	Planning,
	Animating,
	EnemyTurn,
	NextTurn
};

// -----------------------------------------------------------------------
// Planned move: free movement in any cardinal direction
// -----------------------------------------------------------------------
struct PlannedMove {
	int dRow;  // -1, 0, or +1
	int dLane; // -1, 0, or +1
};

// -----------------------------------------------------------------------
// Scenery asteroid instance (non-interactive, floating around the track)
// -----------------------------------------------------------------------
struct SceneryAsteroid {
	glm::vec3 position;
	float radius;
	unsigned int seed;
	int meshIndex; // index into shared mesh pool
};

// -----------------------------------------------------------------------
// Main tactical gameplay controller
// -----------------------------------------------------------------------
class CTacticalGame
{
public:
	CTacticalGame();
	~CTacticalGame();

	void Init(CCatmullRom *pSpline, CShip *pShip,
	          COpenAssetImportMesh *pCruiserMesh, COpenAssetImportMesh *pWarmindMesh,
	          COpenAssetImportMesh *pMissileMesh,
	          CParticleSystem *pParticles, CAudio *pAudio,
	          CTexture *pPortraitPerhonen, GLuint dialogueVAO);
	void StartEscape();

	// Called each frame from Game::Update
	void Update(float dt);

	// Called each frame from Game::Render
	// Sets up its own shader uniforms; caller should have cleared + set viewport
	void Render(CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
	            CFreeTypeFont *pFont, int screenWidth, int screenHeight);

	// Input from Game::KeyCallback
	void OnKeyPress(int key);

	bool IsGameOver() const { return m_gameOver; }
	bool HasEscaped() const { return m_escaped; }

	// Expose camera for Game to set before render
	void UpdateCamera(CCamera *pCamera);

private:
	// References (not owned)
	CCatmullRom *m_pSpline;
	CShip *m_pShip;
	COpenAssetImportMesh *m_pCruiserMesh;
	COpenAssetImportMesh *m_pWarmindMesh;
	CParticleSystem *m_pParticles;
	CAudio *m_pAudio;
	CTexture *m_pPortraitPerhonen;
	GLuint m_dialogueVAO;

	// Grid
	CTacticalGrid m_grid;

	// Turn state
	TurnPhase m_phase;
	int m_turnNumber;

	// Player state
	int m_playerRow;
	int m_playerLane;
	int m_fuel;
	int m_health;
	int m_fuelCollected; // counts to 3, then restores 1 HP
	static const int MAX_FUEL = 10;
	static const int MAX_HEALTH = 3;
	static const int FUEL_PER_TURN = 3;

	// Planning
	std::vector<PlannedMove> m_plannedMoves;
	int m_planCursorRow;
	int m_planCursorLane;

	// Confirmed path — persists through animation for rendering
	std::vector<std::pair<int,int>> m_confirmedPath;
	std::vector<glm::vec3> m_confirmedPathWorldPos; // precomputed world positions

	// Catmull-Rom interpolation along the confirmed path
	glm::vec3 SamplePathCatmullRom(float globalT) const;

	// Animation
	float m_animTimer;
	float m_animStepDuration;
	int m_animStep;
	glm::vec3 m_animFrom, m_animTo;

	// Advancing danger — lowest safe row. Rows below this are red/deadly.
	int m_dangerRow; // all rows <= m_dangerRow are consumed

	// Enemy fire — danger cells placed at start of planning, projectiles on confirm
	struct DangerTarget {
		int row, lane;
		bool isMissile; // true = warmind missile, false = cruiser laser
	};
	std::vector<DangerTarget> m_dangerTargets;
	void PlaceDangerCells();
	void CreateProjectiles();

	// Game state
	bool m_gameOver;
	bool m_escaped;

	// Scenery asteroids
	std::vector<SceneryAsteroid> m_sceneryAsteroids;
	int m_sceneryGeneratedUpTo; // last row we generated scenery for
	std::vector<CAsteroid*> m_asteroidMeshPool; // shared meshes (different seeds)
	static const int ASTEROID_MESH_POOL_SIZE = 8;

	// Grid obstacle asteroid meshes (keyed by seed)
	std::map<int, CAsteroid*> m_obstacleAsteroidMeshes;

	// Ship visual state (for rendering)
	float m_shipCharge;
	float m_sailUnfurl;

	// Danger row popup ("fire control patterns calculated")
	float m_dangerPopupTimer; // counts down from ~3s when danger row advances

	// Screen shake
	float m_screenShake; // 0-1 intensity, decays over time

	// Projectile visuals during animation
	COpenAssetImportMesh *m_pMissileMesh;
	struct Projectile {
		glm::vec3 from;
		glm::vec3 to;
		float fireTime;       // seconds after animation starts
		float flightDuration; // how long the flight takes
		bool isMissile;       // true = missile mesh, false = laser line
		bool landed;          // true once explosion has been spawned
	};
	std::vector<Projectile> m_activeProjectiles;

	// Internal methods
	void BeginPlanningPhase();
	void ExecutePlan();
	void AnimateStep(float dt);
	void AdvanceDangerRow();
	void AdvanceToNextTurn(float dt);

	void GenerateSceneryAsteroids(int fromRow, int toRow);

	CAsteroid* GetOrCreateObstacleMesh(int seed);

	// Rendering helpers
	void RenderShipAt(glm::vec3 pos, glm::vec3 T, glm::vec3 N, glm::vec3 B,
	                  CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
	                  glm::mat4 viewMatrix);
	void RenderProjectiles(CCamera *pCamera, CShaderProgram *pMainProgram,
	                       glm::mat4 viewMatrix);
	void RenderAsteroids(CCamera *pCamera, CShaderProgram *pMainProgram,
	                     glm::mat4 viewMatrix);
	void RenderHUD(CCamera *pCamera, std::vector<CShaderProgram*> *pShaderPrograms,
	               CFreeTypeFont *pFont, int screenWidth, int screenHeight);

	// Build planned path as (row, lane) pairs for grid rendering
	std::vector<std::pair<int,int>> GetPlannedPathCells() const;
};
