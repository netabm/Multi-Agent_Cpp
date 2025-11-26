#include "SupportSoldier.h"

SupportSoldier::SupportSoldier(double start_x, double start_y, int team_id)
    : Soldier(start_x, start_y, team_id),
    current_support_state(SUPPORT_IDLE),
    target_patient(nullptr)
{
}

void SupportSoldier::AssignSupportTask(Soldier* patient) {
    if (!IsAlive() || current_support_state != SUPPORT_IDLE) return;

    target_patient = patient;
    current_support_state = SUPPORT_MOVING_TO_WAREHOUSE;
    path.clear();
    isMoving = false;
}

void SupportSoldier::Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
    if (!IsAlive()) {
        current_support_state = SUPPORT_IDLE;
        target_patient = nullptr;
        isMoving = false;
        path.clear();
        return;
    }

    if (target_patient && !target_patient->IsAlive()) {
        current_support_state = SUPPORT_IDLE;
        target_patient = nullptr;
        isMoving = false;  // Stop moving
        path.clear();      // Clear path
        return;
    }

    if (current_support_state == SUPPORT_IDLE) return;

    Point2D currentPos = getPosition();
    Point2D warehouse = GetWarehousePosition();
    Point2D warriorPos = target_patient ? target_patient->getPosition() : Point2D{ -1,-1 };

    switch (current_support_state) {
    case SUPPORT_MOVING_TO_WAREHOUSE:
        // Check if already at warehouse FIRST
        if (isNear(warehouse)) {
            // Already at warehouse, proceed to next state
            current_support_state = SUPPORT_MOVING_TO_WARRIOR;
            path.clear();
            isMoving = false;
        }
        else if (!isMoving && path.empty()) {
            // Not at warehouse and not moving, start moving
            SetTarget(warehouse.x, warehouse.y, map, securityMap);

            // If still not moving after SetTarget, pathfinding failed
            if (!isMoving) {
                // Can't reach warehouse, abort mission
                current_support_state = SUPPORT_IDLE;
                target_patient = nullptr;
            }
        }
        // else: currently moving toward warehouse, let Move() handle it
        break;

    case SUPPORT_MOVING_TO_WARRIOR:
        // Validate target still exists
        if (!target_patient || warriorPos.x == -1) {
            current_support_state = SUPPORT_IDLE;
            target_patient = nullptr;
            isMoving = false;
            path.clear();
            break;
        }

        // Check if already near warrior
        if (isNear(warriorPos)) {
            // Reached warrior, apply effect
            ApplyEffect(target_patient);
            current_support_state = SUPPORT_IDLE;
            target_patient = nullptr;
            isMoving = false;
            path.clear();
        }
        else if (!isMoving && path.empty()) {
            // Not near warrior and not moving, start moving
            SetTarget(warriorPos.x, warriorPos.y, map, securityMap);

            // If still not moving after SetTarget, pathfinding failed
            if (!isMoving) {
                // Can't reach warrior, abort mission
                current_support_state = SUPPORT_IDLE;
                target_patient = nullptr;
            }
        }
        // else: currently moving toward warrior, let Move() handle it
        break;

    case SUPPORT_RETURNING_HOME:
    {
        Point2D home = GetWarehousePosition();

        // If already there, stay idle
        if (isNear(home)) {
            current_support_state = SUPPORT_IDLE;
            path.clear();
            isMoving = false;
            break;
        }

        // Start moving if no path
        if (!isMoving && path.empty()) {
            SetTarget(home.x, home.y, map, securityMap);

            if (!isMoving) {
                // Can't reach home, just idle
                current_support_state = SUPPORT_IDLE;
            }
        }
    }
    break;

    default:
        // Invalid state, reset
        current_support_state = SUPPORT_IDLE;
        target_patient = nullptr;
        isMoving = false;
        path.clear();
        break;
    }
}

void SupportSoldier::ReturnToWarehouse()
{
    if (!IsAlive()) return;

    current_support_state = SUPPORT_RETURNING_HOME;
    target_patient = nullptr;   // no warrior involved
    path.clear();
    isMoving = false;
}
