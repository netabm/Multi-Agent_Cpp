#include "Commander.h"
#include "glut.h"


Commander::Commander(double start_x, double start_y, int team_id) : Soldier(start_x, start_y, team_id)
{
	currentCommand = IDLE;
	for (int i = 0; i < MSZ; ++i) {
		for (int j = 0; j < MSZ; ++j) {
			team_visibility[i][j] = false;
		}
	}
}

void Commander::Show()
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

	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'C');
}

void Commander::CheckAmmo(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!isAlive) return;

	SupplySoldier* available_supply = nullptr;

	// Find an idle SupplySoldier
	for (Soldier* s : team_soldiers) {
		SupplySoldier* supply_ptr = dynamic_cast<SupplySoldier*>(s);
		if (supply_ptr && supply_ptr->IsAlive() && supply_ptr->getSupportState() == SUPPORT_IDLE) {
			available_supply = supply_ptr;
			break;
		}
	}

	if (!available_supply) return; // no idle supply soldier

	// Find a warrior that needs ammo
	for (Soldier* s : team_soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive() && w->getAmmo() < 10) {
			available_supply->AssignSupportTask(w);
			return; // only assign one warrior per frame
		}
	}
}

void Commander::CheckHealth(const std::vector<Soldier*>& team_soldiers,
	int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!isAlive) return;

	Medic* availableMedic = nullptr;
	for (Soldier* s : team_soldiers) {
		Medic* m = dynamic_cast<Medic*>(s);
		if (m && m->IsAlive() && m->getSupportState() == SUPPORT_IDLE) {
			availableMedic = m;
			break;
		}
	}
	if (!availableMedic) return;

	for (Soldier* s : team_soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive() && w->getHealth() < 25) {
			availableMedic->AssignSupportTask(w);
			return;
		}
	}
}

void Commander::Update(const std::vector<Soldier*>& team_soldiers, const std::vector<Soldier*>& all_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	targetLocation = { -1, -1 };
	if (!isAlive) return;

	CalculateTeamVisibility(team_soldiers, getTeam(), map, team_visibility);
	CheckAmmo(team_soldiers, map, securityMap);
	CheckHealth(team_soldiers, map, securityMap);

	// Analyze WARRIOR health status (ignore support/commander)
	int warriorCount = 0;
	int totalWarriorHealth = 0;
	int lowHealthWarriors = 0; // Warriors below 30 health
	int criticalHealthWarriors = 0; // Warriors below 15 health

	for (Soldier* s : team_soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive()) {
			warriorCount++;
			int health = w->getHealth();
			totalWarriorHealth += health;

			if (health < 30) lowHealthWarriors++;
			if (health < 15) criticalHealthWarriors++;
		}
	}

	double avgWarriorHealth = warriorCount > 0 ? (double)totalWarriorHealth / warriorCount : 100.0;
	double lowHealthRatio = warriorCount > 0 ? (double)lowHealthWarriors / warriorCount : 0.0;

	// Detect visible enemies
	std::vector<Soldier*> visibleEnemies;
	for (Soldier* s : all_soldiers) {
		if (s->getTeam() != team && s->IsAlive()) {
			Point2D pos = s->getPosition();
			if (team_visibility[pos.y][pos.x]) {
				visibleEnemies.push_back(s);
			}
		}
	}
	bool enemyVisible = !visibleEnemies.empty();

	static CommandType previousCommand = IDLE;
	static Point2D previousTarget = { -1, -1 };

	// Only give commands when enemies are visible
	if (enemyVisible) {
		// DEFEND if any of these conditions are met:
		// 1. More than 50% of warriors are low health (< 30)
		// 2. Average warrior health below 40
		// 3. Any warrior is critically wounded (< 15) - fall back to protect them
		bool shouldDefend = (lowHealthRatio > 0.5) ||
			(avgWarriorHealth < 40) ||
			(criticalHealthWarriors > 0);

		if (shouldDefend) {
			currentCommand = DEFEND;
		}
		else {
			currentCommand = ATTACK;
		}

		// Calculate target location (enemy centroid)
		double avgX = 0, avgY = 0;
		for (Soldier* e : visibleEnemies) {
			avgX += e->getX();
			avgY += e->getY();
		}
		avgX /= visibleEnemies.size();
		avgY /= visibleEnemies.size();
		targetLocation = { (int)avgX, (int)avgY };

		// Only broadcast the command if it changed (reduces SetCommand spam)
		if (currentCommand != previousCommand || !(previousTarget == targetLocation)) {
			for (Soldier* s : team_soldiers) {
				if (!s->IsAlive()) continue;
				Warrior* w = dynamic_cast<Warrior*>(s);
				if (!w) continue;

				auto state = w->getCurrentInternalState();

				if (state == STATE_ENGAGING && currentCommand == DEFEND) {
					w->SetCommand(currentCommand, targetLocation);
				}
				if (state == STATE_IDLE || state == STATE_SCANNING_AREA) {
					w->SetCommand(currentCommand, targetLocation);
				}
			}
			previousCommand = currentCommand;
			previousTarget = targetLocation;
		}
	}
	else {
		// No enemies visible: clear outstanding commands so warriors revert to autonomous patrol
		if (previousCommand != IDLE || !(previousTarget == Point2D{ -1, -1 })) {
			currentCommand = IDLE;
			defend_reserved_spots.clear();
			targetLocation = { -1, -1 };
			for (Soldier* s : team_soldiers) {
				if (!s->IsAlive()) continue;
				Warrior* w = dynamic_cast<Warrior*>(s);
				if (!w) continue;

				w->SetCommand(IDLE, targetLocation);
			}
			previousCommand = IDLE;
			previousTarget = targetLocation;
		}
	}
}