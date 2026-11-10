#include "raylib.h"
#include <unordered_map>
#include <cstdlib>

const int GRID_SIZE = 10;
const int TILE = 56;
const int MOVE_RANGE = 3;

struct Unit {
    int x, y;
    bool isPlayer;
};

int tileKey(int x, int y)
{
    return y * GRID_SIZE + x;
}

//Manhatten distance getter for diagonals
int distance(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

int main() {
    InitWindow(GRID_SIZE * TILE, GRID_SIZE * TILE, "Arena Tactics");
    SetTargetFPS(60);

    Unit units[4] = {
        {1, 3, true},
        {1, 6, true},
        {8,3, false},
        {8, 6, false},
    };

    std::unordered_map<int, int> tileToUnit;

    for (int i = 0; i < 4; i++)
    {
        tileToUnit[tileKey(units[i].x, units[i].y)] = i;
    }

    int selected = -1; //Nothing.

    while (!WindowShouldClose())
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouse = GetMousePosition();
            int tx = (int)(mouse.x / TILE);
            int ty = (int)(mouse.y / TILE);

            auto it = tileToUnit.find(tileKey(tx, ty));
            if (it != tileToUnit.end())
            {
                selected = it->second;
            }
            else
            {
                selected = -1;
            }
        }


        BeginDrawing();
        ClearBackground(BLACK);

        //Draw Grid
        for (int y = 0; y < GRID_SIZE; y++)
        {
            for (int x = 0; x < GRID_SIZE; x++)
            {
                DrawRectangle(x * TILE, y * TILE, TILE - 2, TILE - 2, DARKGRAY);
            }
        }

        //Draw Units
        for (int i = 0; i < 4; i++)
        {
            Color c = units[i].isPlayer ? BLUE : RED;
            DrawRectangle(units[i].x * TILE + 8, units[i].y * TILE + 8, TILE - 18, TILE - 18, c);
            if (i == selected)
            {
                DrawRectangleLines(units[i].x * TILE, units[i].y * TILE, TILE - 2, TILE - 2, YELLOW);
            }
        }
        EndDrawing();
    }
    CloseWindow();
}
