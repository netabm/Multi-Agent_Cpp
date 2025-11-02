#include "Bullet.h"
#include <math.h>
#include <cmath>
#include "glut.h"

Bullet::Bullet(double xPos, double yPos, double angle, Soldier* Shooter)
{
	shooter = Shooter;
	x = xPos;
	y = yPos;
	originX = xPos;
	originY = yPos;
	dirX = cos(angle);
	dirY = sin(angle);
	isMoving = false;
}

void Bullet::Move(int map[MSZ][MSZ])
{
	double tmpX = x, tmpY = y;
	double distance_traveled;

	if (isMoving)
	{
		tmpX += B_SPEED * dirX;
		tmpY += B_SPEED * dirY;
		
		double dx = tmpX - originX;
		double dy = tmpY - originY;

		// Euclidean distance
		distance_traveled = sqrt(pow(dx, 2) + pow(dy, 2));

		if (B_RANGE > distance_traveled) {
			if (tmpX >= 0 && tmpX < MSZ && tmpY >= 0 && tmpY < MSZ && map[(int)tmpY][(int)tmpX] != STONE && map[(int)tmpY][(int)tmpX] != TREE)
			{
				x = tmpX;
				y = tmpY;
			}
			else isMoving = false;
		}
		else isMoving = false;
	}
}

void Bullet::Show()
{
	glColor3d(1, 0, 0);
	glBegin(GL_POLYGON);
	glVertex2d(x - 0.5, y);
	glVertex2d(x , y+ 0.5);
	glVertex2d(x + 0.5, y);
	glVertex2d(x , y- 0.5);
	glEnd();
}

void Bullet::UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	int cx = (int)x;
	int cy = (int)y;

	// Only update if the bullet has entered a new cell
	if (cx == lastCellX && cy == lastCellY)
		return;

	lastCellX = cx;
	lastCellY = cy;

	// Skip invalid or blocked cells
	if (cx < 0 || cx >= MSZ || cy < 0 || cy >= MSZ)
		return;
	if (map[cy][cx] == STONE || map[cy][cx] == TREE)
		return;

	// Update the security map once for this cell
	securityMap[cy][cx] += SECURITY;
}

