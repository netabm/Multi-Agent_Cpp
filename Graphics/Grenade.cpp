#include "Grenade.h"
#include "glut.h"

// Constructor now sets up travel
Grenade::Grenade(double startX, double startY, double targetX, double targetY, Soldier* soldier)
{
	shooter = soldier;
	x = startX;
	y = startY;
	target_x = targetX;
	target_y = targetY;

	isTraveling = true;
	isExploded = false;

	// Calculate travel vector
	double dx = target_x - x;
	double dy = target_y - y;
	double dist = hypot(dx, dy);

	if (dist > 0) {
		travel_dx = (dx / dist) * GRENADE_SPEED;
		travel_dy = (dy / dist) * GRENADE_SPEED;
	}
	else {
		travel_dx = 0;
		travel_dy = 0;
	}

	// Init bullets to null. They won't be created until explosion.
	for (int i = 0; i < NUM_BULLETS; i++)
	{
		bullets[i] = nullptr;
	}
}

// Destructor to prevent memory leaks
Grenade::~Grenade()
{
	for (int i = 0; i < NUM_BULLETS; i++)
	{
		if (bullets[i] != nullptr)
			delete bullets[i];
	}
}

// This function is called internally when the grenade reaches its target
void Grenade::TriggerExplosion()
{
	isTraveling = false;
	isExploded = true;

	// Now create the shrapnel at the target location (x, y)
	double angle, teta = (360.0 / NUM_BULLETS) * PI / 180.0;
	for (int i = 0, angle = 0; i < NUM_BULLETS; i++, angle += teta)
	{
		bullets[i] = new Bullet(x, y, angle, shooter);
		bullets[i]->setIsMoving(true); // Activate the shrapnel
	}
}

// This is the new public "explode now" function for debug/mouse
void Grenade::ExplodeNow()
{
	x = target_x; // Instantly move to target
	y = target_y;
	TriggerExplosion();
}

// Update handles travel AND explosion shrapnel
void Grenade::Update(int map[MSZ][MSZ])
{
	if (isTraveling)
	{
		// Check distance to target
		double dist_to_target = hypot(target_x - x, target_y - y);

		if (dist_to_target < GRENADE_SPEED)
		{
			// Arrived: snap to target and explode
			x = target_x;
			y = target_y;
			TriggerExplosion();
		}
		else
		{
			// Still traveling: move the grenade
			x += travel_dx;
			y += travel_dy;
		}
	}
	else if (isExploded)
	{
		// We are exploded, so update the shrapnel
		for (int i = 0; i < NUM_BULLETS; i++)
		{
			if (bullets[i]) // Check if bullet exists
				bullets[i]->Move(map);
		}
	}
}

void Grenade::Show()
{
	if (isTraveling)
	{
		// Draw the grenade itself (a small black square)
		glColor3d(0.1, 0.1, 0.1);
		glBegin(GL_POLYGON);
		glVertex2d(x - 0.2, y - 0.2);
		glVertex2d(x + 0.2, y - 0.2);
		glVertex2d(x + 0.2, y + 0.2);
		glVertex2d(x - 0.2, y + 0.2);
		glEnd();
	}
	else if (isExploded)
	{
		// Draw the shrapnel
		for (int i = 0; i < NUM_BULLETS; i++)
		{
			if (bullets[i] && bullets[i]->getIsMoving())
				bullets[i]->Show();
		}
	}
}

void Grenade::UpdateSecurityMap(int map[MSZ][MSZ], double securityMap[MSZ][MSZ])
{
	// Only update security map if exploded
	if (isExploded)
	{
		for (int i = 0; i < NUM_BULLETS; i++)
		{
			if (bullets[i])
				bullets[i]->UpdateSecurityMap(map, securityMap);
		}
	}
}