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

Point2D FindNearestCover(Point2D start, Point2D threat, const std::vector<Point2D>& avoid_spots, int map[MSZ][MSZ])
{
    const int MAX_SEARCH_RANGE = 20; // Define search range (20 cells)
    const double MIN_FRIENDLY_DISTANCE_SQ = 3.0 * 3.0; // Use squared distance for efficiency

    std::queue<std::pair<Point2D, int>> q; // Queue for {Point, distance}
    bool visited[MSZ][MSZ] = { false };

    q.push({ start, 0 });
    visited[start.y][start.x] = true;

    int dx[] = { 0, 0, 1, -1, 1, 1, -1, -1 };
    int dy[] = { 1, -1, 0, 0, 1, -1, 1, -1 };

    while (!q.empty())
    {
        Point2D currentPos = q.front().first;
        int currentDist = q.front().second;
        q.pop();

        // We can't take cover in our own spot. We must move.
        if (currentDist > 0)
        {
            // 1. Check if spot has cover from threat
            if (!HasLineOfSight(currentPos.x, currentPos.y, threat.x, threat.y, map))
            {
                // 2. This spot has cover. NOW, check if it's too close to an "avoid" spot.
                bool tooCloseToAvoid = false;
                for (const Point2D& spot : avoid_spots) {
                    // Don't check distance to ourself (the start pos)
                    if (spot.x == start.x && spot.y == start.y) continue;

                    double distSq = pow(currentPos.x - spot.x, 2) + pow(currentPos.y - spot.y, 2);
                    if (distSq < MIN_FRIENDLY_DISTANCE_SQ) {
                        tooCloseToAvoid = true;
                        break;
                    }
                }

                // If it's NOT too close, it's our spot!
                if (!tooCloseToAvoid) {
                    return currentPos; // This is the NEAREST, SOCIALLY-DISTANCED cover.
                }
                // If it was too close, we don't return. The BFS continues.
            }
        }

        // 3. If not safe, or too close, or start tile:
        // Check if we are still within search range
        if (currentDist < MAX_SEARCH_RANGE)
        {
            // Add neighbors to the queue
            for (int i = 0; i < 8; i++) // Check all 8 directions
            {
                int nx = currentPos.x + dx[i];
                int ny = currentPos.y + dy[i];

                if (nx >= 0 && nx < MSZ && ny >= 0 && ny < MSZ &&
                    !visited[ny][nx] &&
                    (map[ny][nx] != STONE && map[ny][nx] != WATER))
                {
                    visited[ny][nx] = true;
                    q.push({ {nx, ny}, currentDist + 1 });
                }
            }
        }
    }

    // No cover found within range
    return { -1, -1 };
}
