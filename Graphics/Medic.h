#pragma once
#include <vector>
#include "SupportSoldier.h"

class Medic : public SupportSoldier {
public:
    Medic(double x, double y, int team_id) : SupportSoldier(x, y, team_id) {}

protected:
	void Show();

    Point2D GetWarehousePosition() const override {
        return (team == 0) ? Point2D{ 1,1 } : Point2D{ MSZ - 8, MSZ - 4 }; // Example medical warehouse
    }

    void ApplyEffect(Warrior* warrior) override {
        warrior->heal();
    }
};