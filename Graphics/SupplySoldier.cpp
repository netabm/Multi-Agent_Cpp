#include "SupplySoldier.h"
#include "glut.h"

Point2D AmmoWarehouse0 = { 2,2 };
Point2D AmmoWarehouse1 = { MSZ - 9, MSZ - 3 };

void SupplySoldier::Show()
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

	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'S');
}

//void SupplySoldier::GiveAmmo(Warrior* warrior, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
//	Point2D warehouse = (team == 0) ? AmmoWarehouse0 : AmmoWarehouse1;
//
//	if (getPosition() == warehouse) {
//		visitedWarehouse = true;
//	}
//
//	if (!visitedWarehouse) {
//		printf("go to warehouse\n");
//		this->SetTarget(warehouse.x, warehouse.y, map, securityMap);
//		return;
//	}
//
//	this->SetTarget(warrior->getX(), warrior->getY(), map, securityMap);
//
//	if (getPosition() == warrior->getPosition()) {
//		visitedWarehouse = false;
//		warrior->reloadAmmo();
//	}
//}

//void SupplySoldier::AssignSupplyTask(Warrior* warrior_to_supply) {
//	if (!IsAlive() || current_supply_state != SUPPLY_IDLE) {
//		return; // Only assign if alive and idle
//	}
//	target_warrior = warrior_to_supply;
//	current_supply_state = SUPPLY_MOVING_TO_WAREHOUSE;
//	path.clear(); // Clear any old path
//	isMoving = false; // Ensure movement starts fresh
//}
//
//void SupplySoldier::Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
//	if (!IsAlive()) {
//		// Ensure reset if died mid-task
//		if (current_supply_state != SUPPLY_IDLE) {
//			current_supply_state = SUPPLY_IDLE;
//			target_warrior = nullptr;
//			isMoving = false;
//			path.clear();
//		}
//		return;
//	}
//	// Check if target warrior still exists and is alive
//	if (target_warrior != nullptr && !target_warrior->IsAlive()) {
//		current_supply_state = SUPPLY_IDLE;
//		target_warrior = nullptr;
//		isMoving = false;
//		path.clear();
//		return;
//	}
//	// If idle, nothing to do
//	if (current_supply_state == SUPPLY_IDLE) {
//		return;
//	}
//
//	Point2D currentPos = getPosition(); // Get current grid position
//	Point2D warehouse = (team == 0) ? AmmoWarehouse0 : AmmoWarehouse1;
//	Point2D warriorPos = target_warrior ? target_warrior->getPosition() : Point2D{ -1,-1 }; // Get target pos safely
//
//	switch (current_supply_state) {
//	case SUPPLY_MOVING_TO_WAREHOUSE:
//		// If not currently moving (no path or path just finished), calculate path to warehouse
//		if (!isMoving) {
//			SetTarget(warehouse.x, warehouse.y, map, securityMap);
//			// SetTarget sets isMoving = true internally if path found
//			if (!isMoving) { // Pathfinding failed?
//				current_supply_state = SUPPLY_IDLE; // Give up
//				target_warrior = nullptr;           // Clear target
//			}
//		}
//		else if (isNear(warehouse)) {
//			printf("moving to warrior\n");
//			current_supply_state = SUPPLY_MOVING_TO_WARRIOR;
//		}
//
//		break;
//
//	case SUPPLY_MOVING_TO_WARRIOR:
//		// Target warrior check is done at the top
//		if (warriorPos.x == -1) { // Safety check if target warrior became null unexpectedly
//			current_supply_state = SUPPLY_IDLE;
//			target_warrior = nullptr;
//			break;
//		}
//		// If not currently moving, calculate path to warrior
//		if (!isMoving) {
//			SetTarget(warriorPos.x, warriorPos.y, map, securityMap);
//			if (!isMoving) { // Pathfinding failed?
//				current_supply_state = SUPPLY_IDLE; // Give up
//				target_warrior = nullptr;
//			}
//		}
//		// Check if arrived at warrior (use Point2D == operator)
//		else if (isNear(warriorPos)) {
//			target_warrior->reloadAmmo();
//			current_supply_state = SUPPLY_IDLE;
//			target_warrior = nullptr;
//		}
//		break;
//
//	default:
//		current_supply_state = SUPPLY_IDLE;
//		target_warrior = nullptr;
//		isMoving = false;
//		path.clear();
//		break;
//	}
//}
