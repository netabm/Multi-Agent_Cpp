#pragma once
#include <vector>
#include "SupportSoldier.h"

class Medic : public SupportSoldier {
public:
    Medic(double x, double y, int team_id, Commander* cmd) : SupportSoldier(x, y, team_id, cmd) {}

protected:
	void Show();

    Point2D GetWarehousePosition() const override {
        return (team == 0) ? Point2D{ 2,8 } : Point2D{ MSZ - 3, MSZ - 3 };
    }

    void ApplyEffect(Soldier* patient) override {
        patient->heal();
    }
};