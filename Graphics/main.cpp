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
const double PI = 3.14;

int map[MSZ][MSZ] = { 0 };
double securityMap[MSZ][MSZ] = { 0 };

std::vector<Soldier*> soldiers;
std::vector<Bullet*> active_bullets;
std::vector<Grenade*> active_grenades;

Bullet* pb = nullptr;
Grenade* pg = nullptr;
Warrior* w01 = nullptr;
Commander* c0 = nullptr;

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
	// 1. Set the entire map to be empty ground first
	for (int i = 0; i < MSZ; i++)
		for (int j = 0; j < MSZ; j++)
			map[i][j] = 0;

	// Warehouses
	//team 0
	map[2][2] = AMMO_WAREHOUSE;
	map[8][2] = MEDICAL_WAREHOUSE;

	//team 1
	map[MSZ - 9][MSZ - 3] = AMMO_WAREHOUSE;
	map[MSZ - 3][MSZ - 3] = MEDICAL_WAREHOUSE;

	// bottom-left cluster
	//map[6][6] = STONE; map[7][6] = STONE; map[6][7] = STONE;
	// bottom-center cluster
	map[18][6] = STONE; map[19][6] = STONE; map[18][7] = STONE; map[20][7] = STONE;
	// bottom-right cluster
	map[42][6] = STONE; map[43][6] = STONE; map[42][7] = STONE;
	// mid-left cluster
	map[6][22] = STONE; map[7][22] = STONE; map[6][23] = STONE;
	// mid-right cluster
	map[42][24] = STONE; map[43][24] = STONE; map[44][24] = STONE;
	// top-left cluster
	map[6][42] = STONE; map[7][42] = STONE; map[6][43] = STONE;
	// top-center cluster
	map[24][42] = STONE; map[25][42] = STONE; map[24][43] = STONE;
	// top-right cluster
	//map[42][42] = STONE; map[43][42] = STONE; map[42][43] = STONE;

	// Tree patches (passable but block LOS)
	// cluster A
	map[14][10] = TREE; map[16][11] = TREE; map[15][12] = TREE;
	// cluster B
	map[10][28] = TREE; map[12][26] = TREE; map[11][27] = TREE;
	// cluster C
	map[30][14] = TREE; map[32][15] = TREE; map[31][16] = TREE;
	// cluster D
	map[36][34] = TREE; map[35][36] = TREE; map[34][35] = TREE;
	// small scatter
	map[22][8] = TREE; map[23][9] = TREE; map[21][9] = TREE;

	// Water ponds (impassable for movement, vision passes)
	// central pond
	for (int i = 23; i <= 26; ++i) for (int j = 20; j <= 29; ++j) map[i][j] = WATER;
	// small top pond
	map[12][18] = WATER; map[12][19] = WATER; map[13][19] = WATER;
	// small bottom pond
	map[37][30] = WATER; map[36][31] = WATER; map[37][32] = WATER;

	// Leave some open corridors so A* produces different routes
}

void init()
{
	// keep srand but map deterministic above
	srand((unsigned)time(0));
	glClearColor(0, 0.5, 0.8, 0);// color of window background
	glOrtho(0, MSZ, 0, MSZ, -1, 1); // set the coordinates system

	glEnable(GL_BLEND); // Enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set blend function

	InitMap();

	// Team 0 (bottom-left region) - spread out
	soldiers.push_back(new Commander(6, 6, 0));
	c0 = dynamic_cast<Commander*>(soldiers.back());

	soldiers.push_back(new Warrior(4, 8, 0));
	soldiers.push_back(new Warrior(8, 4, 0));
	soldiers.push_back(new SupplySoldier(12, 6, 0));

	// Team 1 (top-right region) - spread out
	soldiers.push_back(new Commander(43, 43, 1));
	soldiers.push_back(new Warrior(41, 45, 1));
	soldiers.push_back(new Warrior(45, 41, 1));
}

void CreateSecurityMap()
{
	int num_grenades = 10;
	int i;

	for (i = 0;i < num_grenades;i++)
	{
		//pg = new Grenade(MSZ / 2 - 10 + rand() % 20, MSZ / 2 - 10 + rand() % 20);
		//pg->UpdateSecurityMap(map, securityMap);
		// Keep grenade to show explosion/security visualization if desired
		//active_grenades.push_back(pg);
	}
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
			case 0: // Empty
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
				// Clamp to [0,1] defensively
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
		//printf("show pls bullet\n");
		b->Show();
	}

	for (Grenade* g : active_grenades) {
		g->Show();
	}

	glutSwapBuffers(); // show all
}

void displaySecurityMap()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer

	// 1. Draw the security map background
	DrawSecurityMap();

	// 2. Draw all soldiers on top of the map
	for (Soldier* s : soldiers) {
		s->Show();
	}

	for (Bullet* b : active_bullets) {
		b->Show();
	}

	// 3. Draw all active grenades on top of the map
	for (Grenade* g : active_grenades) {
		g->Show();
	}

	glutSwapBuffers(); // show all
}

void idle()
{
	// 1. Let commanders make decisions (so they can assign orders)
	for (Soldier* s : soldiers) {
		Commander* cmd = dynamic_cast<Commander*>(s);
		if (cmd && cmd->IsAlive()) {
			auto team = GetTeamSoldiers(cmd->getTeam());
			cmd->Update(team, soldiers, map, securityMap);
		}
	}

	// 2. Update soldiers: Warriors process orders (ai), others just move
	for (Soldier* s : soldiers) {
		Warrior* w = dynamic_cast<Warrior*>(s);
		if (w && w->IsAlive()) {
			// is commander alive for this team?
			bool commanderAlive = false;
			for (Soldier* t : soldiers) {
				Commander* c = dynamic_cast<Commander*>(t);
				if (c && c->getTeam() == w->getTeam() && c->IsAlive()) { commanderAlive = true; break; }
			}
			w->Update(commanderAlive, soldiers, active_bullets, active_grenades, map, securityMap);
		}
		else if (SupportSoldier* sp = dynamic_cast<SupportSoldier*>(s)) {
			sp->Update(map, securityMap);
		}
	}

	for (Soldier* s : soldiers) {
		if (s->IsAlive()) {
			s->Move(map);
		}
	}
	// 3. Update projectiles and grenades
	for (Grenade* g : active_grenades) {
		g->Explode(map);
	}
	for (Bullet* b : active_bullets) {
		b->Move(map);
	}

	// 4. Collision: bullets -> soldiers
	for (Bullet* bullet : active_bullets) {
		if (bullet->getIsMoving()) {
			//printf("bullet is here\n");
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

	// 5. Collision: grenade bullets -> soldiers
	for (Grenade* grenade : active_grenades) {
		Bullet** bullets_from_grenade = grenade->getBullets();
		for (int i = 0; i < NUM_BULLETS; i++) {
			if (bullets_from_grenade[i]->getIsMoving()) {
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

	// 6. Remove stopped bullets
	active_bullets.erase(std::remove_if(active_bullets.begin(), active_bullets.end(),
		[](Bullet* b) {
			if (!b->getIsMoving()) {
				delete b; // Free memory
				return true;
			}
			return false;
		}), active_bullets.end());

	// 7. Remove dead soldiers
	soldiers.erase(std::remove_if(soldiers.begin(), soldiers.end(),
		[](Soldier* s) {
			if (!s->IsAlive()) {
				delete s; // Free memory
				return true;
			}
			return false;
		}), soldiers.end());

	// 8. Throttled commander visibility update (optional - you already call Update above)
	// (kept if you want separate visibility-only frequency, otherwise redundant)
	/*static int frameCounter = 0;
	if (++frameCounter >= 8) {
		frameCounter = 0;
		for (Soldier* s : soldiers) {
			Commander* cmd = dynamic_cast<Commander*>(s);
			if (cmd) {
				auto team = GetTeamSoldiers(cmd->getTeam());
				cmd->UpdateVisibility(team, map);
			}
		}
	}*/

	glutPostRedisplay();
}

void MouseClick(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		int target_x = MSZ * x / (double)W;
		int target_y = MSZ * (H - y - 1) / (double)H;

		Grenade* pg = new Grenade(target_x, target_y, w01);
		pg->setIsExploded(true);
		pg->UpdateSecurityMap(map, securityMap);
		active_grenades.push_back(pg);

		if (!soldiers.empty())
		{
			// send target to first soldier (for testing)
			soldiers[0]->SetTarget(target_x, target_y, map, securityMap);
		}
	}
}

void menu(int choice)
{
	switch (choice)
	{
	case 1:
		CreateSecurityMap();
		break;
	case 2:
		glutDisplayFunc(displaySecurityMap);
		break;
	case 3:
		glutDisplayFunc(display);
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

	// add menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Create Security Map", 1);
	glutAddMenuEntry("Show Security Map", 2);
	glutAddMenuEntry("Show Regular Map", 3);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	init();

	glutMainLoop();

	return 0;
}