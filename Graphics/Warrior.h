#pragma once
#include <vector>
#include "Grenade.h"
#include "visibility.h"

const int W_NUM_BULLETS = 20;
const int W_NUM_GRENADES = 3;
const int WARRIOR_SHOOT_COOLDOWN_FRAMES = 180;
const int WARRIOR_GRENADE_COOLDOWN_FRAMES = 500;
const int SCAN_TIMEOUT_FRAMES = 120;

class Commander;

enum WarriorInternalState {
	STATE_IDLE,
	STATE_MOVING,
	STATE_SCANNING_AREA,
	STATE_ENGAGING,
	STATE_FINDING_COVER
};

class Warrior : public Soldier {
	Commander* commander;
private:
	int bullet_ammo;
	int grenade_ammo;
	CommandType currentCommand;
	Point2D targetLocation;
	Soldier* currentTargetEnemy;
	WarriorInternalState currentInternalState;
	int shoot_cooldown_timer = 0;
	int grenade_cooldown_timer = 0;
	int grenade_decision_timer = 0;
	int scan_timer = 0;
	int lastHealthInCover = -1;
	bool hasDefensePosition = false;
	Point2D defensePosition = { -1, -1 };

	int cover_safe_timer = 0;
	static const int COVER_SAFE_TIMEOUT_FRAMES = 600; // frames to wait before abandoning cover
	static const int COVER_HEALTH_RELEASE = 30;       // health threshold above which warrior will leave cover

public:
	Warrior(double start_x, double start_y, int team_id, Commander* cmd);

	void Show();
	Bullet* Shoot(Soldier* target);
	Grenade* ThrowGrenade(Soldier* target);
	int getAmmo() const { return bullet_ammo; };

	void reloadAmmo() {
		bullet_ammo = W_NUM_BULLETS;
		grenade_ammo = W_NUM_GRENADES;
	};

	WarriorInternalState getCurrentInternalState() const { return currentInternalState; }
	Point2D CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);
	Soldier* FindClosestVisibleEnemy(const std::vector<Soldier*>& all_soldiers);
	bool CheckFirePath(double startX, double startY, double targetX, double targetY, int map[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);	bool IsCellOccupied(int x, int y, const std::vector<Soldier*>& all_soldiers);

	void SetCommand(CommandType cmd, Point2D location);

	void Update(const std::vector<Soldier*>& all_soldiers, std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);

	void HandleMoving(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void HandleScanning(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void HandleEngaging(std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);
	void HandleFindingCover(int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers, std::vector<Bullet*>& active_bullets);

};