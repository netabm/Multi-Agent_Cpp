#pragma once
#include <vector>
#include "Grenade.h"
#include "visibility.h"

const int W_NUM_BULLETS = 20;
const int W_NUM_GRENADES = 3;
const int WARRIOR_SHOOT_COOLDOWN_FRAMES = 180;
const int WARRIOR_GRENADE_COOLDOWN_FRAMES = 500;
const int SCAN_TIMEOUT_FRAMES = 120;

enum WarriorInternalState {
	STATE_IDLE,
	STATE_MOVING,
	STATE_SCANNING_AREA,
	STATE_ENGAGING,
	STATE_FINDING_COVER
};

class Warrior : public Soldier {
private:
	int bullet_ammo;
	int grenade_ammo;
	CommandType currentCommand;
	Point2D targetLocation; // For MOVE / ATTACK area
	Soldier* currentTargetEnemy; // For ENGAGING
	WarriorInternalState currentInternalState;
	int shoot_cooldown_timer = 0;
	int grenade_cooldown_timer = 0;
	int scan_timer = 0;
public:
	Warrior(double start_x, double start_y, int team_id);

	void Show();

	Bullet* Shoot(Soldier* target);
	Grenade* ThrowGrenade(Soldier* target);
	int getAmmo() const { return bullet_ammo; };
	void reloadAmmo() { bullet_ammo = W_NUM_BULLETS; };
	void heal() { health = 100; };
	WarriorInternalState getCurrentInternalState() const { return currentInternalState; }


	Point2D CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ]);
	Soldier* FindClosestVisibleEnemy(const std::vector<Soldier*>& all_soldiers);

	void SetCommand(CommandType cmd, Point2D location);
	// Fixed signature: accept the real grenades vector type
	void Update(bool is_commander_alive, const std::vector<Soldier*>& all_soldiers, std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);

	void HandleMoving(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void HandleScanning(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void HandleEngaging(std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);
	void HandleFindingCover(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);

};