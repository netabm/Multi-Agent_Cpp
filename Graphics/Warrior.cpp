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
		glColor3d(0.8, 0.2, 0.8); // Purple
	}
	else
	{
		glColor3d(1.0, 0.5, 0.0); // Orange
	}

	glBegin(GL_POLYGON);
	glVertex2d(x, y);
	glVertex2d(x, y + 1);
	glVertex2d(x + 1, y + 1);
	glVertex2d(x + 1, y);
	glEnd();

	glColor3d(0, 0, 0);

	glRasterPos2d(x + 0.1, y + 0.1);

	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'W');

	// Draw Ammo Count
	glColor3d(0.1, 0.1, 0.1);
	glRasterPos2d(x - 1, y - 0.7);

	// Create ammo string
	std::stringstream ss;
	ss << "B:" << bullet_ammo << " G:" << grenade_ammo;
	std::string ammo_str = ss.str();

	// Draw each character of the string
	for (char c : ammo_str) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
	}
}

Bullet* Warrior::Shoot(Soldier* target)
{
	if (bullet_ammo <= 0 || target == nullptr || !target->IsAlive()) {
		return nullptr;
	}

	bullet_ammo--;
	double target_x_precise = target->getX();
	double target_y_precise = target->getY();
	double angle = atan2((target_y_precise + 0.5) - (y + 0.5),
		(target_x_precise + 0.5) - (x + 0.5));

	Bullet* new_bullet = new Bullet(x + 0.5, y + 0.5, angle, this);
	new_bullet->setIsMoving(true);

	return new_bullet;
}

bool Warrior::CheckFirePath(double startX, double startY, double targetX, double targetY, int map[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers)
{
	double dx = targetX - startX;
	double dy = targetY - startY;
	double dist = sqrt(dx * dx + dy * dy);

	if (dist < 0.1) return true;

	double dirX = dx / dist;
	double dirY = dy / dist;

	// Calculate Vector for width (Right Vector)
	// If Forward is (x, y), Right is (-y, x)
	double perpX = -dirY;
	double perpY = dirX;

	double radius = 0.25;

	// Define a Lambda to handle the actual Ray Marching to keep code clean
	auto IsRayClear = [&](double sx, double sy, double ex, double ey) -> bool {
		double rdx = ex - sx;
		double rdy = ey - sy;
		double rDist = sqrt(rdx * rdx + rdy * rdy);
		double rDirX = rdx / rDist;
		double rDirY = rdy / rDist;

		double currX = sx;
		double currY = sy;

		// Iterate. Start slightly forward to avoid hitting own hitbox if offset
		for (double d = 0.0; d < rDist; d += 0.1)
		{
			int gridX = (int)currX;
			int gridY = (int)currY;

			if (gridX < 0 || gridX >= MSZ || gridY < 0 || gridY >= MSZ) return false;

			if (map[gridY][gridX] == STONE || map[gridY][gridX] == TREE) return false;

			// Friendly fire check
			for (Soldier* s : all_soldiers) {
				if (s == this || !s->IsAlive() || s->getTeam() != this->team) continue;
				if (pow(currX - s->getX() - 0.5, 2) + pow(currY - s->getY() - 0.5, 2) < 0.25) return false;
			}

			currX += rDirX * 0.05;
			currY += rDirY * 0.05;
		}
		return true;
		};

	// Check Center Ray
	if (!IsRayClear(startX, startY, targetX, targetY)) return false;

	// Check Left Wing (Offset by radius)
	if (!IsRayClear(startX + perpX * radius, startY + perpY * radius,
		targetX + perpX * radius, targetY + perpY * radius)) return false;

	// Check Right Wing (Offset by radius)
	if (!IsRayClear(startX - perpX * radius, startY - perpY * radius,
		targetX - perpX * radius, targetY - perpY * radius)) return false;

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

	// Cooldowns
	if (shoot_cooldown_timer > 0) shoot_cooldown_timer--;
	if (grenade_cooldown_timer > 0) grenade_cooldown_timer--;

	CalculateVisibility(this, map, visibility);

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
			currentInternalState = STATE_FINDING_COVER;
			break;

		case IDLE:
		default:
			currentInternalState = STATE_SCANNING_AREA;
			break;
		}
	}

	// Handle states
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

	bool inRange = (dist <= WARRIOR_FIRING_RANGE);
	bool clearShot = CheckFirePath(x + 0.5, y + 0.5, tgtPos.x + 0.5, tgtPos.y + 0.5, map, all_soldiers);

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

	// If already moving along a path allow movement to continue
	if (isMoving && !path.empty()) {
		return;
	}

	// Calculate new attack position only if needed
	Point2D myPos = getPosition();
	Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map, all_soldiers);

	// If no better pos found and we're still not in range/clear, try a wider search.
	if (attackPos.x == myPos.x && attackPos.y == myPos.y && !(inRange && clearShot)) {
		// Try to move sideways 3 units to find a better angle.
		// Calculate a vector vertically to the enemy direction.
		double rx = x + (-dy / (dist + 1e-6)) * 3.0;
		double ry = y + (dx / (dist + 1e-6)) * 3.0;
		int tryX = std::max(0, std::min(MSZ - 1, (int)round(rx)));
		int tryY = std::max(0, std::min(MSZ - 1, (int)round(ry)));
		if (map[tryY][tryX] != STONE && map[tryY][tryX] != WATER && !IsCellOccupied(tryX, tryY, all_soldiers)) {
			attackPos = { tryX, tryY };
		}
	}

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
		if (enemy && CheckFirePath(x + 0.5, y + 0.5, enemy->getX() + 0.5, enemy->getY() + 0.5, map, all_soldiers)) {
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

Point2D Warrior::CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers) {
	if (!target || !target->IsAlive()) return getPosition();

	Point2D targetPos = target->getPosition();
	Point2D currentPos = getPosition();

	// Vector Setup
	bool commanderIsValid = false;
	if (commander) {
		for (Soldier* s : all_soldiers) {
			if (s == commander) {
				commanderIsValid = true;
				break;
			}
		}
	}
	std::vector<Point2D> dummy_reserved;
	std::vector<Point2D>* reserved_ptr = &dummy_reserved;
	if (commanderIsValid && commander->IsAlive()) {
		reserved_ptr = &commander->GetReservedAttackSpots();
	}
	auto& reserved = *reserved_ptr;

	// If in range, have a clear shot, and not overlapping anyone, stay here
	double distSq = pow(targetPos.x - currentPos.x, 2) + pow(targetPos.y - currentPos.y, 2);
	bool amIOverlapping = IsCellOccupied((int)currentPos.x, (int)currentPos.y, all_soldiers);

	if (!amIOverlapping && distSq <= WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) {
		if (CheckFirePath(currentPos.x + 0.5, currentPos.y + 0.5, targetPos.x + 0.5, targetPos.y + 0.5, map, all_soldiers)) {
			return currentPos;
		}
	}

	Point2D bestShootingPos = { -1, -1 };
	double minDistSqToWarrior = 1e9;
	Point2D bestChasePos = { -1, -1 };
	double minChaseDistSq = 1e9;

	// PHASE 1: Search around CURRENT position (Local Repositioning)
	int limit = 8;
	for (int dx = -limit; dx <= limit; ++dx) {
		for (int dy = -limit; dy <= limit; ++dy) {
			int xCandidate = currentPos.x + dx;
			int yCandidate = currentPos.y + dy;

			if (xCandidate < 0 || xCandidate >= MSZ || yCandidate < 0 || yCandidate >= MSZ) continue;
			if (map[yCandidate][xCandidate] == STONE || map[yCandidate][xCandidate] == WATER || map[yCandidate][xCandidate] == TREE) continue;

			Point2D candidate = { xCandidate, yCandidate };

			bool isReserved = false;
			if (!(candidate.x == currentPos.x && candidate.y == currentPos.y)) {
				for (const Point2D& r : reserved) {
					if (r.x == candidate.x && r.y == candidate.y) {
						isReserved = true;
						break;
					}
				}
			}
			if (isReserved) continue;

			if (IsCellOccupied(xCandidate, yCandidate, all_soldiers)) continue;

			double distSqToTarget = pow(xCandidate - targetPos.x, 2) + pow(yCandidate - targetPos.y, 2);

			// Track best chase position (closest to enemy)
			if (distSqToTarget < minChaseDistSq) {
				minChaseDistSq = distSqToTarget;
				bestChasePos = { xCandidate, yCandidate };
			}

			// Track best shooting position (closest, but in range of enemy)
			if (distSqToTarget <= WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) {
				if (CheckFirePath(xCandidate + 0.5, yCandidate + 0.5, targetPos.x + 0.5, targetPos.y + 0.5, map, all_soldiers)) {
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
		reserved.push_back(bestShootingPos);
		return bestShootingPos;
	}
	if (bestChasePos.x != -1) {
		reserved.push_back(bestChasePos);
		return bestChasePos;
	}

	// PHASE 2: Search around target position
	// Used when local area is blocked or bad. We look for spots near the enemy.
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

			bool isReserved = false;
			if (!(candidate.x == currentPos.x && candidate.y == currentPos.y)) {
				for (const Point2D& r : reserved) {
					if (r.x == candidate.x && r.y == candidate.y) {
						isReserved = true;
						break;
					}
				}
			}
			if (isReserved) continue;

			if (IsCellOccupied(xCandidate, yCandidate, all_soldiers)) continue;

			double distSqToTarget = dx * dx + dy * dy;

			// Must be in firing range
			if (distSqToTarget <= WARRIOR_FIRING_RANGE * WARRIOR_FIRING_RANGE) {
				// Must have clear shot
				if (CheckFirePath(xCandidate + 0.5, yCandidate + 0.5, targetPos.x + 0.5, targetPos.y + 0.5, map, all_soldiers)) {
					// Pick the closest position
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
		reserved.push_back(bestFlankPos);
		return bestFlankPos;
	}

	// PHASE 3: No position found, move toward enemy
	Point2D moveToward = { -1, -1 };
	double dx = targetPos.x - currentPos.x;
	double dy = targetPos.y - currentPos.y;
	double dist = sqrt(dx * dx + dy * dy);

	if (dist > 0) {
		int stepSize = 4;
		int newX = currentPos.x + (int)((dx / dist) * stepSize);
		int newY = currentPos.y + (int)((dy / dist) * stepSize);

		// Clamp
		if (newX < 0) newX = 0; if (newX >= MSZ) newX = MSZ - 1;
		if (newY < 0) newY = 0; if (newY >= MSZ) newY = MSZ - 1;

		if (map[newY][newX] != STONE && map[newY][newX] != WATER && map[newY][newX] != TREE) {
			bool occupied = IsCellOccupied(newX, newY, all_soldiers);
			bool reservedSpot = false;

			if (!(newX == currentPos.x && newY == currentPos.y)) {
				for (const Point2D& r : reserved) {
					if (r.x == newX && r.y == newY) {
						reservedSpot = true;
						break;
					}
				}
			}

			if (!occupied && !reservedSpot) {
				moveToward = { newX, newY };
			}
		}
	}

	if (moveToward.x != -1) {
		reserved.push_back(moveToward);
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