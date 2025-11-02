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

//void Commander::UpdateVisibility(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ])
//{
//	// 1. Reset the team's visibility map
//	for (int i = 0; i < MSZ; i++) for (int j = 0; j < MSZ; j++) team_visibility[i][j] = false;
//
//	// 2. Loop through every soldier on the team
//	for (Soldier* s : team_soldiers)
//	{
//		// 3. Calculate what this individual soldier can see
//		CalculateVisibility(s, map, s->visibility);
//
//		// 4. Merge the soldier's view into the team's map
//			for (int i = 0; i < MSZ; i++) {
//				for (int j = 0; j < MSZ; j++) {
//					if (s->visibility[i][j]) {
//						team_visibility[i][j] = true;
//					}
//				}
//			}
//	}
//}

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

	// Analyze team status
	int aliveCount = 0;
	int totalHealth = 0;
	for (Soldier* s : team_soldiers) {
		if (s->IsAlive()) {
			aliveCount++;
			totalHealth += s->getHealth();
		}
	}
	double avgHealth = aliveCount > 0 ? (double)totalHealth / aliveCount : 0.0;

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
		if (avgHealth < 40) {
			currentCommand = DEFEND;
			// Calculate safe position if needed
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