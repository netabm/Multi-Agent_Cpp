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

void Commander::UpdateVisibility(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ])
{
	// 1. Reset the team's visibility map
	for (int i = 0; i < MSZ; i++) for (int j = 0; j < MSZ; j++) team_visibility[i][j] = false;

	// 2. Loop through every soldier on the team
	for (Soldier* s : team_soldiers)
	{
		// 3. Calculate what this individual soldier can see
		CalculateVisibility(s, map, s->visibility);

		// 4. Merge the soldier's view into the team's map
			for (int i = 0; i < MSZ; i++) {
				for (int j = 0; j < MSZ; j++) {
					if (s->visibility[i][j]) {
						team_visibility[i][j] = true;
					}
				}
			}
	}
}

//void Commander::CheckAmmo(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
//{
//    if (!isAlive) return;
//
//    SupplySoldier* supply = nullptr;
//    for (Soldier* s : team_soldiers) {
//        supply = dynamic_cast<SupplySoldier*>(s);
//        if (supply && supply->IsAlive()) {
//            break; // Found an alive Supply Soldier
//        }
//    }
//
//    for (Soldier* s : team_soldiers) {
//        Warrior* w = dynamic_cast<Warrior*>(s);
//        if (w && w->IsAlive() && w->getAmmo() < 10) {
//            supply->GiveAmmo(w, map, securityMap);
//        }
//    }
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



void Commander::Update(const std::vector<Soldier*>& team_soldiers, const std::vector<Soldier*>& all_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
    if (!isAlive) return;

    // Update the team's visibility map
    UpdateVisibility(team_soldiers, map);

    // Check for ammo
    CheckAmmo(team_soldiers, map, securityMap);

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

    // Decide command
    if (!enemyVisible) {
        currentCommand = MOVE; // reposition / explore
    }
    else {
        if (avgHealth < 40) {
            printf("team %d health under 40\n", team);
            currentCommand = DEFEND; // hold position if team is weak
        }
        else if (aliveCount >= (int)visibleEnemies.size()) {
            currentCommand = ATTACK; // favorable odds
        }
        else {
            currentCommand = ATTACK; // favorable odds
            //currentCommand = DEFEND; // otherwise play safe
        }
    }

    // Determine target location
    if (currentCommand == ATTACK && enemyVisible) {
        double avgX = 0, avgY = 0;
        for (Soldier* e : visibleEnemies) {
            avgX += e->getX();
            avgY += e->getY();
        }
        avgX /= visibleEnemies.size();
        avgY /= visibleEnemies.size();
        targetLocation = { (int)avgX, (int)avgY };
    }
    else if (currentCommand == MOVE) {
        double avgX = 0, avgY = 0;
        for (Soldier* s : team_soldiers) {
            if (s->IsAlive()) {
                avgX += s->getX();
                avgY += s->getY();
            }
        }
        if (aliveCount > 0) {
            avgX /= aliveCount;
            avgY /= aliveCount;
            targetLocation = { (int)avgX, (int)avgY };
        }
    }
    // DEFEND: optionally keep last position

    // Give commands only to Warriors
    for (Soldier* s : team_soldiers) {
        if (!s->IsAlive()) continue;

        Warrior* w = dynamic_cast<Warrior*>(s);
        if (w) {
            w->SetCommand(currentCommand, targetLocation);
        }
    }
}
