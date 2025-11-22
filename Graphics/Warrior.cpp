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

bool Warrior::CheckFirePath(double startX, double startY, double targetX, double targetY, int map[MSZ][MSZ])
{
	double dx = targetX - startX;
	double dy = targetY - startY;
	double dist = sqrt(dx * dx + dy * dy);

	// FIX 1: Only return true if we are basically inside the same pixel. 
	// 1.0 was way too generous.
	if (dist < 0.05) return true;

	double dirX = dx / dist;
	double dirY = dy / dist;

	double currX = startX;
	double currY = startY;
	double stepSize = 0.05;

	// FIX 2: Check closer to the target. 
	// Previously 'dist - 0.5' meant we stopped checking half a tile early.
	// If the enemy is right next to a wall, we might miss the wall.
	// stopping at 'dist - 0.1' ensures we check the space right up to their face.
	for (double d = 0; d < dist - 0.1; d += stepSize)
	{
		currX += dirX * stepSize;
		currY += dirY * stepSize;

		int gridX = (int)currX;
		int gridY = (int)currY;

		// Sanity check to ensure we don't check out of bounds
		if (gridX < 0 || gridX >= MSZ || gridY < 0 || gridY >= MSZ) return false;

		// FIX 3: If the ray has reached the target's actual cell, stop checking.
		// We don't want to return 'false' just because the target is standing near a tree.
		if (gridX == (int)targetX && gridY == (int)targetY) break;

		// 1. Direct Hit Check
		if (map[gridY][gridX] == STONE || map[gridY][gridX] == TREE) return false;

		// 2. "Thick Bullet" Check (Corner grazing)
		double subX = currX - gridX;
		double subY = currY - gridY;
		double safetyMargin = 0.3;

		if (subX > (1.0 - safetyMargin)) {
			if (gridX + 1 < MSZ && (map[gridY][gridX + 1] == STONE || map[gridY][gridX + 1] == TREE)) return false;
		}
		else if (subX < safetyMargin) {
			if (gridX - 1 >= 0 && (map[gridY][gridX - 1] == STONE || map[gridY][gridX - 1] == TREE)) return false;
		}

		if (subY > (1.0 - safetyMargin)) {
			if (gridY + 1 < MSZ && (map[gridY + 1][gridX] == STONE || map[gridY + 1][gridX] == TREE)) return false;
		}
		else if (subY < safetyMargin) {
			if (gridY - 1 >= 0 && (map[gridY - 1][gridX] == STONE || map[gridY - 1][gridX] == TREE)) return false;
		}
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

	// If the command and location are unchanged, do nothing (avoid wiping path every frame)
	if (currentCommand == cmd && targetLocation.x == location.x && targetLocation.y == location.y) {
		return;
	}

	// Only reset cover when transitioning into DEFEND from another state
	if (cmd == DEFEND && currentCommand != DEFEND) {
		hasDefensePosition = false;
		lastHealthInCover = -1;  // reset health in cover
	}

	// Leaving DEFEND
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

void Warrior::Update(const std::vector<Soldier*>& all_soldiers, std::vector<Bullet*>& active_bullets, std::vector<Grenade*>& active_grenades, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!IsAlive()) return;

	if (!commander || !commander->IsAlive()) {
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
		if (currentCommand != DEFEND) {
			currentInternalState = STATE_ENGAGING;
		}
		else {
			currentInternalState = STATE_FINDING_COVER;
		}
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
		HandleFindingCover(map, securityMap, all_soldiers, active_bullets);
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

	// 1. General Visibility Check (Can we see them at all?)
	if (!visibility[tgtPos.y][tgtPos.x]) {
		currentTargetEnemy = nullptr;
		currentInternalState = STATE_SCANNING_AREA;
		scan_timer = 0;
		return;
	}

	double dx = tgtPos.x + 0.5 - x;
	double dy = tgtPos.y + 0.5 - y;
	double dist = hypot(dx, dy);

	// --- GRENADE LOGIC ---
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
		path.clear();
		isMoving = false;
	}
	else
	{
		// --- SHOOTING LOGIC ---
		bool inRange = (dist <= WARRIOR_FIRING_RANGE);

		// Check shot from centers
		bool clearShot = CheckFirePath(x + 0.5, y + 0.5, tgtPos.x + 0.5, tgtPos.y + 0.5, map);

		if (inRange && clearShot)
		{
			// We have a valid shot. Stop and fire.
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
		else
		{
			// Either too far or obstacle in way: pick attack position
			Point2D attackPos = CalculateAttackPosition(currentTargetEnemy, map, all_soldiers);

			// If CalculateAttackPosition failed and returned our current cell, nudge toward target
			Point2D myPos = getPosition();
			if ((attackPos.x == myPos.x && attackPos.y == myPos.y) && (tgtPos.x != myPos.x || tgtPos.y != myPos.y)) {
				// Compute a one-cell nudge toward the target (fallback)
				double ddx = tgtPos.x - myPos.x;
				double ddy = tgtPos.y - myPos.y;
				double len = sqrt(ddx * ddx + ddy * ddy);
				if (len > 0.0) {
					int nudgeX = myPos.x + (int)round((ddx / len) * 1.0);
					int nudgeY = myPos.y + (int)round((ddy / len) * 1.0);

					// clamp and validate
					if (nudgeX < 0) nudgeX = 0; if (nudgeX >= MSZ) nudgeX = MSZ - 1;
					if (nudgeY < 0) nudgeY = 0; if (nudgeY >= MSZ) nudgeY = MSZ - 1;

					if (map[nudgeY][nudgeX] != STONE && map[nudgeY][nudgeX] != WATER && map[nudgeY][nudgeX] != TREE
						&& !IsCellOccupied(nudgeX, nudgeY, all_soldiers))
					{
						attackPos = { nudgeX, nudgeY };
					}
				}
			}

			// Move if we aren't already moving
			if ((attackPos.x != myPos.x || attackPos.y != myPos.y) && (path.empty() || !isMoving)) {
				SetTarget(attackPos.x, attackPos.y, map, securityMap);
			}
			else if (attackPos.x == myPos.x && attackPos.y == myPos.y) {
				// If still cannot move, switch to scanning so unit doesn't freeze forever.
				currentInternalState = STATE_SCANNING_AREA;
				scan_timer = 0;
			}
		}
	}
}

void Warrior::HandleFindingCover(int map[MSZ][MSZ], double securityMap[MSZ][MSZ],
	const std::vector<Soldier*>& all_soldiers,
	std::vector<Bullet*>& active_bullets)
{
	// 1. Defensive Fire Logic
	if (shoot_cooldown_timer == 0)
	{
		Soldier* enemy = FindClosestVisibleEnemy(all_soldiers);
		
		if (enemy)
		{
			// FIX: Check if we actually have a clear shot path before shooting.
			// Even if the enemy is "visible", a tree might block the center-to-center bullet path.
			if (CheckFirePath(x + 0.5, y + 0.5, enemy->getX() + 0.5, enemy->getY() + 0.5, map))
			{
				Bullet* b = Shoot(enemy);
				if (b)
				{
					b->setIsMoving(true);
					active_bullets.push_back(b);
				}
				// Shoot slower when in cover (defensive fire)
				shoot_cooldown_timer = WARRIOR_SHOOT_COOLDOWN_FRAMES * 2;
			}
		}
	}

	if (!commander || !commander->IsAlive()) return; // Safety check

	auto& reserved = commander->GetReservedDefendSpots();
	Point2D myPos = getPosition();
	Point2D threat = commander->getTargetLocation(); 

	// 2. Check if current cover is compromised
	if (hasDefensePosition && lastHealthInCover != -1 && health < lastHealthInCover)
	{
		hasDefensePosition = false;
		lastHealthInCover = -1;
		path.clear(); 
	}

	// 3. Find a new spot if we don't have one
	if (!hasDefensePosition)
	{
		Point2D found = FindNearestCover(myPos, threat, reserved, map);

		if (found.x != -1)
		{
			defensePosition = found;
			reserved.push_back(found); 
			hasDefensePosition = true;
			lastHealthInCover = health;

			// Start moving there
			SetTarget(defensePosition.x, defensePosition.y, map, securityMap);
			return;
		}

		// If no cover found, retreat away from threat
		if (threat.x != -1)
		{
			double dx = myPos.x - threat.x;
			double dy = myPos.y - threat.y;
			double len = sqrt(dx * dx + dy * dy);

			if (len > 0) {
				int retreatX = (int)(myPos.x + (dx / len) * 5);
				int retreatY = (int)(myPos.y + (dy / len) * 5);

				if (retreatX < 0) retreatX = 0; if (retreatX >= MSZ) retreatX = MSZ - 1;
				if (retreatY < 0) retreatY = 0; if (retreatY >= MSZ) retreatY = MSZ - 1;

				if (path.empty() || !isMoving) {
					SetTarget(retreatX, retreatY, map, securityMap);
				}
			}
		}
		return;
	}

	// 4. Move towards the chosen cover
	double dist = hypot(myPos.x - defensePosition.x, myPos.y - defensePosition.y);

	if (dist > 1.5)
	{
		if (path.empty() || !isMoving)
			SetTarget(defensePosition.x, defensePosition.y, map, securityMap);
		return;
	}

	// 5. We are IN POSITION (Holding the line)
	lastHealthInCover = health; 
}

Point2D Warrior::CalculateAttackPosition(Soldier* target, int map[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers) {
	if (!target || !target->IsAlive()) return getPosition();

	Point2D targetPos = target->getPosition();
	Point2D currentPos = getPosition();

	auto& reserved = commander->GetReservedAttackSpots();

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
		reserved.push_back(bestShootingPos);
		return bestShootingPos;
	}

	if (bestChasePos.x != -1) {
		reserved.push_back(bestChasePos);
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
		reserved.push_back(bestFlankPos);
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
		reserved.push_back(moveToward);
		return moveToward;
	}

	return currentPos;
}

bool Warrior::IsCellOccupied(int x, int y, const std::vector<Soldier*>& all_soldiers) {
	for (Soldier* s : all_soldiers) {
		if (!s->IsAlive()) continue;
		// Don't check against self
		if (s == this) continue;
		Point2D pos = s->getPosition();
		if ((int)pos.x == x && (int)pos.y == y) return true;
	}
	return false;
}

