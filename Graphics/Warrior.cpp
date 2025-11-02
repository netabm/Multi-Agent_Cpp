#include "Warrior.h"
#include "glut.h"
#include <math.h>
#include <sstream>

Warrior::Warrior(double start_x, double start_y, int team_id) : Soldier(start_x, start_y, team_id)
{
	bullet_ammo = W_NUM_BULLETS;
	grenade_ammo = W_NUM_GRENADES;
	team = team_id;
	currentCommand = IDLE;
	targetLocation = { -1,-1 };
	currentInternalState = STATE_IDLE;
	currentTargetEnemy = nullptr;
}

void Warrior::Show()
{
	Soldier::Show();

	if (!isAlive) return;

	if (team == 0)
	{
		// purple/magenta
		glColor3d(0.8, 0.2, 0.8);
	}
	else
	{
		glColor3d(1.0, 0.5, 0.0); // Orange
	}

	// Draw the 1x1 square
	glBegin(GL_POLYGON);
	glVertex2d(x, y);
	glVertex2d(x, y + 1);
	glVertex2d(x + 1, y + 1);
	glVertex2d(x + 1, y);
	glEnd();

	// Set the color for the text
	glColor3d(0, 0, 0); // Black

	glRasterPos2d(x + 0.1, y + 0.1);

	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'W');

	// Draw Ammo Count
	glColor3d(0.1, 0.1, 0.1);
	glRasterPos2d(x - 1, y - 0.7); // Position below the square

	// Create ammo string
	std::stringstream ss;
	ss << "B:" << bullet_ammo << " G:" << grenade_ammo;
	std::string ammo_str = ss.str();

	// Draw each character of the string
	for (char c : ammo_str) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c); // Use a small font
	}

}

Bullet* Warrior::Shoot(Soldier* target)
{
	// Check ammo and target validity
	if (bullet_ammo <= 0 || target == nullptr || !target->IsAlive()) {
		return nullptr;
	}

	// Use precise coordinates for calculations
	double target_x_precise = target->getX();
	double target_y_precise = target->getY();

	// Calculate squared distance for efficiency (precise shooter to precise target)
	double dx_dist = target_x_precise - x;
	double dy_dist = target_y_precise - y;
	double dist_sq = dx_dist * dx_dist + dy_dist * dy_dist; // Use squared distance

	// Check range using squared distance
	// Add a small tolerance (e.g., half a unit squared) to prevent range issues at edge cases
	double range_sq_with_tolerance = (WARRIOR_FIRING_RANGE + 0.5) * (WARRIOR_FIRING_RANGE + 0.5);

	if (dist_sq <= range_sq_with_tolerance) {
		bullet_ammo--;

		double angle = atan2((target_y_precise + 0.5) - (y + 0.5),
			(target_x_precise + 0.5) - (x + 0.5));

		// Create bullet, passing 'this' as the shooter
		Bullet* new_bullet = new Bullet(x + 0.5, y + 0.5, angle, this); // Pass 'this'
		new_bullet->setIsMoving(true); // Activate the bullet

		return new_bullet;
	}
	return nullptr; // Target out of range
}

Grenade* Warrior::ThrowGrenade(Soldier* target)
{
	if (grenade_ammo <= 0)
		return nullptr;

	Point2D target_pos = target->getPosition();
	double distance = sqrt(pow(target_pos.x - x, 2) + pow(target_pos.y - y, 2));

	if (distance <= GRENADE_THROW_RANGE)
	{
		grenade_ammo--;
		return new Grenade(target_pos.x, target_pos.y, this);
	}

	return nullptr;
}

void Warrior::SetCommand(CommandType cmd, Point2D location) {
	if (!IsAlive()) return;

	// If the command and location are unchanged, do nothing (avoid wiping path every frame)
	if (currentCommand == cmd && targetLocation.x == location.x && targetLocation.y == location.y) {
		return;
	}

	currentCommand = cmd;
	targetLocation = location;
	currentTargetEnemy = nullptr;
	currentInternalState = (cmd == DEFEND) ? STATE_FINDING_COVER : STATE_MOVING;
	path.clear();
}

Soldier* Warrior::FindClosestVisibleEnemy(const std::vector<Soldier*>& all_soldiers) {
	Soldier* closest = nullptr;
	double minDist = 1e9;

	for (Soldier* s : all_soldiers) {
		if (s->getTeam() != team && s->IsAlive()) {
			Point2D pos = s->getPosition();
			if (visibility[pos.y][pos.x]) {
				double dist = hypot(x - s->getX(), y - s->getY());
				if (dist < minDist) {
					minDist = dist;
					closest = s;
				}
			}
		}
	}
	return closest;
}

void Warrior::Update(bool is_commander_alive, const std::vector<Soldier*>& all_soldiers, std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!IsAlive()) return;

	if (shoot_cooldown_timer > 0) {
		shoot_cooldown_timer--;
	}

	CalculateVisibility(this, map, visibility);

	// Validate currentTargetEnemy
	if (currentTargetEnemy != nullptr) {
		bool targetStillValid = false;
		for (Soldier* s : all_soldiers) {
			if (s == currentTargetEnemy && s->IsAlive()) {
				targetStillValid = true;
				break;
			}
		}
		if (!targetStillValid) {
			currentTargetEnemy = nullptr;
			if (currentInternalState == STATE_ENGAGING) {
				currentInternalState = STATE_SCANNING_AREA;
				scan_timer = 0;
			}
		}
	}

	// HIGHEST PRIORITY: Check for visible enemies EVERY frame
	Soldier* visibleEnemy = FindClosestVisibleEnemy(all_soldiers);
	if (visibleEnemy) {
		// Only switch to engaging if we're not already engaging this target
		if (currentInternalState != STATE_ENGAGING || currentTargetEnemy != visibleEnemy) {
			currentTargetEnemy = visibleEnemy;
			currentInternalState = STATE_ENGAGING;
			// Don't clear path here - let STATE_ENGAGING handle movement
		}
	}
	else {
		// NO enemies visible - clear any ATTACK commands from commander
		// This prevents warriors from going to stale enemy positions
		if (currentCommand == ATTACK) {
			currentCommand = IDLE;
			targetLocation = { -1, -1 };
		}
	}

	switch (currentInternalState) {
	case STATE_IDLE:
		// Start scanning/patrolling
		currentInternalState = STATE_SCANNING_AREA;
		break;

	case STATE_MOVING:
	{
		// This state is ONLY for moving to commander-designated locations when NO enemy is visible
		printf("Warrior at (%.2f, %.2f) moving to target ( %d, %d )...\n", x, y, targetLocation.x, targetLocation.y);

		// Validate target location
		if (targetLocation.x < 0 || targetLocation.x >= MSZ ||
			targetLocation.y < 0 || targetLocation.y >= MSZ) {
			currentInternalState = STATE_SCANNING_AREA;
			break;
		}

		// Start moving if not already
		if (!isMoving && path.empty()) {
			SetTarget(targetLocation.x, targetLocation.y, map, securityMap);
		}

		// Check if reached destination
		Point2D currentPos = getPosition();
		double distToTarget = hypot(currentPos.x - targetLocation.x, currentPos.y - targetLocation.y);

		if (distToTarget < 2.0 || (!isMoving && path.empty())) {
			// Reached commander's target location - start scanning area
			currentInternalState = STATE_SCANNING_AREA;
			scan_timer = 0;
		}
		break;
	}

	case STATE_SCANNING_AREA:
	{
		printf("Warrior at (%.2f, %.2f) scanning area...\n", x, y);

		// If commander gives a MOVE command and we're not currently moving, accept it
		if (is_commander_alive && currentCommand == ATTACK && (!isMoving || path.empty())) {
			// Check if commander's target is far enough to warrant going there
			Point2D currentPos = getPosition();
			double distToCommandTarget = hypot(currentPos.x - targetLocation.x,
				currentPos.y - targetLocation.y);

			if (distToCommandTarget > 8.0) {
				currentInternalState = STATE_MOVING;
				break;
			}
		}

		// Continue autonomous patrol - pick new destination when not moving
		if (!isMoving && path.empty()) {
			Point2D currentPos = getPosition();
			Point2D patrol_target = { -1, -1 };

			// Try to find a valid patrol target
			int attempts = 0;
			while (attempts < 30) {
				patrol_target.x = rand() % MSZ;
				patrol_target.y = rand() % MSZ;

				// Skip if it's our current position
				if (patrol_target.x == currentPos.x && patrol_target.y == currentPos.y) {
					attempts++;
					continue;
				}

				// Check if it's walkable
				if (map[patrol_target.y][patrol_target.x] != STONE &&
					map[patrol_target.y][patrol_target.x] != WATER) {
					// Calculate distance from current position
					double dist = hypot(patrol_target.x - currentPos.x,
						patrol_target.y - currentPos.y);

					// Accept if it's reasonably far
					if (dist > 8.0) {
						break;
					}
				}
				attempts++;
			}

			// Set target if valid
			if (patrol_target.x != -1 && patrol_target.y != -1 &&
				(patrol_target.x != currentPos.x || patrol_target.y != currentPos.y) &&
				map[patrol_target.y][patrol_target.x] != STONE &&
				map[patrol_target.y][patrol_target.x] != WATER) {
				SetTarget(patrol_target.x, patrol_target.y, map, securityMap);
			}
			else {
				// Fallback: pick ANY valid spot
				for (int i = 0; i < 100; i++) {
					int rx = rand() % MSZ;
					int ry = rand() % MSZ;
					if ((rx != currentPos.x || ry != currentPos.y) &&
						map[ry][rx] != STONE && map[ry][rx] != WATER) {
						SetTarget(rx, ry, map, securityMap);
						break;
					}
				}
			}
		}
		break;
	}

	case STATE_ENGAGING:
	{
		printf("Warrior at (%.2f, %.2f) engaging enemy!\n", x, y);

		// Double-check target is still valid
		if (!currentTargetEnemy) {
			currentInternalState = STATE_SCANNING_AREA;
			scan_timer = 0;
			break;
		}

		// Verify target still exists and is alive
		bool targetExists = false;
		for (Soldier* s : all_soldiers) {
			if (s == currentTargetEnemy && s->IsAlive()) {
				targetExists = true;
				break;
			}
		}

		if (!targetExists) {
			currentTargetEnemy = nullptr;
			currentInternalState = STATE_SCANNING_AREA;
			scan_timer = 0;
			break;
		}

		// Check if target is still visible
		Point2D tgtPos = currentTargetEnemy->getPosition();

		if (tgtPos.x < 0 || tgtPos.x >= MSZ || tgtPos.y < 0 || tgtPos.y >= MSZ ||
			!visibility[tgtPos.y][tgtPos.x]) {
			// Lost sight
			currentTargetEnemy = nullptr;
			currentInternalState = STATE_SCANNING_AREA;
			scan_timer = 0;
			break;
		}

		// Calculate distance to target
		double dx = tgtPos.x + 0.5 - x;
		double dy = tgtPos.y + 0.5 - y;
		double dist = hypot(dx, dy);

		if (dist > WARRIOR_FIRING_RANGE) {
			// Too far - need to move closer
			// Calculate best attack position
			Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map);

			// Only recalculate path if we don't have one or target moved significantly
			Point2D currentPos = getPosition();
			bool needNewPath = path.empty() || !isMoving;

			if (!needNewPath && !path.empty()) {
				// Check if our current path endpoint is still near the attack position
				Point2D pathEnd = path.front(); // First element is the final destination
				double pathEndDist = hypot(pathEnd.x - attackPos.x, pathEnd.y - attackPos.y);
				if (pathEndDist > 3.0) {
					needNewPath = true;
				}
			}

			if (needNewPath) {
				SetTarget(attackPos.x, attackPos.y, map, securityMap);
			}
		}
		else {
			// In range - stop moving and shoot
			path.clear();
			isMoving = false;

			if (shoot_cooldown_timer == 0) {
				Bullet* new_bullet = Shoot(currentTargetEnemy);
				if (new_bullet != nullptr) {
					new_bullet->setIsMoving(true);
					active_bullets.push_back(new_bullet);
				}
				shoot_cooldown_timer = WARRIOR_SHOOT_COOLDOWN_FRAMES;
			}
		}
		break;
	}

	case STATE_FINDING_COVER:
		currentInternalState = STATE_SCANNING_AREA;
		currentTargetEnemy = nullptr;
		scan_timer = 0;
		break;
	}
}

Point2D Warrior::CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ]) {
	// SAFETY CHECK: Verify target is valid
	if (!target || !target->IsAlive()) {
		return getPosition(); // Return current position if invalid
	}

	Point2D targetPos = target->getPosition();

	// SAFETY CHECK: Verify position is valid
	if (targetPos.x < 0 || targetPos.x >= MSZ || targetPos.y < 0 || targetPos.y >= MSZ) {
		return getPosition();
	}

	Point2D currentPos = getPosition();
	Point2D bestPos = { -1, -1 };
	double minDistSqToWarrior = 1e9;

	int radius = (int)WARRIOR_FIRING_RANGE;

	for (int dx = -radius; dx <= radius; ++dx) {
		for (int dy = -radius; dy <= radius; ++dy) {
			int x = targetPos.x + dx;
			int y = targetPos.y + dy;

			if (x < 0 || x >= MSZ || y < 0 || y >= MSZ) continue;
			if (map[y][x] == STONE || map[y][x] == WATER) continue;

			double distSqToTarget = dx * dx + dy * dy;
			if (distSqToTarget > WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) continue;
			if (!HasLineOfSight(x, y, targetPos.x, targetPos.y, map)) continue;

			double distSqToWarrior = pow(x - currentPos.x, 2) + pow(y - currentPos.y, 2);
			if (distSqToWarrior < minDistSqToWarrior) {
				minDistSqToWarrior = distSqToWarrior;
				bestPos = { x, y };
			}
		}
	}

	if (bestPos.x == -1) {
		// fallback: return current position (don't chase invalid target)
		return currentPos;
	}

	return bestPos;
}