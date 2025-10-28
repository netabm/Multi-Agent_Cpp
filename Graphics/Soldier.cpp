#include "Soldier.h"
#include "glut.h"

Soldier::Soldier(double start_x, double start_y, int team_id) {
	x = start_x;
	y = start_y;
	team = team_id;
	health = 100;
	isAlive = true;
	isMoving = false;
}

void Soldier::Show() {
	if (!isAlive) return;

	// --- Draw Health Bar ---
	double healthPercentage = (double)health / 100.0; // Assuming max health is 100
	if (healthPercentage < 0) healthPercentage = 0;

	// Bar position (slightly above the soldier's square)
	double barY = y + 1.1; // Adjust Y offset as needed
	double barHeight = 0.15; // Adjust height as needed
	double barWidth = 1.0;   // Same width as the soldier square

	// Draw background (e.g., dark red)
	glColor3d(0.5, 0.1, 0.1);
	glBegin(GL_POLYGON); // Use GL_RECTANGLE for simple filled rect
	glVertex2d(x, barY);
	glVertex2d(x + barWidth, barY);
	glVertex2d(x + barWidth, barY + barHeight);
	glVertex2d(x, barY + barHeight);
	glEnd();

	// Draw foreground (e.g., green) based on health percentage
	glColor3d(0.1, 0.8, 0.1);
	glBegin(GL_POLYGON);
	glVertex2d(x, barY);
	glVertex2d(x + (barWidth * healthPercentage), barY); // Width depends on health
	glVertex2d(x + (barWidth * healthPercentage), barY + barHeight);
	glVertex2d(x, barY + barHeight);
	glEnd();
	// ----------------------
}

void Soldier::SetTarget(int target_x, int target_y, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
	Point2D start = { (int)x, (int)y };
	Point2D target = { target_x, target_y };
	path = AStar(start, target, map, securityMap);
	if (!path.empty()) {
		isMoving = true;
	}
}

void Soldier::Move(int map[MSZ][MSZ]) {
	if (!isAlive) return;

	if (!isMoving || path.empty()) {
		isMoving = false;
		return;
	}
	Point2D waypoint = path.back();
	double dest_x = waypoint.x + 0.5;
	double dest_y = waypoint.y + 0.5;
	double dirX = dest_x - x;
	double dirY = dest_y - y;
	double mag = sqrt(dirX * dirX + dirY * dirY);
	if (mag > 0) {
		dirX /= mag;
		dirY /= mag;
	}
	x += dirX * SPEED;
	y += dirY * SPEED;
	if (sqrt(pow(dest_x - x, 2) + pow(dest_y - y, 2)) < SPEED) {
		path.pop_back();
		if (path.empty()) {
			isMoving = false;
		}
	}
}

bool Soldier::isNear(const Point2D& cell, double threshold) const {
	double dx = (cell.x + 0.5) - x;
	double dy = (cell.y + 0.5) - y;
	return sqrt(dx * dx + dy * dy) < threshold;
}

void Soldier::TakeDamage() {
	health -= DAMAGE;
	if (health < 0) {
		health = 0;
		isAlive = false;
	}
}