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
		if (w && w->IsAlive() && w->getHealth() <= 25) {
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

	// --- 1. Check if we even HAVE a Medic ---
	bool medicAvailable = false;
	for (Soldier* s : team_soldiers) {
		Medic* m = dynamic_cast<Medic*>(s);
		if (m && m->IsAlive()) {
			medicAvailable = true;
			break;
		}
	}

	// --- 2. Analyze Health ---
	int warriorCount = 0;
	int totalWarriorHealth = 0;
	int lowHealthWarriors = 0;
	int criticalHealthWarriors = 0;

	for (Soldier* s : team_soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive()) {
			warriorCount++;
			int health = w->getHealth();
			totalWarriorHealth += health;

			// LOGIC CHANGE: Align with Medic's rule (<= 25).
			// If we retreat at 30, but Medic only heals at 25, we get stuck.
			if (health <= 25) lowHealthWarriors++;
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

	if (enemyVisible) {

		// --- 3. Defense Logic ---
		// Only DEFEND if we have a Medic AND the team is actually hurt.
		bool shouldDefend = false;

		if (medicAvailable) {
			shouldDefend = (lowHealthRatio > 0.5) ||
				(avgWarriorHealth < 40) ||
				(criticalHealthWarriors > 0);
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

		// --- 4. Broadcast with Spam Protection ---
		// Check if command changed OR if target moved significantly (prevent jitter)
		bool commandChanged = (currentCommand != previousCommand);
		bool targetMoved = (abs(previousTarget.x - targetLocation.x) > 2 || abs(previousTarget.y - targetLocation.y) > 2);

		if (commandChanged || targetMoved) {
			for (Soldier* s : team_soldiers) {
				if (!s->IsAlive()) continue;
				Warrior* w = dynamic_cast<Warrior*>(s);
				if (!w) continue;

				auto state = w->getCurrentInternalState();

				// KEEPING YOUR IF STATEMENTS, BUT ADDING THE MISSING CASE:

				// Case A: We are fighting, but need to retreat
				if (state == STATE_ENGAGING && currentCommand == DEFEND) {
					w->SetCommand(currentCommand, targetLocation);
				}
				// Case B: We are doing nothing, so take orders
				else if (state == STATE_IDLE || state == STATE_SCANNING_AREA) {
					w->SetCommand(currentCommand, targetLocation);
				}
				// Case C (THE FIX): We are hiding, but the Commander says ATTACK!
				// Without this, hiding warriors ignore the Attack order and stay stuck.
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