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
		return new Grenade(x + 0.5, y + 0.5, target_pos.x + 0.5, target_pos.y + 0.5, this);
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
	//currentInternalState = (cmd == DEFEND) ? STATE_FINDING_COVER : STATE_MOVING;
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

	if (!is_commander_alive && currentCommand != IDLE) {
		currentCommand = IDLE;
		targetLocation = { -1, -1 };
	}

	// --- Cooldowns ---
	if (shoot_cooldown_timer > 0) shoot_cooldown_timer--;
	if (grenade_cooldown_timer > 0) grenade_cooldown_timer--;

	// --- Update visibility ---
	CalculateVisibility(this, map, visibility);

	// --- Detect closest visible enemy ---
	Soldier* visibleEnemy = FindClosestVisibleEnemy(all_soldiers);
	if (visibleEnemy) {
		// Engage visible enemy immediately
		currentTargetEnemy = visibleEnemy;
		currentInternalState = STATE_ENGAGING;
	}
	else {
		// No visible enemy: act based on commander command
		currentTargetEnemy = nullptr;
		switch (currentCommand) {
		case ATTACK:
			if (targetLocation.x >= 0) {
				double dist = hypot(x - targetLocation.x, y - targetLocation.y);
				currentInternalState = (dist > 2.0) ? STATE_MOVING : STATE_SCANNING_AREA;
			}
			else {
				currentInternalState = STATE_SCANNING_AREA;
			}
			break;

		case DEFEND:
			currentInternalState = STATE_FINDING_COVER;
			break;

		case IDLE:
		default:
			currentInternalState = STATE_SCANNING_AREA;
			break;
		}
	}

	// --- Handle states ---
	switch (currentInternalState) {
	case STATE_MOVING:
		HandleMoving(map, securityMap);
		break;

	case STATE_SCANNING_AREA:
		HandleScanning(map, securityMap);
		break;

	case STATE_ENGAGING:
		HandleEngaging(active_bullets, active_grenades, map, securityMap, all_soldiers);
		break;

	case STATE_FINDING_COVER:
		HandleFindingCover(map, securityMap);
		break;

	case STATE_IDLE:
	default:
		break; // do nothing
	}
}

void Warrior::HandleMoving(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!isMoving && path.empty()) SetTarget(targetLocation.x, targetLocation.y, map, securityMap);

	Point2D pos = getPosition();
	double dist = hypot(pos.x - targetLocation.x, pos.y - targetLocation.y);
	if (dist < 2.0 || (!isMoving && path.empty())) {
		currentInternalState = STATE_SCANNING_AREA;
		scan_timer = 0;
	}
	//Move(map);
}

void Warrior::HandleScanning(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!isMoving && path.empty()) {
		Point2D pos = getPosition();
		Point2D patrol_target = { -1, -1 };

		for (int attempts = 0; attempts < 30; attempts++) {
			patrol_target.x = rand() % MSZ;
			patrol_target.y = rand() % MSZ;

			if (patrol_target.x == pos.x && patrol_target.y == pos.y) continue;
			if (map[patrol_target.y][patrol_target.x] == STONE || map[patrol_target.y][patrol_target.x] == WATER) continue;

			double dist = hypot(patrol_target.x - pos.x, patrol_target.y - pos.y);
			if (dist > 8.0) break;
		}

		if (patrol_target.x != -1) SetTarget(patrol_target.x, patrol_target.y, map, securityMap);
	}
}

void Warrior::HandleEngaging(std::vector<Bullet*>& active_bullets,
	std::vector<Grenade*>& active_grenades,
	int map[MSZ][MSZ],
	double securityMap[MSZ][MSZ],
	const std::vector<Soldier*>& all_soldiers)
{
	if (!currentTargetEnemy || !currentTargetEnemy->IsAlive()) {
		currentTargetEnemy = nullptr;
		currentInternalState = STATE_SCANNING_AREA;
		scan_timer = 0;
		return;
	}

	Point2D tgtPos = currentTargetEnemy->getPosition();
	if (!visibility[tgtPos.y][tgtPos.x]) {
		currentTargetEnemy = nullptr;
		currentInternalState = STATE_SCANNING_AREA;
		scan_timer = 0;
		return;
	}

	double dx = tgtPos.x + 0.5 - x;
	double dy = tgtPos.y + 0.5 - y;
	double dist = hypot(dx, dy);

	// --- GRENADE THROWING LOGIC ---
	const double MIN_GRENADE_DISTANCE = 8.0;
	const double MAX_GRENADE_DISTANCE = GRENADE_THROW_RANGE;
	const double GRENADE_CHANCE = 0.005; // 30% chance (60 FPS * 0.005 = ~0.30 per second)

	// THROW GRENADE
	if (dist >= MIN_GRENADE_DISTANCE &&
		dist <= MAX_GRENADE_DISTANCE &&
		grenade_ammo > 0 &&
		grenade_cooldown_timer == 0 &&
		((double)rand() / RAND_MAX) < GRENADE_CHANCE)
	{
		// ThrowGrenade() handles ammo decrement and creating the grenade
		Grenade* g = ThrowGrenade(currentTargetEnemy);
		if (g) {
			active_grenades.push_back(g);
			grenade_cooldown_timer = WARRIOR_GRENADE_COOLDOWN_FRAMES;
		}
		// Threw a grenade, so stop moving and wait
		path.clear();
		isMoving = false;
	}
	else if (dist > WARRIOR_FIRING_RANGE)
	{
		// Out of range, move
		Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map);
		if (path.empty() || !isMoving) {
			SetTarget(attackPos.x, attackPos.y, map, securityMap);
		}
	}
	else
	{
		path.clear();
		isMoving = false;
		if (shoot_cooldown_timer == 0) {
			Bullet* b = Shoot(currentTargetEnemy);
			if (b) {
				b->setIsMoving(true);
				active_bullets.push_back(b);
			}
			shoot_cooldown_timer = WARRIOR_SHOOT_COOLDOWN_FRAMES;
		}
	}
}


void Warrior::HandleFindingCover(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	// TODO: implement BFS or nearest safe spot
	// For now, just patrol
	//HandleScanning(map, securityMap);
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