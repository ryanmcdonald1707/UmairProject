#include "raylib.h"
#include <unordered_map>
#include <cstdlib>
#include <vector>
#include <algorithm>

const int GRID_SIZE = 10;
const int TILE = 56;
const int MOVE_RANGE = 3;
const int PANEL_WIDTH = 240;

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

struct TurnNode {
    int unitId;
    TurnNode* next;
};

class TurnOrder {
public:
    TurnOrder() = default;
    ~TurnOrder() { clear(); }

    //avoids double deletees
    TurnOrder(const TurnOrder&) = delete;
    TurnOrder& operator=(const TurnOrder&) = delete;

    //builds circle from list of units
    void build(const std::unordered_map<int, Unit>& units)
    {
        clear();

        std::vector<const Unit*> sorted;
        for (auto& [id, u] : units)
        {
            sorted.push_back(&u);
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](const Unit* a, const Unit* b) { return a->initiative > b->initiative; });

        TurnNode* head = nullptr;
        TurnNode* tail = nullptr;
        for (const Unit* u : sorted)
        {
            TurnNode* node = new TurnNode{ u->id, nullptr };
            if (head == nullptr) head = node;
            else                 tail->next = node;
            tail = node;
            count++;
        }

        if (tail != nullptr)
        {
            tail->next = head;   // ends circl
        }
        current = head;
    }

    //-1 IF list empty otherwise ID of turn
    int currentUnit() const
    {
        return current ? current->unitId : -1;
    }

    // next unit, wraps automatically
    void advance()
    {
        if (current) current = current->next;
    }

    // unlinking is O(1) automatic
    bool remove(int unitId)
    {
        if (current == nullptr) return false;

        TurnNode* prev = current;
        TurnNode* node = current->next;

        // Walk exactly once around the circle; prev always trails one behind node
        for (int i = 0; i < count; i++)
        {
            if (node->unitId == unitId)
            {
                if (node == prev)
                {
                    delete node;
                    current = nullptr;
                    count = 0;
                    return true;
                }

                prev->next = node->next;
                if (node == current)
                {
                    current = node->next;
                }
                delete node;
                count--;
                return true;
            }
            prev = node;
            node = node->next;
        }
        return false;
    }

    //order starting from current unit
    std::vector<int> order() const
    {
        std::vector<int> ids;
        TurnNode* n = current;
        for (int i = 0; i < count; i++)
        {
            ids.push_back(n->unitId);
            n = n->next;
        }
        return ids;
    }

    int size() const { return count; }

private:
    void clear()
    {
        for (int i = 0; i < count; i++)
        {
            TurnNode* next = current->next;
            delete current;
            current = next;
        }
        current = nullptr;
        count = 0;
    }

    TurnNode* current = nullptr;
    int count = 0;
};

int main() {
    InitWindow(GRID_SIZE * TILE + PANEL_WIDTH, GRID_SIZE * TILE, "Arena Tactics");
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

    TurnOrder turns;
    turns.build(units);

    int selected = -1; //Nothing.

    while (!WindowShouldClose())
    {
        int active = turns.currentUnit();
        bool playerTurn = (active != -1) && units.at(active).isPlayer;

        if (IsKeyPressed(KEY_SPACE))
        {
            //Skip turn/End Turns
            turns.advance();
            selected = -1;
        }
        else if (playerTurn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouse = GetMousePosition();
            int tx = (int)(mouse.x / TILE);
            int ty = (int)(mouse.y / TILE);

            if (tx < GRID_SIZE && ty < GRID_SIZE)
            {
                auto it = tileToUnit.find(tileKey(tx, ty));
                if (it != tileToUnit.end())
                {
                    //select only the units whos turn it is.
                    selected = (it->second == active) ? active : -1;
                }
                else if (selected == active && distance(units.at(selected).x, units.at(selected).y, tx, ty) <= MOVE_RANGE)
                {
                    Unit& u = units.at(selected);
                    tileToUnit.erase(tileKey(u.x, u.y));
                    u.x = tx;
                    u.y = ty;
                    tileToUnit[tileKey(tx, ty)] = selected;
                    selected = -1;
                    turns.advance(); //moving ends turn
                }
                else
                {
                    selected = -1;
                }
            }
        }



        BeginDrawing();
        ClearBackground(BLACK);
        int activeId = turns.currentUnit();

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
            if (id == activeId)
            {
                //green = this guys turn
                DrawRectangleLinesEx(Rectangle{ (float)(u.x * TILE), (float)(u.y * TILE), (float)(TILE - 2), (float)(TILE - 2) }, 3, GREEN);
            }
            if (id == selected)
            {
                DrawRectangleLines(u.x * TILE, u.y * TILE, TILE - 2, TILE - 2, YELLOW);
            }
        }

        //Draw Turn Order Panel
        int px = GRID_SIZE * TILE + 12;
        DrawText("TURN ORDER", px, 12, 20, WHITE);

        int lineY = 44;
        for (int id : turns.order())
        {
            const Unit& u = units.at(id);
            const char* label = TextFormat("%s %s %d (init %d)",
                                        id == activeId ? ">" : " ",
                                        u.isPlayer ? "Player" : "Enemy",
                                        id, u.initiative);
            DrawText(label, px, lineY, 18, u.isPlayer ? SKYBLUE : RED);
            lineY += 24;
        }

        lineY += 16;
        if (activeId != -1 && units.at(activeId).isPlayer)
        {
            DrawText("Your turn:", px, lineY, 18, LIGHTGRAY);
            DrawText("click unit, then a tile", px, lineY + 22, 18, LIGHTGRAY);
        }
        else
        {
            DrawText("Enemy turn", px, lineY, 18, LIGHTGRAY);
        }
        DrawText("SPACE: end turn", px, lineY + 50, 18, GRAY);
        EndDrawing();
    }
    CloseWindow();
}
