#pragma once
#include "Bullet.h"

const int NUM_BULLETS = 20;
const double GRENADE_SPEED = 0.1; // How fast the grenade flies

class Grenade
{
private:
	Bullet* bullets[NUM_BULLETS];
	double x, y; // current position
	double target_x, target_y;

	bool isTraveling;
	bool isExploded;
	Soldier* shooter;

	double travel_dx; // travel vector
	double travel_dy;

	// Private helper to create the explosion
	void TriggerExplosion();

public:
	Grenade(double startX, double startY, double targetX, double targetY, Soldier* soldier);
	~Grenade(); // Destructor to clean up bullets

	void Update(int map[MSZ][MSZ]);

	void ExplodeNow();

	Soldier* getShooter() { return shooter; }
	void Show();
	void UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	Bullet** getBullets() { return bullets; }
};
