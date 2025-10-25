#pragma once
#include "Bullet.h"


const int NUM_BULLETS = 20;

class Grenade
{
private:
	Bullet* bullets[NUM_BULLETS];
	double x, y;
	bool isExploded;
	Soldier* shooter;
public:
	Grenade(double xPos, double yPos, Soldier* soldier);
	void Explode(int map[MSZ][MSZ]);
	void setIsExploded(bool value);
	Soldier* getShooter() { return shooter; }
	void Show();
	void UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	Bullet** getBullets() { return bullets; }
};

