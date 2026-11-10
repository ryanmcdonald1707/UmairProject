#include "raylib.h"

const int GRID_SIZE = 10;
const int TILE = 56;

struct Unit {
    int x, y;
    bool isPlayer;
};

int main() {
    InitWindow(GRID_SIZE * TILE, GRID_SIZE * TILE, "Arena Tactics");
    SetTargetFPS(60);

    Unit units[4] = {
        {1, 3, true},
        {1, 6, true},
        {8,3, false},
        {8, 6, false},
    };

    while (!WindowShouldClose())
    {
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
        }
        EndDrawing();
    }
    CloseWindow();
}
