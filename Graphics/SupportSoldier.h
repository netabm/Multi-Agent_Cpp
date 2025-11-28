#pragma once
#include "Warrior.h"

enum SupportState {
    SUPPORT_IDLE,
    SUPPORT_MOVING_TO_WAREHOUSE,
    SUPPORT_MOVING_TO_WARRIOR,
    SUPPORT_RETURNING_HOME,
	SUPPORT_PATROLLING
};

class SupportSoldier : public Soldier {
protected:
    SupportState current_support_state = SUPPORT_IDLE;
    Soldier* target_patient = nullptr; // Soldier to supply/heal
	Commander* commander = nullptr; // Assigned commander

    virtual Point2D GetWarehousePosition() const = 0; // virtual supply/medic warehouse
    virtual void ApplyEffect(Soldier* patient) = 0;   // virtual ammo/heal/etc

public:
    SupportSoldier(double start_x, double start_y, int team_id, Commander* cmd);

    void AssignSupportTask(Soldier* patient);

    void Update(int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);

    SupportState getSupportState() const { return current_support_state;}

    void ReturnToWarehouse();

    void PatrolRandomly(int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);

    void HandleScanningSupport(int map[MSZ][MSZ], double securityMap[MSZ][MSZ], const std::vector<Soldier*>& all_soldiers);
    
    bool ScanAndAssignMedicTasks(const std::vector<Soldier*>& all_soldiers);

    bool ScanAndAssignSupplyTasks(const std::vector<Soldier*>& all_soldiers);

    void ResetToIdle();

};
