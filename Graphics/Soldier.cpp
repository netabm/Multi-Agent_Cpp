#include "Soldier.h"
#include <string>
#include "glut.h"

Soldier::Soldier(double start_x, double start_y, int team_id) {
	x = start_x;
	y = start_y;
	team = team_id;
	health = 100;
	isAlive = true;
	isMoving = false;

	// Initialize per-soldier visibility map
	for (int i = 0; i < MSZ; ++i) {
		for (int j = 0; j < MSZ; ++j) {
			visibility[i][j] = false;
		}
	}
}

void Soldier::Show() {
	if (!isAlive) return;

	// --- Draw Soldier ---
	glColor3d(0.2, 0.2, 0.8); // Example soldier color
	glBegin(GL_POLYGON);
	glVertex2d(x, y);
	glVertex2d(x + 1.0, y);
	glVertex2d(x + 1.0, y + 1.0);
	glVertex2d(x, y + 1.0);
	glEnd();

	// --- Draw Health Bar ---
	double healthPercentage = (double)health / 100.0; // Assuming max health is 100
	if (healthPercentage < 0) healthPercentage = 0;

	double barY = y + 1.2;       // Slightly above the soldier
	double barHeight = 0.15;
	double barWidth = 1.0;       // Same width as soldier square

	// Background (dark red)
	glColor3d(0.5, 0.1, 0.1);
	glBegin(GL_POLYGON);
	glVertex2d(x , barY);
	glVertex2d(x + barWidth, barY);
	glVertex2d(x + barWidth, barY + barHeight);
	glVertex2d(x, barY + barHeight);
	glEnd();

	// Foreground (green) proportional to health
	glColor3d(0.1, 0.8, 0.1);
	glBegin(GL_POLYGON);
	glVertex2d(x, barY);
	glVertex2d(x + (barWidth * healthPercentage), barY);
	glVertex2d(x + (barWidth * healthPercentage), barY + barHeight);
	glVertex2d(x, barY + barHeight);
	glEnd();

	// --- Draw Numerical Health ---
	glColor3d(0, 0, 0); // Black text
	std::string healthText = std::to_string(health);
	glRasterPos2d(x, barY + barHeight + 0.1); // Slightly above the bar
	for (char c : healthText) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
	}
}


void Soldier::SetTarget(int target_x, int target_y, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
	Point2D start = { (int)x, (int)y };
	Point2D target = { target_x, target_y };

	// Don't pathfind if already at target
	if (start.x == target.x && start.y == target.y) {
		path.clear();
		isMoving = false;
		return;
	}

	path = AStar(start, target, map, securityMap);
	if (!path.empty()) {
		isMoving = true;
	}
	else {
		isMoving = false;
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

	// Check if already at destination
	if (sqrt(pow(dest_x - x, 2) + pow(dest_y - y, 2)) < SPEED * 0.5) {
		x = dest_x;  // Snap to exact position
		y = dest_y;
		path.pop_back();
		if (path.empty()) {
			isMoving = false;
		}
		return;  // Important: return after reaching waypoint
	}

	double dirX = dest_x - x;
	double dirY = dest_y - y;
	double mag = sqrt(dirX * dirX + dirY * dirY);

	if (mag > 0) {
		dirX /= mag;
		dirY /= mag;
	}

	x += dirX * SPEED;
	y += dirY * SPEED;
}

bool Soldier::isNear(const Point2D& cell, double threshold) const {
	double dx = (cell.x + 0.5) - x;
	double dy = (cell.y + 0.5) - y;
	return sqrt(dx * dx + dy * dy) < threshold;
}

void Soldier::TakeDamage() {
	health -= DAMAGE;
	if (health <= 0) {
		health = 0;
		isAlive = false;
	}
}