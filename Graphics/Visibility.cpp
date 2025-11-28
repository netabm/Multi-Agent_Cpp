#include "Visibility.h"

void BresenhamLine(int x1, int y1, int x2, int y2, int map[MSZ][MSZ], bool visible[MSZ][MSZ])
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        visible[y1][x1] = true; // Mark the current cell as visible

        // If the cell is an obstacle that blocks sight, stop tracing this line
        if (map[y1][x1] == STONE || map[y1][x1] == TREE)
            return;

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void CalculateVisibility(Soldier* soldier, int map[MSZ][MSZ], bool visible[MSZ][MSZ])
{
    // Reset map
    for (int i = 0; i < MSZ; i++) for (int j = 0; j < MSZ; j++) visible[i][j] = false;

    Point2D pos = soldier->getPosition();

    // Trace lines from the soldier to every cell on the map's perimeter
    for (int i = 0; i < MSZ; i++) {
        BresenhamLine(pos.x, pos.y, i, 0, map, visible);
        BresenhamLine(pos.x, pos.y, i, MSZ - 1, map, visible);
        BresenhamLine(pos.x, pos.y, 0, i, map, visible);
        BresenhamLine(pos.x, pos.y, MSZ - 1, i, map, visible);
    }
}

void CalculateTeamVisibility(const std::vector<Soldier*>& soldiers, int team_id, int map[MSZ][MSZ], bool teamVisible[MSZ][MSZ])
{
    // Initialize all cells as not visible
    for (int i = 0; i < MSZ; i++)
        for (int j = 0; j < MSZ; j++)
            teamVisible[i][j] = false;

    bool soldierVisible[MSZ][MSZ];

    for (Soldier* s : soldiers) {
        if (!s || !s->IsAlive() || s->getTeam() != team_id)
            continue;

        // Compute individual visibility
        CalculateVisibility(s, map, soldierVisible);

        // Merge into team visibility
        for (int i = 0; i < MSZ; i++)
            for (int j = 0; j < MSZ; j++)
                if (soldierVisible[i][j])
                    teamVisible[i][j] = true;
    }
}

bool HasLineOfSight(int x1, int y1, int x2, int y2, int map[MSZ][MSZ])
{
    if (x1 < 0 || x1 >= MSZ || y1 < 0 || y1 >= MSZ || x2 < 0 || x2 >= MSZ || y2 < 0 || y2 >= MSZ) {
        return false;
    }
    if (map[y1][x1] == STONE || map[y1][x1] == WATER) {
        return false;
    }


    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int currentX = x1;
    int currentY = y1;

    while (true) {
        if (currentX == x2 && currentY == y2) {
            break;
        }

        // Check the next potential step based on Bresenham's error term
        int e2 = 2 * err;
        int nextX = currentX;
        int nextY = currentY;

        if (e2 > -dy) {
            err -= dy;
            nextX += sx;
        }
        if (e2 < dx) {
            err += dx;
            nextY += sy;
        }

        // Check if the calculated next step is valid before checking map
        if (nextX < 0 || nextX >= MSZ || nextY < 0 || nextY >= MSZ) {
            return false;
        }

        // Check if the intermediate cell blocks LoS
        // If the intermediate cell is the target, don't check it as blocking
        if (!(nextX == x2 && nextY == y2) && (map[nextY][nextX] == STONE || map[nextY][nextX] == TREE)) {
            return false; 
        }

        // Move to the next cell
        currentX = nextX;
        currentY = nextY;

        // Failsafe in case of infinite loop
        if (abs(currentX - x1) > MSZ || abs(currentY - y1) > MSZ) {
            return false;
        }
    }
    return true;
}