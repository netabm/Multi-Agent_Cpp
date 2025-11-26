#pragma once
#include <vector>
#include "SupportSoldier.h"

class SupplySoldier : public SupportSoldier {
public:
    SupplySoldier(double x, double y, int team_id) : SupportSoldier(x, y, team_id) {}

protected:
    void Show();

    Point2D GetWarehousePosition() const override {
        return (team == 0) ? Point2D{ 2,2 } : Point2D{ MSZ - 3, MSZ - 9 };
    }

    void ApplyEffect(Soldier* soldier) override {
        Warrior* w = dynamic_cast<Warrior*>(soldier);
        if (w) {
            w->reloadAmmo();
        }
    }
};