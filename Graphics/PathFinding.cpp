#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include "PathFinding.h"

struct PathNode {
    int x, y;
    double g, h, f;
    PathNode* parent;
    PathNode(int _x, int _y, PathNode* _p = nullptr) : x(_x), y(_y), parent(_p), g(0), h(0), f(0) {}
};

struct CompareNodes {
    bool operator()(const PathNode* a, const PathNode* b) const { return a->f > b->f; }
};

std::vector<Point2D> AStar(Point2D start, Point2D target, int map[MSZ][MSZ], double securityMap[MSZ][MSZ]) {
    std::priority_queue<PathNode*, std::vector<PathNode*>, CompareNodes> open_list;
    std::vector<PathNode*> all_nodes;
    bool closed_list[MSZ][MSZ] = { false };
    std::vector<Point2D> final_path;

    PathNode* start_node = new PathNode(start.x, start.y);
    double dx_start = abs(start.x - target.x);
    double dy_start = abs(start.y - target.y);
    start_node->h = (dx_start + dy_start) + (sqrt(2) - 2) * std::min(dx_start, dy_start);
    start_node->f = start_node->g + start_node->h;
    open_list.push(start_node);
    all_nodes.push_back(start_node);

    while (!open_list.empty()) {
        PathNode* current = open_list.top();
        open_list.pop();
        if (current->x == target.x && current->y == target.y) {
            PathNode* temp = current;
            while (temp != nullptr) {
                final_path.push_back({ temp->x, temp->y });
                temp = temp->parent;
            }
            for (auto& node : all_nodes) delete node;
            return final_path;
        }
        if (closed_list[current->y][current->x]) continue;
        closed_list[current->y][current->x] = true;

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int neighbor_x = current->x + dx;
                int neighbor_y = current->y + dy;
                if (neighbor_x < 0 || neighbor_x >= MSZ || neighbor_y < 0 || neighbor_y >= MSZ ||
                    map[neighbor_y][neighbor_x] == STONE || map[neighbor_y][neighbor_x] == WATER ||
                    closed_list[neighbor_y][neighbor_x]) continue;

                PathNode* neighbor = new PathNode(neighbor_x, neighbor_y, current);
                double move_cost = (dx != 0 && dy != 0) ? 1.414 : 1.0;
                neighbor->g = current->g + move_cost + securityMap[neighbor_y][neighbor_x];
                double dx_h = abs(neighbor->x - target.x);
                double dy_h = abs(neighbor->y - target.y);
                neighbor->h = (dx_h + dy_h) + (sqrt(2) - 2) * std::min(dx_h, dy_h);
                neighbor->f = neighbor->g + neighbor->h;
                open_list.push(neighbor);
                all_nodes.push_back(neighbor);
            }
        }
    }

    for (auto& node : all_nodes) delete node;
    return final_path;
}
