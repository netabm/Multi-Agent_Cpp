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

    if (target_warrior && !target_warrior->IsAlive()) {
        current_support_state = SUPPORT_IDLE;
        target_warrior = nullptr;
        isMoving = false;
        path.clear();
        return;
    }

    if (current_support_state == SUPPORT_IDLE) return;

    Point2D currentPos = getPosition();
    Point2D warehouse = GetWarehousePosition();
    Point2D warriorPos = target_warrior ? target_warrior->getPosition() : Point2D{ -1,-1 };

    switch (current_support_state) {
    case SUPPORT_MOVING_TO_WAREHOUSE:
        if (!isMoving) {
            SetTarget(warehouse.x, warehouse.y, map, securityMap);
            if (!isMoving) {
                current_support_state = SUPPORT_IDLE;
                target_warrior = nullptr;
            }
        }
        else if (isNear(warehouse)) {
            current_support_state = SUPPORT_MOVING_TO_WARRIOR;
        }
        break;

    case SUPPORT_MOVING_TO_WARRIOR:
        if (!target_warrior || warriorPos.x == -1) {
            current_support_state = SUPPORT_IDLE;
            target_warrior = nullptr;
            break;
        }
        if (!isMoving) {
            SetTarget(warriorPos.x, warriorPos.y, map, securityMap);
            if (!isMoving) {
                current_support_state = SUPPORT_IDLE;
                target_warrior = nullptr;
            }
        }
        else if (isNear(warriorPos)) {
            ApplyEffect(target_warrior); // Give ammo or heal
            current_support_state = SUPPORT_IDLE;
            target_warrior = nullptr;
        }
        break;

    default:
        current_support_state = SUPPORT_IDLE;
        target_warrior = nullptr;
        isMoving = false;
        path.clear();
        break;
    }
}
