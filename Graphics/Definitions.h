#pragma once

enum CommandType { IDLE, MOVE, ATTACK, DEFEND };

const double PI = 3.1415;

const int MSZ = 50;
const int STONE = 1;
const int TREE = 2;
const int WATER = 3;
const int AMMO_WAREHOUSE = 4;
const int MEDICAL_WAREHOUSE = 5;
const double SPEED = 0.02;
const int DAMAGE = 5;

const double WARRIOR_FIRING_RANGE = 7.0;
const double GRENADE_THROW_RANGE = 15.0;

struct Point2D {
	int x, y;

	bool operator==(const Point2D& other) const {
		return x == other.x && y == other.y;
	}
};


