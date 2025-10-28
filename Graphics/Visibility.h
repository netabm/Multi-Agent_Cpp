#pragma once
#include "Soldier.h"

class Soldier;

void BresenhamLine(int x1, int y1, int x2, int y2, int map[MSZ][MSZ], bool visible[MSZ][MSZ]);
void CalculateVisibility(Soldier* soldier, int map[MSZ][MSZ], bool visible[MSZ][MSZ]);
void CalculateTeamVisibility(const std::vector<Soldier*>& soldiers, int team_id, int map[MSZ][MSZ], bool teamVisible[MSZ][MSZ]);
bool HasLineOfSight(int x1, int y1, int x2, int y2, int map[MSZ][MSZ]);