#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include "glut.h"
#include "Definitions.h"
#include "Bullet.h"
#include "Grenade.h"
#include "Warrior.h"
#include "Commander.h"

const int W = 600;
const int H = 600; // window sizes
int teamToShow = -1; // -1 = no team, 0 = team 0, 1 = team 1
bool gameOver = false;
int winningTeam = -1; // -1 = no winner yet


int map[MSZ][MSZ] = { 0 };
double securityMap[MSZ][MSZ] = { 0 };

std::vector<Soldier*> soldiers;
std::vector<Bullet*> active_bullets;
std::vector<Grenade*> active_grenades;

Bullet* pb = nullptr;
Grenade* pg = nullptr;
Warrior* w01 = nullptr;
Commander* c0 = nullptr;
Commander* c1 = nullptr;

std::vector<Soldier*> GetTeamSoldiers(int team_id)
{
	std::vector<Soldier*> team;
	for (Soldier* s : soldiers) {
		if (s->getTeam() == team_id) team.push_back(s);
	}
	return team;
}

void InitMap()
{
	for (int i = 0; i < MSZ; i++)
		for (int j = 0; j < MSZ; j++)
			map[i][j] = 0;

	// WAREHOUSES
	// Team 0 (Purple)
	map[2][2] = AMMO_WAREHOUSE;
	map[8][2] = MEDICAL_WAREHOUSE;
	// Team 1 (Orange)
	map[MSZ - 9][MSZ - 3] = AMMO_WAREHOUSE;
	map[MSZ - 3][MSZ - 3] = MEDICAL_WAREHOUSE;

	// STONE FORMATIONS
	// Bottom-center scattered rocks
	map[18][6] = STONE; map[19][6] = STONE; map[20][7] = STONE;
	map[22][9] = STONE; map[23][10] = STONE;
	// Bottom-right rock cluster
	map[40][8] = STONE; map[41][8] = STONE; map[42][8] = STONE;
	map[40][9] = STONE; map[42][9] = STONE;
	map[41][10] = STONE;
	// Mid-left mountain range
	for (int j = 18; j <= 22; j++) map[7][j] = STONE;
	map[8][20] = STONE; map[9][20] = STONE;
	map[8][19] = STONE; map[9][21] = STONE;
	// Mid-right defensive wall
	for (int j = 22; j <= 28; j++) map[42][j] = STONE;
	map[41][24] = STONE; map[41][26] = STONE;
	map[43][23] = STONE; map[43][27] = STONE;
	// Top-left rock pile
	map[6][38] = STONE; map[7][38] = STONE; map[8][38] = STONE;
	map[6][39] = STONE; map[8][39] = STONE;
	map[7][40] = STONE;
	// Top-center scattered formation
	map[20][40] = STONE; map[21][41] = STONE; map[22][40] = STONE;
	map[24][42] = STONE; map[25][43] = STONE;
	// Top-right corner rocks
	map[40][40] = STONE; map[43][42] = STONE;
	map[41][40] = STONE; map[41][42] = STONE;
	map[40][41] = STONE; map[41][41] = STONE;
	map[40][42] = STONE; map[41][43] = STONE;
	map[42][42] = STONE;

	// TREE PATCHES
	// Bottom-left forest
	map[12][8] = TREE; map[13][8] = TREE; map[14][8] = TREE;
	map[11][9] = TREE; map[12][9] = TREE; map[13][9] = TREE; map[14][9] = TREE;
	map[12][10] = TREE; map[13][10] = TREE; map[15][10] = TREE;
	map[14][11] = TREE; map[16][11] = TREE;
	map[15][12] = TREE;
	// Bottom-center tree line
	map[27][8] = TREE; map[28][8] = TREE; map[29][9] = TREE;
	map[30][10] = TREE; map[31][11] = TREE;
	// Bottom-right sparse trees
	map[35][7] = TREE; map[36][9] = TREE; map[37][8] = TREE;
	map[38][10] = TREE;
	// Mid-left dense forest
	map[10][26] = TREE; map[11][26] = TREE; map[12][26] = TREE;
	map[9][27] = TREE; map[10][27] = TREE; map[11][27] = TREE; map[12][27] = TREE;
	map[10][28] = TREE; map[11][28] = TREE; map[13][28] = TREE;
	map[11][29] = TREE; map[12][29] = TREE;
	// Mid-center jungle patch
	map[18][18] = TREE; map[19][18] = TREE; map[20][18] = TREE;
	map[17][19] = TREE; map[18][19] = TREE; map[19][19] = TREE;
	map[18][20] = TREE; map[20][20] = TREE;
	// Mid-right scattered trees
	map[35][20] = TREE; map[36][21] = TREE; map[37][19] = TREE;
	map[38][22] = TREE; map[39][20] = TREE;
	// Top-left tree cluster
	map[10][35] = TREE; map[11][35] = TREE; map[12][36] = TREE;
	map[13][37] = TREE; map[14][36] = TREE;
	// Top-center forest
	map[26][36] = TREE; map[27][36] = TREE; map[28][36] = TREE;
	map[26][37] = TREE; map[27][37] = TREE; map[28][37] = TREE; map[29][37] = TREE;
	map[27][38] = TREE; map[28][38] = TREE;
	// Top-right woodland
	map[33][40] = TREE; map[34][40] = TREE; map[35][40] = TREE;
	map[33][41] = TREE; map[34][41] = TREE; map[36][41] = TREE;
	map[34][42] = TREE; map[35][42] = TREE;
	// Scattered individual trees for ambush points
	map[16][15] = TREE; map[32][14] = TREE;
	map[15][30] = TREE; map[30][32] = TREE;
	map[20][35] = TREE; map[35][25] = TREE;

	// WATER BODIES
	// Central lake
	for (int i = 22; i <= 28; i++) {
		for (int j = 28; j <= 34; j++) {
			map[i][j] = WATER;
		}
	}
	map[21][30] = WATER; map[21][31] = WATER; map[21][32] = WATER;
	map[29][29] = WATER; map[29][30] = WATER; map[29][33] = WATER;
	// Bottom-left pond
	map[12][14] = WATER; map[13][14] = WATER; map[14][14] = WATER;
	map[12][15] = WATER; map[13][15] = WATER; map[14][15] = WATER;
	map[13][16] = WATER;
	// Bottom-right stream
	map[36][12] = WATER; map[37][12] = WATER;
	map[37][13] = WATER; map[38][13] = WATER;
	map[38][14] = WATER; map[39][14] = WATER;
	map[39][15] = WATER;
	// Top-left pool
	map[9][42] = WATER; map[10][42] = WATER; map[11][42] = WATER;
	map[9][43] = WATER; map[10][43] = WATER; map[11][43] = WATER;
	map[10][44] = WATER;
	// Top-right river
	map[43][36] = WATER; map[44][36] = WATER;
	map[43][37] = WATER; map[44][37] = WATER; map[45][37] = WATER;
	map[44][38] = WATER; map[45][38] = WATER;
	map[45][39] = WATER;
	// Mid-left creek
	map[15][22] = WATER; map[16][22] = WATER;
	map[16][23] = WATER; map[17][23] = WATER;
	map[17][24] = WATER;
	// Small scattered puddles
	map[33][10] = WATER; map[34][10] = WATER;
	map[18][33] = WATER; map[19][33] = WATER;
	map[31][39] = WATER; map[32][39] = WATER;
	// Additional tactical water features
	map[5][16] = WATER; map[6][16] = WATER; map[5][17] = WATER;
	map[40][33] = WATER; map[41][33] = WATER; map[41][34] = WATER;
}

void init()
{
	srand((unsigned)time(0));
	glClearColor(0, 0.5, 0.8, 0);
	glOrtho(0, MSZ, 0, MSZ, -1, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	InitMap();

	// Team 0 (bottom-left region)
	soldiers.push_back(new Commander(6, 6, 0));
	c0 = dynamic_cast<Commander*>(soldiers.back());
	soldiers.push_back(new Warrior(4, 8, 0, c0));
	soldiers.push_back(new Warrior(8, 4, 0, c0));
	soldiers.push_back(new SupplySoldier(12, 6, 0, c0));
	soldiers.push_back(new Medic(6, 12, 0, c0));

	// Team 1 (top-right region)
	soldiers.push_back(new Commander(43, 43, 1));
	c1 = dynamic_cast<Commander*>(soldiers.back());
	soldiers.push_back(new Warrior(41, 45, 1, c1));
	soldiers.push_back(new Warrior(45, 41, 1, c1));
	soldiers.push_back(new SupplySoldier(44, 38, 1, c1));
	soldiers.push_back(new Medic(38, 44, 1, c1));
}

void DrawTree(int x, int y)
{
	glColor3d(0, 0.3, 0);
	glBegin(GL_POLYGON);
	glVertex2d(x - 0.5, y);
	glVertex2d(x, y + 0.5);
	glVertex2d(x + 0.5, y);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex2d(x - 0.5, y + 0.5);
	glVertex2d(x, y + 1);
	glVertex2d(x + 0.5, y + 0.5);
	glEnd();
}

void DrawStone(double x, double y)
{
	glColor3d(0.4, 0.4, 0.4);
	glBegin(GL_POLYGON);
	glVertex2d(x + 0.1, y + 0.2);
	glVertex2d(x + 0.5, y + 0.1);
	glVertex2d(x + 0.9, y + 0.3);
	glVertex2d(x + 0.8, y + 0.8);
	glVertex2d(x + 0.4, y + 0.9);
	glVertex2d(x + 0.2, y + 0.7);
	glEnd();
}

void DrawMap()
{
	int i, j;

	for (i = 0; i < MSZ; i++)
		for (j = 0; j < MSZ; j++)
		{
			switch (map[i][j])
			{
			case STONE:
			case TREE:
			case 0:
				glColor3d(0.7, 1, 0.7); // Light Green
				break;
			case WATER:
				glColor3d(0.2, 0.5, 1.0); // Blue
				break;
			case AMMO_WAREHOUSE:
				glColor3d(1.0, 1.0, 0.0); // Yellow
				break;
			case MEDICAL_WAREHOUSE:
				glColor3d(1.0, 0.4, 0.7); // Pink
				break;
			}
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j, i + 1);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j + 1, i);
			glEnd();

			if (map[i][j] == TREE)
			{
				DrawTree(j, i);
			}
			else if (map[i][j] == STONE)
			{
				DrawStone(j, i);
			}
		}
}

void DrawSecurityMap()
{
	int i, j;

	for (i = 0;i < MSZ;i++)
		for (j = 0;j < MSZ;j++)
		{
			switch (map[i][j])
			{
			case 0:
			{
				double v = securityMap[i][j];
				if (v < 0) v = 0; if (v > 1) v = 1;
				glColor3d(1 - v, 1 - v, 1 - v);
			}
			break;
			case STONE:
				glColor3d(0.3, 0.3, 0.3);
				break;
			default:
				glColor3d(0.7, 1, 0.7);
				break;
			}
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j, i + 1);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j + 1, i);
			glEnd();
		}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer
	DrawMap();

	for (Soldier* s : soldiers) {
		s->Show();
	}

	for (Bullet* b : active_bullets) {
		b->Show();
	}

	for (Grenade* g : active_grenades) {
		g->Show();
	}

	if (gameOver)
	{
		const char* msg = winningTeam == 0 ? "TEAM 0 WINS!" : "TEAM 1 WINS!";

		float textLength = glutStrokeLength(GLUT_STROKE_ROMAN, (unsigned char*)msg);

		glPushMatrix();

		glTranslated(MSZ / 2.0, MSZ / 2.0, 0);


		float scaleFactor = (MSZ * 0.8) / textLength;
		glScaled(scaleFactor, scaleFactor, 1.0);

		glTranslated(-textLength / 2.0, -60.0, 0);

		// DRAWING
		glColor3d(0.0, 0.0, 0.0);
		glLineWidth(4.0);

		glPushMatrix();
		glTranslated(10.0, -10.0, 0);
		for (const char* c = msg; *c; c++)
			glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		glPopMatrix();

		glColor3d(1.0, 0.0, 0.0); // Red
		glLineWidth(3.0);

		for (const char* c = msg; *c; c++)
			glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);

		// CLEANUP
		glLineWidth(1.0);
		glPopMatrix();
	}
	glutSwapBuffers(); // show all
}

void displaySecurityMap()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer

	DrawSecurityMap();

	// Draw all soldiers on top of the map
	for (Soldier* s : soldiers) {
		s->Show();
	}

	for (Bullet* b : active_bullets) {
		b->Show();
	}

	// Draw all active grenades on top of the map
	for (Grenade* g : active_grenades) {
		g->Show();
	}

	glutSwapBuffers(); // show all
}

void DrawVisibilityMap()
{
	int i, j;
	// Determine which team's map to show
	bool(*visibility_map)[MSZ] = nullptr;
	Commander* relevant_commander = nullptr;

	if (teamToShow == 0 && c0 != nullptr && c0->IsAlive()) {
		relevant_commander = c0;
		visibility_map = c0->getTeamVisibility();
	}
	else if (teamToShow == 1 && c1 != nullptr && c1->IsAlive()) {
		relevant_commander = c1;
		visibility_map = c1->getTeamVisibility();
	}

	// Get soldiers only for the relevant team to calculate distance
	std::vector<Soldier*> relevant_team_soldiers = GetTeamSoldiers(teamToShow);

	for (i = 0; i < MSZ; i++)
	{
		for (j = 0; j < MSZ; j++)
		{
			if (visibility_map != nullptr && visibility_map[i][j])
			{
				// Calculate Gradient
				double min_dist_sq = MSZ * MSZ * 2.0; // Start with a large squared distance
				// Find squared distance to the closest living soldier on the selected team
				for (Soldier* s : relevant_team_soldiers) { // Use the filtered list
					if (s->IsAlive()) {
						double dx = (s->getX() + 0.5) - (j + 0.5);
						double dy = (s->getY() + 0.5) - (i + 0.5);
						double dist_sq = dx * dx + dy * dy;
						if (dist_sq < min_dist_sq) {
							min_dist_sq = dist_sq;
						}
					}
				}
				// Normalize distance (0 = very close, 1 = far)
				// Use a reasonable max distance for normalization
				double max_vis_dist = sqrt(2.0 * (MSZ / 2.0) * (MSZ / 2.0)); // Approx max distance
				double normalized_dist = sqrt(min_dist_sq) / max_vis_dist;
				normalized_dist = fmax(0.0, fmin(1.0, normalized_dist)); // Clamp between 0 and 1

				double brightness = 1.0 - normalized_dist; // Invert: 1 - close, 0 - far
				glColor3d(0.1 + 0.4 * brightness, 0.4 + 0.4 * brightness, 0.1);
			}
			else {
				glColor3d(0.15, 0.25, 0.15);
			}
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j, i + 1);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j + 1, i);
			glEnd();
		}
	}
}

void DrawSightLines()
{
	if (teamToShow < 0) return;

	glColor3d(0, 0, 0);
	glLineWidth(0.5);

	std::vector<Soldier*> relevant_team_soldiers = GetTeamSoldiers(teamToShow);

	for (Soldier* s : relevant_team_soldiers)
	{
		if (s->IsAlive())
		{
			// Draw lines from center of soldier to map corners
			double soldier_center_x = s->getX() + 0.5;
			double soldier_center_y = s->getY() + 0.5;

			glBegin(GL_LINES);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(0, 0);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(MSZ, 0);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(0, MSZ);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(MSZ, MSZ);
			glEnd();

			glBegin(GL_LINES);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(MSZ / 2.0, 0);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(MSZ / 2.0, MSZ);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(0, MSZ / 2.0);
			glVertex2d(soldier_center_x, soldier_center_y); glVertex2d(MSZ, MSZ / 2.0);
			glEnd();
		}
	}
	glLineWidth(1.0); // Reset line width
}

void displayTeamVisibility()
{
	glClear(GL_COLOR_BUFFER_BIT);
	DrawVisibilityMap();
	for (Soldier* s : soldiers) {
		s->Show();
	}
	DrawSightLines();
	glutSwapBuffers();
}

void CheckVictory()
{
	if (gameOver) return;

	bool team0Alive = false;
	bool team1Alive = false;

	// Check who is alive...
	for (Soldier* s : soldiers)
	{
		if (!s->IsAlive()) continue;
		if (s->getTeam() == 0) team0Alive = true;
		if (s->getTeam() == 1) team1Alive = true;
	}

	if (!team0Alive) { gameOver = true; winningTeam = 1; }
	else if (!team1Alive) { gameOver = true; winningTeam = 0; }

	if (gameOver)
	{
		printf("Team %d Wins!\n", winningTeam);
	}
}


void idle()
{
	if (!gameOver) {
		for (Soldier* s : soldiers) {
			Commander* cmd = dynamic_cast<Commander*>(s);
			if (cmd && cmd->IsAlive()) {
				auto team = GetTeamSoldiers(cmd->getTeam());
				cmd->Update(team, soldiers, map, securityMap);
			}
		}

		for (Soldier* s : soldiers) {
			Warrior* w = dynamic_cast<Warrior*>(s);
			if (w && w->IsAlive()) {
				w->Update(soldiers, active_bullets, active_grenades, map, securityMap);
			}
			else if (SupportSoldier* sp = dynamic_cast<SupportSoldier*>(s)) {
				sp->Update(map, securityMap, soldiers);
			}
		}

		for (Soldier* s : soldiers) {
			if (s->IsAlive()) {
				s->Move(map);
			}
		}
		for (Grenade* g : active_grenades) {
			g->Update(map);
		}
		for (Bullet* b : active_bullets) {
			b->Move(map);
		}

		// Collision: bullets hitting soldiers
		for (Bullet* bullet : active_bullets) {
			if (bullet->getIsMoving()) {
				bullet->UpdateSecurityMap(map, securityMap);
				for (Soldier* soldier : soldiers) {
					if (soldier != bullet->getShooter() && soldier->IsAlive()) {
						double soldier_posX = soldier->getX();
						double soldier_posY = soldier->getY();
						double bullet_posX = bullet->getX();
						double bullet_posY = bullet->getY();

						if (bullet_posX >= soldier_posX && bullet_posX < soldier_posX + 1 && bullet_posY >= soldier_posY && bullet_posY < soldier_posY + 1) {
							soldier->TakeDamage();
							bullet->setIsMoving(false);
							break;
						}
					}
				}
			}
		}

		// Collision: grenade bullets hitting soldiers
		for (Grenade* grenade : active_grenades) {
			Bullet** bullets_from_grenade = grenade->getBullets();
			for (int i = 0; i < NUM_BULLETS; i++) {
				if (bullets_from_grenade[i] && bullets_from_grenade[i]->getIsMoving()) {
					bullets_from_grenade[i]->UpdateSecurityMap(map, securityMap);
					for (Soldier* soldier : soldiers) {
						if (soldier->IsAlive()) {
							double soldier_posX = soldier->getX();
							double soldier_posY = soldier->getY();
							double bullet_posX = bullets_from_grenade[i]->getX();
							double bullet_posY = bullets_from_grenade[i]->getY();

							if (bullet_posX >= soldier_posX && bullet_posX < soldier_posX + 1 && bullet_posY >= soldier_posY && bullet_posY < soldier_posY + 1) {
								soldier->TakeDamage();
								bullets_from_grenade[i]->setIsMoving(false);
								break;
							}
						}
					}
				}
			}
		}

		// Decay security map values
		for (int i = 0; i < MSZ; i++)
			for (int j = 0; j < MSZ; j++)
				securityMap[i][j] *= 0.9999;

		// Remove stopped bullets
		active_bullets.erase(std::remove_if(active_bullets.begin(), active_bullets.end(),
			[](Bullet* b) {
				if (!b->getIsMoving()) {
					delete b; // Free memory
					return true;
				}
				return false;
			}), active_bullets.end());

		// Remove dead soldiers
		soldiers.erase(std::remove_if(soldiers.begin(), soldiers.end(),
			[](Soldier* s) {
				if (!s->IsAlive()) {
					delete s; // Free memory
					return true;
				}
				return false;
			}), soldiers.end());


		CheckVictory();
	}
	glutPostRedisplay();
}

void MouseClick(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		// Calculate map coordinates from mouse pixels
		double click_x = MSZ * x / (double)W;
		double click_y = MSZ * (H - y - 1) / (double)H;

		// Create the grenade at the clicked position.
		Grenade* pg = new Grenade(click_x, click_y, click_x, click_y, nullptr);

		// Force immediate explosion
		pg->ExplodeNow();

		active_grenades.push_back(pg);
	}
}

void menu(int choice)
{
	switch (choice)
	{
	case 1:
		glutDisplayFunc(displaySecurityMap);
		break;
	case 2:
		glutDisplayFunc(display);
		break;
	case 3: 
		teamToShow = 0;
		glutDisplayFunc(displayTeamVisibility);
		break;
	case 4: 
		teamToShow = 1;
		glutDisplayFunc(displayTeamVisibility);
		break;
	}
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");

	glutDisplayFunc(display);
	glutIdleFunc(idle);

	glutMouseFunc(MouseClick);

	glutCreateMenu(menu);
	glutAddMenuEntry("Show Security Map", 1);
	glutAddMenuEntry("Show Regular Map", 2);
	glutAddMenuEntry("Show Team 1 Visibility", 3);
	glutAddMenuEntry("Show Team 2 Visibility", 4);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	init();

	glutMainLoop();

	return 0;
}