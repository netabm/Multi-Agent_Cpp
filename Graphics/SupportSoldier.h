#pragma once
#include "Warrior.h"

enum SupportState {
    SUPPORT_IDLE,
    SUPPORT_MOVING_TO_WAREHOUSE,
    SUPPORT_MOVING_TO_WARRIOR,
    SUPPORT_RETURNING_HOME
};

class SupportSoldier : public Soldier {
protected:
    SupportState current_support_state = SUPPORT_IDLE;
    Soldier* target_patient = nullptr; // Soldier to supply/heal

    virtual Point2D GetWarehousePosition() const = 0; // Pure virtual: supply/medic warehouse
    virtual void ApplyEffect(Soldier* patient) = 0;   // Pure virtual: ammo/heal/etc

public:
    SupportSoldier(double start_x, double start_y, int team_id);

    void AssignSupportTask(Soldier* patient);

    void Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);

    SupportState getSupportState() const { return current_support_state;}

    void ReturnToWarehouse();

};
