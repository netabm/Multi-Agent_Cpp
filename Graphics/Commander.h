#pragma once
#include <vector>
#include "SupplySoldier.h"
#include "Medic.h"

class Commander : public Soldier {
private:
	bool team_visibility[MSZ][MSZ];
	Point2D targetLocation;
	CommandType currentCommand;
public:
	std::vector<Point2D> defend_reserved_spots{};
	Commander(double start_x, double start_y, int team_id);

	void Update(const std::vector<Soldier*>& team_soldiers, const std::vector<Soldier*>& all_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void Show();

	void CheckAmmo(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	void CheckHealth(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
	CommandType getCurrentCommand() const { return currentCommand; }
	Point2D getTargetLocation() const { return targetLocation; }
	std::vector<Point2D>& GetReservedDefendSpots() { return defend_reserved_spots; }


	//void UpdateVisibility(const std::vector<Soldier*>& team_soldiers, int map[MSZ][MSZ]);
	bool (*getTeamVisibility())[MSZ] { return team_visibility; }
};