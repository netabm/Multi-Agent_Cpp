#include "Grenade.h"
const double PI = 3.14;

Grenade::Grenade(double xPos, double yPos, Soldier* soldier)
{
	shooter = soldier;
	int i;
	double angle, teta = (360/NUM_BULLETS)*PI/180;
	x = xPos;
	y = yPos;

	for (i = 0,angle =0;i < NUM_BULLETS;i++,angle+=teta)
	{
		bullets[i] = new Bullet(x, y, angle, shooter);
	}
	isExploded = false;
}

void Grenade::Explode(int map[MSZ][MSZ])
{
	int i;

	for (i = 0;i < NUM_BULLETS;i++)
	{
		bullets[i]->Move(map);
	}
}

void Grenade::setIsExploded(bool value)
{
	isExploded = value;
	int i;

	for (i = 0;i < NUM_BULLETS;i++)
	{
		bullets[i]->setIsMoving(value);
	}

}

void Grenade::Show()
{
	int i;

	for (i = 0;i < NUM_BULLETS;i++)
	{
		if (bullets[i]->getIsMoving())
			bullets[i]->Show();
	}

}

void Grenade::UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	int i;

	for (i = 0;i < NUM_BULLETS;i++)
	{
		bullets[i]->UpdateSecurityMap(map, securityMap);
	}

}

void Grenade::Move(int map[MSZ][MSZ])
{
	if (isExploded) return;

	double dx = targetX - x;
	double dy = targetY - y;
	double dist = hypot(dx, dy);

	if (dist < 0.5) {
		exploded = true; // reached target
		Explode(map);    // create bullet fragments here
	}
	else {
		x += GRENADE_SPEED * dx / dist;
		y += GRENADE_SPEED * dy / dist;
	}
}

