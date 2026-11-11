#include "raylib.h"
#include <unordered_map>
#include <cstdlib>

const int GRID_SIZE = 10;
const int TILE = 56;
const int MOVE_RANGE = 3;

struct Unit {
    int id;
    int x, y;
    bool isPlayer;
    int hp;
    int maxHp;
    int attack;
    int initiative;
};


int tileKey(int x, int y)
{
    return y * GRID_SIZE + x;
}

//Manhatten distance getter for distance with no diag
int distance(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

int main() {
    InitWindow(GRID_SIZE * TILE, GRID_SIZE * TILE, "Arena Tactics");
    SetTargetFPS(60);

    std::unordered_map<int, Unit> units;  
    std::unordered_map<int, int> tileToUnit; 

    //          id  x  y  player  hp maxHp atk init
    units[0] = { 0, 1, 3, true,   10, 10,   4,  7 };
    units[1] = { 1, 1, 6, true,   10, 10,   4,  4 };
    units[2] = { 2, 8, 3, false,   8,  8,   3,  6 };
    units[3] = { 3, 8, 6, false,   8,  8,   3,  3 };

    for (auto& [id, u] : units)
    {
        tileToUnit[tileKey(u.x, u.y)] = id;
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
            else if (selected != -1 && distance(units.at(selected).x, units.at(selected).y, tx, ty) <= MOVE_RANGE)
            {
                Unit& u = units.at(selected);
                tileToUnit.erase(tileKey(u.x, u.y));
                u.x = tx;
                u.y = ty;
                tileToUnit[tileKey(tx, ty)] = selected;
                selected = -1;
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
                Color c = DARKGRAY;
                if (selected != -1 && distance(units.at(selected).x, units.at(selected).y, x, y) <= MOVE_RANGE
                    && tileToUnit.find(tileKey(x, y)) == tileToUnit.end())
                {
                    c = Color{ 60, 90, 140, 255 };
                }
                DrawRectangle(x * TILE, y * TILE, TILE - 2, TILE - 2, c);
            }
        }

        //Draw Units
        for (auto& [id, u] : units)
        {
            Color c = u.isPlayer ? BLUE : RED;
            DrawRectangle(u.x * TILE + 8, u.y * TILE + 8, TILE - 18, TILE - 18, c);
            const char* hpText = TextFormat("%d", u.hp);
            int fontSize = 20;
            int squareSize = TILE - 18;
            int centerX = u.x * TILE + 8 + squareSize / 2;
            int centerY = u.y * TILE + 8 + squareSize / 2;
            DrawText(hpText, centerX - MeasureText(hpText, fontSize) / 2, centerY - fontSize / 2, fontSize, WHITE);
            if (id == selected)
            {
                DrawRectangleLines(u.x * TILE, u.y * TILE, TILE - 2, TILE - 2, YELLOW);
            }
        }
        EndDrawing();
    }
    CloseWindow();
}
