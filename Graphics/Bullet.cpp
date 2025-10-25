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
	double currentX = x;
	double currentY = y;
	double distance_traveled = 0;

	// Keep simulating the bullet's path as long as it's within its range
	while (distance_traveled < B_RANGE)
	{
		// Move one step forward
		currentX += B_SPEED * dirX;
		currentY += B_SPEED * dirY;

		// Check for obstacles or map boundaries
		if (currentX < 0 || currentX >= MSZ || currentY < 0 || currentY >= MSZ ||
			map[(int)currentY][(int)currentX] == STONE || map[(int)currentY][(int)currentX] == TREE)
		{
			break; // Stop the loop if the bullet hits something
		}

		// If the path is clear, update the security map at this point
		securityMap[(int)currentY][(int)currentX] += SECURITY;

		// Recalculate the total distance traveled from the origin
		double dx = currentX - originX;
		double dy = currentY - originY;
		distance_traveled = sqrt(dx * dx + dy * dy);
	}
}
