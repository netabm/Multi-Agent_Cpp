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

	// If no one needs ammo, send idle supply soldier back to warehouse
	for (Soldier* s : team_soldiers)
	{
		SupplySoldier* sup = dynamic_cast<SupplySoldier*>(s);
		if (sup && sup->IsAlive() && sup->getSupportState() == SUPPORT_IDLE)
		{
			sup->ReturnToWarehouse();
		}
	}


}

void Commander::CheckHealth(const std::vector<Soldier*>& team_soldiers,
	int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	if (!isAlive) return;

	// Find an idle medic
	Medic* availableMedic = nullptr;
	for (Soldier* s : team_soldiers) {
		Medic* m = dynamic_cast<Medic*>(s);
		if (m && m->IsAlive() && m->getSupportState() == SUPPORT_IDLE) {
			availableMedic = m;
			break;
		}
	}
	if (!availableMedic) return;

	Warrior* bestLowHP = nullptr;
	Warrior* bestCoverNeed = nullptr;

	// Scan for warriors who need healing
	for (Soldier* s : team_soldiers)
	{
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (!w || !w->IsAlive()) continue;

		int hp = w->getHealth();
		int state = w->getCurrentInternalState();

		// Priority 1: lowest HP warrior <= 25
		if (hp <= 25) {
			if (!bestLowHP || hp < bestLowHP->getHealth())
				bestLowHP = w;
		}

		// Priority 2: warrior finding cover (pick the lowest HP among them)
		if (state == STATE_FINDING_COVER) {
			if (!bestCoverNeed || hp < bestCoverNeed->getHealth())
				bestCoverNeed = w;
		}
	}

	// Assign task by priority
	if (bestLowHP) {
		availableMedic->AssignSupportTask(bestLowHP);
		return;
	}

	if (bestCoverNeed) {
		availableMedic->AssignSupportTask(bestCoverNeed);
		return;
	}

	// If no one needs healing, send idle medic back to warehouse
	for (Soldier* s : team_soldiers)
	{
		Medic* m = dynamic_cast<Medic*>(s);
		if (m && m->IsAlive() && m->getSupportState() == SUPPORT_IDLE)
		{
			m->ReturnToWarehouse();
		}
	}

}


void Commander::Update(const std::vector<Soldier*>& team_soldiers, const std::vector<Soldier*>& all_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	attack_reserved_spots.clear();
	targetLocation = { -1, -1 };
	if (!isAlive) return;

	CalculateTeamVisibility(team_soldiers, getTeam(), map, team_visibility);
	CheckAmmo(team_soldiers, map, securityMap);
	CheckHealth(team_soldiers, map, securityMap);

	// Check if we have a medic
	bool medicAvailable = false;
	for (Soldier* s : team_soldiers) {
		Medic* m = dynamic_cast<Medic*>(s);
		if (m && m->IsAlive()) {
			medicAvailable = true;
			break;
		}
	}

	// Analyze Health
	int warriorCount = 0;
	int totalWarriorHealth = 0;
	int criticalHealthWarriors = 0;

	for (Soldier* s : team_soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive()) {
			warriorCount++;
			int health = w->getHealth();
			totalWarriorHealth += health;

			if (health <= 25) criticalHealthWarriors++;
		}
	}
	double avgWarriorHealth = warriorCount > 0 ? (double)totalWarriorHealth / warriorCount : 0.0;

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

	CommandType previousCommand = IDLE;
	Point2D previousTarget = { -1, -1 };

	if (enemyVisible) {
		// Only defend if we have a Medic and the team is actually hurt
		bool shouldDefend = false;

		if (medicAvailable) {
			shouldDefend = (avgWarriorHealth < 40) || (criticalHealthWarriors > 0);
		}

		if (shouldDefend) {
			currentCommand = DEFEND;
		}
		else {
			currentCommand = ATTACK;
		}

		// Calculate Enemy Center
		double avgX = 0, avgY = 0;
		for (Soldier* e : visibleEnemies) {
			avgX += e->getX();
			avgY += e->getY();
		}
		avgX /= visibleEnemies.size();
		avgY /= visibleEnemies.size();
		targetLocation = { (int)avgX, (int)avgY };

		// Broadcast with Spam Protection
		// Check if command changed OR if target moved significantly (prevent jitter)
		bool commandChanged = (currentCommand != previousCommand);
		bool targetMoved = (abs(previousTarget.x - targetLocation.x) > 2 || abs(previousTarget.y - targetLocation.y) > 2);

		if (commandChanged || targetMoved) {
			for (Soldier* s : team_soldiers) {
				if (!s->IsAlive()) continue;
				Warrior* w = dynamic_cast<Warrior*>(s);
				if (!w) continue;

				auto state = w->getCurrentInternalState();

				// Case A: We are fighting, but need to retreat
				if (state == STATE_ENGAGING && currentCommand == DEFEND) {
					w->SetCommand(currentCommand, targetLocation);
				}
				// Case B: We are doing nothing, so take orders
				else if (state == STATE_IDLE || state == STATE_SCANNING_AREA) {
					w->SetCommand(currentCommand, targetLocation);
				}
				// Case C: We are hiding, but the Commander says attack
				else if (state == STATE_FINDING_COVER && currentCommand == ATTACK) {
					w->SetCommand(currentCommand, targetLocation);
				}
			}
			previousCommand = currentCommand;
			previousTarget = targetLocation;
		}
	}
	else {
		// No enemies visible
		if (previousCommand != IDLE) {
			currentCommand = IDLE;
			defend_reserved_spots.clear();
			targetLocation = { -1, -1 };
			for (Soldier* s : team_soldiers) {
				Warrior* w = dynamic_cast<Warrior*>(s);
				if (w && w->IsAlive()) w->SetCommand(IDLE, targetLocation);
			}
			previousCommand = IDLE;
		}
	}
}