#pragma once
#include "Definitions.h"
#include "Soldier.h"
const double B_SPEED = 0.07;
const double SECURITY = 0.003;
const double B_RANGE = 7.0;

class Bullet
{
private:
	double x, y;
	bool isMoving;
	double dirX, dirY;
	double originX, originY;
	Soldier* shooter;
public:
	Bullet(double xPos, double yPos, double angle, Soldier* Shooter);
	void Move(int map[MSZ][MSZ]);
	void Show();
	void setIsMoving(bool value) { isMoving = value; }
	bool getIsMoving() { return isMoving; }
	Soldier* getShooter() { return shooter; }
	void UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	double getX() const { return x; }
	double getY() const { return y; }
};

