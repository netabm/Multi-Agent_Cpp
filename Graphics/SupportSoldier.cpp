#include "SupportSoldier.h"

SupportSoldier::SupportSoldier(double start_x, double start_y, int team_id)
    : Soldier(start_x, start_y, team_id),
    current_support_state(SUPPORT_IDLE),
    target_warrior(nullptr)
{
}

void SupportSoldier::AssignSupportTask(Warrior* warrior_to_support) {
    if (!IsAlive() || current_support_state != SUPPORT_IDLE) return;

    target_warrior = warrior_to_support;
    current_support_state = SUPPORT_MOVING_TO_WAREHOUSE;
    path.clear();
    isMoving = false;
}

void SupportSoldier::Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
    if (!IsAlive()) {
        current_support_state = SUPPORT_IDLE;
        target_warrior = nullptr;
        isMoving = false;
        path.clear();
        return;
    }

    // FIX 1: If target died, completely stop movement
    if (target_warrior && !target_warrior->IsAlive()) {
        current_support_state = SUPPORT_IDLE;
        target_warrior = nullptr;
        isMoving = false;  // Stop moving
        path.clear();      // Clear path
        return;
    }

    if (current_support_state == SUPPORT_IDLE) return;

    Point2D currentPos = getPosition();
    Point2D warehouse = GetWarehousePosition();
    Point2D warriorPos = target_warrior ? target_warrior->getPosition() : Point2D{ -1,-1 };

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
                target_warrior = nullptr;
            }
        }
        // else: currently moving toward warehouse, let Move() handle it
        break;

    case SUPPORT_MOVING_TO_WARRIOR:
        // Validate target still exists
        if (!target_warrior || warriorPos.x == -1) {
            current_support_state = SUPPORT_IDLE;
            target_warrior = nullptr;
            isMoving = false;
            path.clear();
            break;
        }

        // Check if already near warrior
        if (isNear(warriorPos)) {
            // Reached warrior, apply effect
            ApplyEffect(target_warrior);
            current_support_state = SUPPORT_IDLE;
            target_warrior = nullptr;
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
                target_warrior = nullptr;
            }
        }
        // else: currently moving toward warrior, let Move() handle it
        break;

    default:
        // Invalid state, reset
        current_support_state = SUPPORT_IDLE;
        target_warrior = nullptr;
        isMoving = false;
        path.clear();
        break;
    }
}