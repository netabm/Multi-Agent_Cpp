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

	// if commander is dead, always look for enemies when idle
	if (!is_commander_alive && currentInternalState == STATE_IDLE) {
		Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
		if (enemy) {
			currentTargetEnemy = enemy;
			currentInternalState = STATE_ENGAGING;
		}
	}

	switch (currentInternalState) {
	case STATE_IDLE:
		// Scan for enemies if commander is dead
		if (!is_commander_alive) {
			Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
			if (enemy) {
				currentTargetEnemy = enemy;
				currentInternalState = STATE_ENGAGING;
			}
		}
		else
			printf("hello\n");
			currentInternalState = STATE_SCANNING_AREA;
		break;

	case STATE_MOVING:
	{
		//printf("moving\n");
		// Validate target location before pathfinding
		if (targetLocation.x < 0 || targetLocation.x >= MSZ ||
			targetLocation.y < 0 || targetLocation.y >= MSZ) {
			// Invalid target, go idle
			currentInternalState = STATE_IDLE;
			break;
		}

		// Move toward target location
		if (!isMoving && path.empty()) {
			SetTarget(targetLocation.x, targetLocation.y, map, securityMap);
		}

		// If reached destination, switch state
		if (!isMoving && path.empty()) {
			if (currentCommand == ATTACK) {
				currentInternalState = STATE_SCANNING_AREA;
				scan_timer = 0; // Reset scan timer when entering scanning
			}
			else {
				currentInternalState = STATE_IDLE;
			}
		}

		// While moving to ATTACK area, check for enemies
		if (currentCommand == ATTACK) {
			Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
			if (enemy) {
				currentTargetEnemy = enemy;
				currentInternalState = STATE_ENGAGING;
				path.clear();
				isMoving = false;
			}
		}
		break;
	}

	case STATE_SCANNING_AREA:
	{
		// Keep scanning for enemies over time
		Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
		if (enemy) {
			currentTargetEnemy = enemy;
			currentInternalState = STATE_ENGAGING;
			scan_timer = 0;
			break;
		}

		// Only update scan behavior when NOT actively moving
		if (!isMoving && path.empty()) {
			scan_timer++;

			if (scan_timer > SCAN_TIMEOUT_FRAMES) {
				// After timeout, pick a new random nearby position
				Point2D pos = getPosition();
				int dx = (rand() % 7) - 3; // -3..3
				int dy = (rand() % 7) - 3;
				Point2D new_target = { pos.x + dx, pos.y + dy };

				// Clamp to map bounds
				if (new_target.x < 0) new_target.x = 0;
				else if (new_target.x >= MSZ) new_target.x = MSZ - 1;

				if (new_target.y < 0) new_target.y = 0;
				else if (new_target.y >= MSZ) new_target.y = MSZ - 1;

				// Only set new target if it's different from current position
				if (new_target.x != pos.x || new_target.y != pos.y) {
					SetTarget(new_target.x, new_target.y, map, securityMap);
				}

				scan_timer = 0; // Reset timer
			}
		}
		// Stay in scanning state
		break;
	}

	case STATE_ENGAGING:
	{
		if (!currentTargetEnemy || !currentTargetEnemy->IsAlive()) {
			// Target lost or dead
			currentTargetEnemy = nullptr;
			// Return to scanning if we were in ATTACK mode, otherwise idle
			currentInternalState = (currentCommand == ATTACK) ? STATE_SCANNING_AREA : STATE_IDLE;
			scan_timer = 0;
			break;
		}

		// Check if target is visible
		Point2D tgtPos = currentTargetEnemy->getPosition();

		if (!visibility[tgtPos.y][tgtPos.x]) {
			// Lost sight
			currentTargetEnemy = nullptr;
			currentInternalState = (currentCommand == ATTACK) ? STATE_SCANNING_AREA : STATE_IDLE;
			scan_timer = 0;
			break;
		}

		// Calculate distance to target
		double dx = tgtPos.x + 0.5 - x;
		double dy = tgtPos.y + 0.5 - y;
		double dist = hypot(dx, dy);

		// Determine attack position
		Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map);

		if (dist > WARRIOR_FIRING_RANGE) {
			// Move toward attack position
			if (!isMoving || path.empty()) {
				SetTarget(attackPos.x, attackPos.y, map, securityMap);
			}
		}
		else {
			// In range, shoot
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
		// TODO: BFS to safe spot
		// Placeholder: return to idle if no BFS implemented
		currentInternalState = STATE_IDLE;
		currentCommand = IDLE;
		break;
	}
}

Point2D Warrior::CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ]) {
	Point2D targetPos = target->getPosition();
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
		// fallback: move toward target
		return targetPos;
	}

	return bestPos;
}