#include "Warrior.h"
#include "Commander.h"
#include "glut.h"
#include <math.h>
#include <sstream>

Warrior::Warrior(double start_x, double start_y, int team_id, Commander* cmd)
	: Soldier(start_x, start_y, team_id), commander(cmd)
{
	bullet_ammo = W_NUM_BULLETS;
	grenade_ammo = W_NUM_GRENADES;
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
	if (bullet_ammo <= 0 || target == nullptr || !target->IsAlive()) {
		return nullptr;
	}

	bullet_ammo--;

	// Use precise coordinates for angle calculation
	double target_x_precise = target->getX();
	double target_y_precise = target->getY();

	double angle = atan2((target_y_precise + 0.5) - (y + 0.5),
		(target_x_precise + 0.5) - (x + 0.5));

	Bullet* new_bullet = new Bullet(x + 0.5, y + 0.5, angle, this);
	new_bullet->setIsMoving(true);

	return new_bullet;
}

bool Warrior::CheckFirePath(double startX, double startY, double targetX, double targetY, int map[MSZ][MSZ])
{
	int x0 = (int)startX;
	int y0 = (int)startY;
	int x1 = (int)targetX;
	int y1 = (int)targetY;

	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	int x = x0;
	int y = y0;

	while (true)
	{
		// don't check the starting cell (the shooter’s own tile)
		if (!(x == x0 && y == y0))
		{
			if (x < 0 || x >= MSZ || y < 0 || y >= MSZ)
				return false;

			if (map[y][x] == STONE || map[y][x] == TREE)
				return false;
		}

		if (x == x1 && y == y1)
			break;

		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x += sx; }
		if (e2 < dx) { err += dx; y += sy; }
	}

	return true;
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

	if (currentCommand == cmd && targetLocation.x == location.x && targetLocation.y == location.y) {
		return;
	}

	if (cmd == DEFEND && currentCommand != DEFEND) {
		hasDefensePosition = false;
		lastHealthInCover = -1;
	}

	if (cmd != DEFEND) {
		hasDefensePosition = false;
		lastHealthInCover = -1;
	}

	currentCommand = cmd;
	targetLocation = location;
	currentTargetEnemy = nullptr;
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

void Warrior::Update(const std::vector<Soldier*>& all_soldiers,
	std::vector<Bullet*>& active_bullets,
	std::vector<Grenade*>& active_grenades,
	int map[MSZ][MSZ],
	double securityMap[MSZ][MSZ])
{
	if (!IsAlive()) return;

	// --- 1. ROSTER CHECK (Prevent Crash) ---
	if (commander) {
		bool commanderStillExists = false;
		for (Soldier* s : all_soldiers) {
			if (s == commander) {
				commanderStillExists = true;
				break;
			}
		}
		if (!commanderStillExists) commander = nullptr;
	}

	// Autonomous logic
	if (commander == nullptr)
	{
		if (health < 25) {
			currentCommand = DEFEND;
		}
		else {
			currentCommand = ATTACK;
		}
	}

	// --- Cooldowns ---
	if (shoot_cooldown_timer > 0) shoot_cooldown_timer--;
	if (grenade_cooldown_timer > 0) grenade_cooldown_timer--;

	// --- Update visibility ---
	CalculateVisibility(this, map, visibility);

	// --- Detect closest visible enemy ---
	Soldier* visibleEnemy = FindClosestVisibleEnemy(all_soldiers);

	if (visibleEnemy && visibleEnemy->IsAlive()) {
		currentTargetEnemy = visibleEnemy;

		if (currentCommand == DEFEND) {
			currentInternalState = STATE_FINDING_COVER;
		}
		else {
			currentInternalState = STATE_ENGAGING;
		}
	}
	else {
		// No enemy visible
		currentTargetEnemy = nullptr;

		switch (currentCommand) {
		case ATTACK:
			// If autonomous and attacking but no enemy seen, Scan the area
			if (commander == nullptr) {
				currentInternalState = STATE_SCANNING_AREA;
			}
			else if (targetLocation.x >= 0) {
				// Follow commander's order
				double dist = hypot(x - targetLocation.x, y - targetLocation.y);
				currentInternalState = (dist > 2.0) ? STATE_MOVING : STATE_SCANNING_AREA;
			}
			else {
				currentInternalState = STATE_SCANNING_AREA;
			}
			break;

		case DEFEND:
			// If we need to defend but see no one, we still need to find a cover spot
			currentInternalState = STATE_FINDING_COVER;
			break;

		case IDLE:
		default:
			// If autonomous, IDLE usually means SCAN
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
		HandleFindingCover(map, securityMap, all_soldiers, active_bullets);
		break;
	case STATE_IDLE:
	default:
		break;
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
	int tx = clamp(tgtPos.x, 0, MSZ - 1);
	int ty = clamp(tgtPos.y, 0, MSZ - 1);

	if (!visibility[ty][tx]) {
		currentTargetEnemy = nullptr;
		currentInternalState = STATE_SCANNING_AREA;
		scan_timer = 0;
		return;
	}

	double dx = tgtPos.x + 0.5 - x;
	double dy = tgtPos.y + 0.5 - y;
	double dist = hypot(dx, dy);

	// Grenade logic
	const double MIN_GRENADE_DISTANCE = 8.0;
	const double MAX_GRENADE_DISTANCE = GRENADE_THROW_RANGE;
	const double GRENADE_CHANCE = 0.005;

	if (dist >= MIN_GRENADE_DISTANCE &&
		dist <= MAX_GRENADE_DISTANCE &&
		grenade_ammo > 0 &&
		grenade_cooldown_timer == 0 &&
		((double)rand() / RAND_MAX) < GRENADE_CHANCE)
	{
		Grenade* g = ThrowGrenade(currentTargetEnemy);
		if (g) {
			active_grenades.push_back(g);
			grenade_cooldown_timer = WARRIOR_GRENADE_COOLDOWN_FRAMES;
		}

		return;
	}

	// --- KEY FIX: Check if we're ALREADY in a good position ---
	bool inRange = (dist <= WARRIOR_FIRING_RANGE);
	bool clearShot = CheckFirePath(x + 0.5, y + 0.5, tgtPos.x + 0.5, tgtPos.y + 0.5, map);

	if (inRange && clearShot) {
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
		return;
	}

	// --- Only move if NOT already moving to a valid position ---
	Point2D myPos = getPosition();

	// If we're already moving, let the movement complete before recalculating
	if (isMoving && !path.empty()) {
		return;
	}

	// Calculate new attack position only if needed
	Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map, all_soldiers);

	if ((attackPos.x != myPos.x || attackPos.y != myPos.y)) {
		SetTarget(attackPos.x, attackPos.y, map, securityMap);
	}
}


void Warrior::HandleFindingCover(int map[MSZ][MSZ], double securityMap[MSZ][MSZ],
	const std::vector<Soldier*>& all_soldiers,
	std::vector<Bullet*>& active_bullets)
{
	// Defensive fire
	if (shoot_cooldown_timer == 0) {
		Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
		if (enemy && CheckFirePath(x + 0.5, y + 0.5, enemy->getX() + 0.5, enemy->getY() + 0.5, map)) {
			Bullet* b = Shoot(enemy);
			if (b) {
				b->setIsMoving(true);
				active_bullets.push_back(b);
			}
			shoot_cooldown_timer = WARRIOR_SHOOT_COOLDOWN_FRAMES * 2;
		}
	}

	// Safe reserved reference
	std::vector<Point2D> dummy_reserved;
	std::vector<Point2D>* reserved_ptr = &dummy_reserved;
	Point2D threat = { -1, -1 };

	if (commander && commander->IsAlive()) {
		reserved_ptr = &commander->GetReservedDefendSpots();
		threat = commander->getTargetLocation();
	}
	else {
		Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
		if (enemy) threat = enemy->getPosition();
	}
	auto& reserved = *reserved_ptr;
	Point2D myPos = getPosition();

	// Check compromised cover
	if (hasDefensePosition && lastHealthInCover != -1 && health < lastHealthInCover) {
		hasDefensePosition = false;
		lastHealthInCover = -1;
		path.clear();
	}

	// Find new cover
	if (!hasDefensePosition) {
		Point2D found = FindNearestCover(myPos, threat, reserved, map);
		if (found.x != -1) {
			defensePosition = found;
			if (commander && commander->IsAlive()) reserved.push_back(found);
			hasDefensePosition = true;
			lastHealthInCover = health;
			SetTarget(defensePosition.x, defensePosition.y, map, securityMap);
			return;
		}

		// Retreat if no cover
		if (threat.x != -1) {
			double dx = myPos.x - threat.x;
			double dy = myPos.y - threat.y;
			double len = sqrt(dx * dx + dy * dy);
			if (len > 0) {
				int retreatX = myPos.x + (int)((dx / len) * 5);
				int retreatY = myPos.y + (int)((dy / len) * 5);
				retreatX = std::max(0, std::min(MSZ - 1, retreatX));
				retreatY = std::max(0, std::min(MSZ - 1, retreatY));
				if (path.empty() || !isMoving) SetTarget(retreatX, retreatY, map, securityMap);
			}
		}
		return;
	}

	// Move toward cover
	double dist = hypot(myPos.x - defensePosition.x, myPos.y - defensePosition.y);
	if (dist > 1.5) {
		if (path.empty() || !isMoving)
			SetTarget(defensePosition.x, defensePosition.y, map, securityMap);
		return;
	}

	// In position
	lastHealthInCover = health;
}

Point2D Warrior::CalculateAttackPosition(Soldier * target, int map[MSZ][MSZ], const std::vector<Soldier*>&all_soldiers) {
	if (!target || !target->IsAlive()) return getPosition();

	Point2D targetPos = target->getPosition();
	Point2D currentPos = getPosition();

	// --- FIX START: Safe Vector Setup ---
	// 1. Verify Commander actually exists (prevent dangling pointer crash)
	bool commanderIsValid = false;
	if (commander) {
		for (Soldier* s : all_soldiers) {
			if (s == commander) {
				commanderIsValid = true;
				break;
			}
		}
	}

	// 2. Create a safe reference to a vector
	// If commander is dead/invalid, 'reserved' points to this local dummy.
	// If commander is alive, 'reserved' points to the real list.
	std::vector<Point2D> dummy_reserved;
	std::vector<Point2D>* reserved_ptr = &dummy_reserved;

	if (commanderIsValid && commander->IsAlive()) {
		reserved_ptr = &commander->GetReservedAttackSpots();
	}

	// Now use this reference for the rest of the function
	auto& reserved = *reserved_ptr;
	// --- FIX END ---


	Point2D bestShootingPos = { -1, -1 };
	double minDistSqToWarrior = 1e9;

	Point2D bestChasePos = { -1, -1 };
	double minChaseDistSq = 1e9;

	// PHASE 1: Search around CURRENT position (local repositioning)
	int limit = 8;

	for (int dx = -limit; dx <= limit; ++dx) {
		for (int dy = -limit; dy <= limit; ++dy) {
			int xCandidate = currentPos.x + dx;
			int yCandidate = currentPos.y + dy;

			if (xCandidate < 0 || xCandidate >= MSZ || yCandidate < 0 || yCandidate >= MSZ) continue;
			if (map[yCandidate][xCandidate] == STONE || map[yCandidate][xCandidate] == WATER || map[yCandidate][xCandidate] == TREE) continue;

			Point2D candidate = { xCandidate, yCandidate };

			// Safe check using 'reserved'
			bool isReserved = false;
			for (const Point2D& r : reserved) {
				if (r.x == candidate.x && r.y == candidate.y) {
					isReserved = true;
					break;
				}
			}
			if (isReserved) continue;

			if (IsCellOccupied(xCandidate, yCandidate, all_soldiers)) continue;


			double distSqToTarget = pow(xCandidate - targetPos.x, 2) + pow(yCandidate - targetPos.y, 2);

			if (distSqToTarget < minChaseDistSq) {
				minChaseDistSq = distSqToTarget;
				bestChasePos = { xCandidate, yCandidate };
			}

			if (distSqToTarget <= WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) {
				if (CheckFirePath(xCandidate + 0.5, yCandidate + 0.5, targetPos.x + 0.5, targetPos.y + 0.5, map)) {
					double distSqToMe = dx * dx + dy * dy;
					if (distSqToMe < minDistSqToWarrior) {
						minDistSqToWarrior = distSqToMe;
						bestShootingPos = { xCandidate, yCandidate };
					}
				}
			}
		}
	}

	if (bestShootingPos.x != -1) {
		reserved.push_back(bestShootingPos); // Safe push
		return bestShootingPos;
	}

	if (bestChasePos.x != -1) {
		reserved.push_back(bestChasePos); // Safe push
		return bestChasePos;
	}

	// PHASE 2: No local position works - search around the TARGET to flank
	// This handles cases where enemy is behind cover
	int flankRadius = (int)WARRIOR_FIRING_RANGE;
	Point2D bestFlankPos = { -1, -1 };
	double bestFlankDistSq = 1e9;

	for (int dx = -flankRadius; dx <= flankRadius; ++dx) {
		for (int dy = -flankRadius; dy <= flankRadius; ++dy) {
			int xCandidate = targetPos.x + dx;
			int yCandidate = targetPos.y + dy;

			if (xCandidate < 0 || xCandidate >= MSZ || yCandidate < 0 || yCandidate >= MSZ) continue;
			if (map[yCandidate][xCandidate] == STONE || map[yCandidate][xCandidate] == WATER || map[yCandidate][xCandidate] == TREE) continue;

			Point2D candidate = { xCandidate, yCandidate };

			// Safe check using 'reserved'
			bool isReserved = false;
			for (const Point2D& r : reserved) {
				if (r.x == candidate.x && r.y == candidate.y) {
					isReserved = true;
					break;
				}
			}
			if (isReserved) continue;

			if (IsCellOccupied(xCandidate, yCandidate, all_soldiers)) continue;

			double distSqToTarget = dx * dx + dy * dy;

			// Must be in firing range
			if (distSqToTarget <= WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) {
				// Must have clear shot
				if (CheckFirePath(xCandidate + 0.5, yCandidate + 0.5, targetPos.x + 0.5, targetPos.y + 0.5, map)) {
					// Pick the position closest to our current position
					double distSqToMe = pow(xCandidate - currentPos.x, 2) + pow(yCandidate - currentPos.y, 2);
					if (distSqToMe < bestFlankDistSq) {
						bestFlankDistSq = distSqToMe;
						bestFlankPos = { xCandidate, yCandidate };
					}
				}
			}
		}
	}

	if (bestFlankPos.x != -1) {
		reserved.push_back(bestFlankPos); // Safe push
		return bestFlankPos;
	}

	// PHASE 3: Complete failure - move toward enemy and hope to find an angle
	Point2D moveToward = { -1, -1 };
	double dx = targetPos.x - currentPos.x;
	double dy = targetPos.y - currentPos.y;
	double dist = sqrt(dx * dx + dy * dy);

	if (dist > 0) {
		// Move 4 units toward the enemy
		int stepSize = 4;
		int newX = currentPos.x + (int)((dx / dist) * stepSize);
		int newY = currentPos.y + (int)((dy / dist) * stepSize);

		// Clamp to map
		if (newX < 0) newX = 0;
		if (newX >= MSZ) newX = MSZ - 1;
		if (newY < 0) newY = 0;
		if (newY >= MSZ) newY = MSZ - 1;

		// Make sure it's walkable
		if (map[newY][newX] != STONE && map[newY][newX] != WATER && map[newY][newX] != TREE) {
			moveToward = { newX, newY };
		}
	}

	if (moveToward.x != -1) {
		reserved.push_back(moveToward); // Safe push
		return moveToward;
	}

	return currentPos;
}

bool Warrior::IsCellOccupied(int x, int y, const std::vector<Soldier*>& all_soldiers) {
	for (Soldier* s : all_soldiers) {
		if (!s->IsAlive()) continue;
		if (s == this) continue;
		Point2D pos = s->getPosition();
		if ((int)pos.x == x && (int)pos.y == y) return true;
	}
	return false;
}