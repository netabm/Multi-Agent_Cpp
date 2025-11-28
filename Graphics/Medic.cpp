#include "Medic.h"
#include "glut.h"

Point2D MedicalWarehouse0 = { 8 , 2 };
Point2D MedicalWarehouse1 = { MSZ - 3, MSZ - 3 };

void Medic::Show()
{
	Soldier::Show();

	if (!isAlive) return;

	if (team == 0)
	{
		glColor3d(0.8, 0.2, 0.8); // Purple
	}
	else
	{
		glColor3d(1.0, 0.5, 0.0); // Orange
	}

	glBegin(GL_POLYGON);
	glVertex2d(x, y);
	glVertex2d(x, y + 1);
	glVertex2d(x + 1, y + 1);
	glVertex2d(x + 1, y);
	glEnd();

	glColor3d(0, 0, 0); 

	glRasterPos2d(x + 0.1, y + 0.1);

	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'M');
}