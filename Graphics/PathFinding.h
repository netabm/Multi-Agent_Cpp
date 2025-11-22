#pragma once
#include <vector>
#include "Definitions.h"
#include "visibility.h"

struct PathNode;
struct CompareNodes;

std::vector<Point2D> AStar(Point2D start, Point2D target, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]);
Point2D FindNearestCover(Point2D start, Point2D threat, const std::vector<Point2D>& avoid_spots, int map[MSZ][MSZ]);