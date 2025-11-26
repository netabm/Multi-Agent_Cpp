#pragma once
#include "Pathfinding.h"
#include "visibility.h"

class Soldier {
protected:
	double x, y;
	double dirX, dirY;
	int team;
	int health;
	bool isAlive;
	bool isMoving;
	std::vector<Point2D> path;

public:
	bool visibility[MSZ][MSZ];
	Soldier(double start_x, double start_y, int team_id);

	void Move(int map[MSZ][MSZ]);
	bool isNear(const Point2D& cell, double threshold = 1) const;

	void SetTarget(int target_x, int target_y, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void TakeDamage();

	virtual void Show();

	int getHealth() { return health; }
	void heal() { health = 100; }
	bool IsAlive() const { return isAlive; }
	bool getIsMoving() { return isMoving; }
	int getTeam() { return team; }
	bool(&getVisibility())[MSZ][MSZ]{ return visibility; }

	Point2D getPosition() { return { (int)x, (int)y }; }
	double getX() const { return x; }
	double getY() const { return y; }
};