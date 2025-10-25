#pragma once
#include <vector>
#include "Definitions.h"

struct PathNode;
struct CompareNodes;

// The A* function DECLARATION (the prototype)
std::vector<Point2D> AStar(Point2D start, Point2D target, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);