#include "SupportSoldier.h"
#include "Medic.h"
#include "SupplySoldier.h"
#include "Commander.h"

SupportSoldier::SupportSoldier(double start_x, double start_y, int team_id, Commander* cmd)
    : Soldier(start_x, start_y, team_id), commander(cmd),
    current_support_state(SUPPORT_IDLE),
    target_patient(nullptr)
{
}

void SupportSoldier::ResetToIdle() {
    current_support_state = SUPPORT_IDLE;
    target_patient = nullptr;
    isMoving = false;
    path.clear();
}

void SupportSoldier::AssignSupportTask(Soldier* patient) {
    if (!IsAlive() || current_support_state != SUPPORT_IDLE) return;

    target_patient = patient;
    current_support_state = SUPPORT_MOVING_TO_WAREHOUSE;
    path.clear();
    isMoving = false;
}

void SupportSoldier::Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers) {
    if (!IsAlive()) {
        ResetToIdle();
        return;
    }

    // Autonomous mode if commander is dead
    if (!commander || !commander->IsAlive()) {
        HandleScanningSupport(map, securityMap, all_soldiers);
    }

    // If the target died while moving, reset
    if (target_patient && !target_patient->IsAlive()) {
        ResetToIdle();
        return;
    }

    // If idle, handle scanning and patrolling
    if (current_support_state == SUPPORT_IDLE) {
        HandleScanningSupport(map, securityMap, all_soldiers);
        return;
    }

    Point2D currentPos = getPosition();
    Point2D warehouse = GetWarehousePosition();
    Point2D warriorPos = target_patient ? target_patient->getPosition() : Point2D{ -1, -1 };

    switch (current_support_state) {
    case SUPPORT_MOVING_TO_WAREHOUSE:
        if (isNear(warehouse)) {
            current_support_state = SUPPORT_MOVING_TO_WARRIOR;
            path.clear();
            isMoving = false;
        }
        else if (!isMoving && path.empty()) {
            SetTarget(warehouse.x, warehouse.y, map, securityMap);
            if (!isMoving) ResetToIdle(); // Pathfinding failed
        }
        break;

    case SUPPORT_MOVING_TO_WARRIOR:
        if (!target_patient || warriorPos.x == -1) {
            ResetToIdle();
            break;
        }

        if (isNear(warriorPos)) {
            ApplyEffect(target_patient);
            ResetToIdle();
        }
        else if (!isMoving && path.empty()) {
            SetTarget(warriorPos.x, warriorPos.y, map, securityMap);
            if (!isMoving) ResetToIdle(); // Pathfinding failed
        }
        break;

    case SUPPORT_RETURNING_TO_WAREHOUSE:
        if (isNear(warehouse)) {
            ResetToIdle();
            break;
        }
        if (!isMoving && path.empty()) {
            SetTarget(warehouse.x, warehouse.y, map, securityMap);
            if (!isMoving) ResetToIdle(); // Can't reach warehouse
        }
        break;

    default:
        ResetToIdle();
        break;
    }
}

void SupportSoldier::ReturnToWarehouse() {
    if (!IsAlive()) return;

    current_support_state = SUPPORT_RETURNING_TO_WAREHOUSE;
    target_patient = nullptr;
    path.clear();
    isMoving = false;
}

void SupportSoldier::PatrolRandomly(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
    if (isMoving || !path.empty()) return;

    Point2D pos = getPosition();
    Point2D patrol = { -1, -1 };

    for (int attempts = 0; attempts < 30; attempts++) {
        patrol.x = rand() % MSZ;
        patrol.y = rand() % MSZ;

        if (patrol.x == pos.x && patrol.y == pos.y) continue;
        if (map[patrol.y][patrol.x] == STONE || map[patrol.y][patrol.x] == WATER) continue;

        double dist = hypot(patrol.x - pos.x, patrol.y - pos.y);
        if (dist > 8.0) break;
    }

    if (patrol.x != -1)
        SetTarget(patrol.x, patrol.y, map, securityMap);
}

void SupportSoldier::HandleScanningSupport(int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers) {
    if (!IsAlive()) return;
    if (current_support_state != SUPPORT_IDLE) return;

    CalculateVisibility(this, map, visibility);

    bool isMedic = dynamic_cast<Medic*>(this) != nullptr;
    bool isSupply = dynamic_cast<SupplySoldier*>(this) != nullptr;

    bool assigned = false;
    if (isMedic) {
        assigned = ScanAndAssignMedicTasks(all_soldiers);
    }
    else if (isSupply) {
        assigned = ScanAndAssignSupplyTasks(all_soldiers);
    }

    if (assigned) return;

    // No visible target, patrol
    PatrolRandomly(map, securityMap);
}

bool SupportSoldier::ScanAndAssignMedicTasks(const std::vector<Soldier*>& all_soldiers) {
    Soldier* bestLowHP = nullptr;
    Soldier* bestCoverNeed = nullptr;

    for (Soldier* s : all_soldiers) {
        if (!s || !s->IsAlive()) continue;
        if (s == this) continue;
        if (s->getTeam() != this->team) continue;

        Point2D pos = s->getPosition();
        if (!visibility[pos.y][pos.x]) continue;

        int hp = s->getHealth();

        if (hp <= 25) {
            if (!bestLowHP || hp < bestLowHP->getHealth())
                bestLowHP = s;
        }

        Warrior* w = dynamic_cast<Warrior*>(s);
        if (w && w->getCurrentInternalState() == STATE_FINDING_COVER) {
            if (!bestCoverNeed || hp < bestCoverNeed->getHealth())
                bestCoverNeed = w;
        }
    }

    if (bestLowHP) { AssignSupportTask(bestLowHP); return true; }
    if (bestCoverNeed) { AssignSupportTask(bestCoverNeed); return true; }

    return false;
}

bool SupportSoldier::ScanAndAssignSupplyTasks(const std::vector<Soldier*>& all_soldiers) {
    Soldier* bestLowAmmo = nullptr;
    int lowestAmmo = 9999;

    for (Soldier* s : all_soldiers) {
        if (!s || !s->IsAlive()) continue;
        if (s == this) continue;
        if (s->getTeam() != this->team) continue;

        Point2D pos = s->getPosition();
        if (!visibility[pos.y][pos.x]) continue;

        Warrior* w = dynamic_cast<Warrior*>(s);
        if (!w) continue;

        int ammo = w->getAmmo();
        if (ammo < lowestAmmo) {
            lowestAmmo = ammo;
            bestLowAmmo = w;
        }
    }

    if (bestLowAmmo && lowestAmmo < 10) {
        AssignSupportTask(bestLowAmmo);
        return true;
    }

    return false;
}
