#pragma once
#include "Bullet.h"

const int NUM_BULLETS = 20;
const double GRENADE_SPEED = 0.1; // How fast the grenade flies

class Grenade
{
private:
	Bullet* bullets[NUM_BULLETS];
	double x, y; // CURRENT position (while traveling)
	double target_x, target_y; // Where it's going

	bool isTraveling;
	bool isExploded;
	Soldier* shooter;

	double travel_dx; // Pre-calculated travel vector
	double travel_dy;

	// Private helper to create the explosion
	void TriggerExplosion();

public:
	// Constructor now takes start AND target positions
	Grenade(double startX, double startY, double targetX, double targetY, Soldier* soldier);
	~Grenade(); // Destructor to clean up bullets

	// Renamed from Explode to Update, as it now handles travel
	void Update(int map[MSZ][MSZ]);

	// This is for debug (like your mouse click) to make it explode instantly
	void ExplodeNow();

	Soldier* getShooter() { return shooter; }
	void Show();
	void UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	Bullet** getBullets() { return bullets; }
};
