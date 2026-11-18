#include "raylib.h"
#include <unordered_map>
#include <cstdlib>
#include <vector>
#include <algorithm>

const int GRID_SIZE = 10;
const int TILE = 56;
const int MOVE_RANGE = 3;
const int ATTACK_RANGE = 1;
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


//removes unit from everything tracked
void killUnit(int id, std::unordered_map<int, Unit>& units,
              std::unordered_map<int, int>& tileToUnit, TurnOrder& turns)
{
    tileToUnit.erase(tileKey(units.at(id).x, units.at(id).y));
    turns.remove(id);
    units.erase(id);
}

//applies damage to target
bool attackUnit(int attackerId, int targetId, std::unordered_map<int, Unit>& units,
                std::unordered_map<int, int>& tileToUnit, TurnOrder& turns)
{
    Unit& target = units.at(targetId);
    target.hp -= units.at(attackerId).attack;
    if (target.hp <= 0)
    {
        killUnit(targetId, units, tileToUnit, turns);
        return true;
    }
    return false;
}

bool hasEnemyInRange(int id, const std::unordered_map<int, Unit>& units)
{
    const Unit& me = units.at(id);
    for (auto& [otherId, other] : units)
    {
        if (other.isPlayer != me.isPlayer && distance(me.x, me.y, other.x, other.y) <= ATTACK_RANGE)
        {
            return true;
        }
    }
    return false;
}

//how many are left
int countSide(const std::unordered_map<int, Unit>& units, bool isPlayer)
{
    int n = 0;
    for (auto& [id, u] : units)
    {
        if (u.isPlayer == isPlayer) n++;
    }
    return n;
}


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
    bool moved = false; //has moved?

    while (!WindowShouldClose())
    {
        int active = turns.currentUnit();
        bool playerTurn = (active != -1) && units.at(active).isPlayer;
        bool gameOver = countSide(units, true) == 0 || countSide(units, false) == 0;

        if (!gameOver && IsKeyPressed(KEY_SPACE))
        {
            //Skip turn/End Turns
            turns.advance();
            selected = -1;
            moved = false;
        }
        else if (!gameOver && playerTurn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouse = GetMousePosition();
            int tx = (int)(mouse.x / TILE);
            int ty = (int)(mouse.y / TILE);

            if (tx < GRID_SIZE && ty < GRID_SIZE)
            {
                auto it = tileToUnit.find(tileKey(tx, ty));
                if (it != tileToUnit.end())
                {
                    int clicked = it->second;
                    const Unit& me = units.at(active);
                    const Unit& other = units.at(clicked);

                    if (clicked == active)
                    {
                        //select only the unit whose turn it is
                        selected = active;
                    }
                    else if (selected == active && !other.isPlayer
                             && distance(me.x, me.y, other.x, other.y) <= ATTACK_RANGE)
                    {
                        attackUnit(active, clicked, units, tileToUnit, turns);
                        //attacking ends the turn
                        turns.advance();
                        selected = -1;
                        moved = false;
                    }
                }
                else if (selected == active && !moved
                         && distance(units.at(selected).x, units.at(selected).y, tx, ty) <= MOVE_RANGE)
                {
                    Unit& u = units.at(selected);
                    tileToUnit.erase(tileKey(u.x, u.y));
                    u.x = tx;
                    u.y = ty;
                    tileToUnit[tileKey(tx, ty)] = selected;
                    moved = true;

                    if (!hasEnemyInRange(active, units))
                    {
                        //nothing to attack moving ends the turn
                        turns.advance();
                        selected = -1;
                        moved = false;
                    }
                }
                else if (!moved)
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
                if (selected != -1 && !moved && distance(units.at(selected).x, units.at(selected).y, x, y) <= MOVE_RANGE
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
            //orange outline = enemy you can attack right now
            if (selected != -1 && !u.isPlayer
                && distance(units.at(selected).x, units.at(selected).y, u.x, u.y) <= ATTACK_RANGE)
            {
                DrawRectangleLinesEx(Rectangle{ (float)(u.x * TILE), (float)(u.y * TILE), (float)(TILE - 2), (float)(TILE - 2) }, 3, ORANGE);
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
            DrawText(moved ? "click enemy to attack" : "click unit, then a tile", px, lineY + 22, 18, LIGHTGRAY);
        }
        else
        {
            DrawText("Enemy turn", px, lineY, 18, LIGHTGRAY);
        }
        DrawText("SPACE: end turn", px, lineY + 50, 18, GRAY);

        //game over screen
        int playersLeft = countSide(units, true);
        int enemiesLeft = countSide(units, false);
        if (playersLeft == 0 || enemiesLeft == 0)
        {
            const char* msg = (enemiesLeft == 0) ? "VICTORY" : "DEFEAT";
            int msgSize = 60;
            DrawRectangle(0, 0, GRID_SIZE * TILE, GRID_SIZE * TILE, Color{ 0, 0, 0, 170 });
            DrawText(msg, (GRID_SIZE * TILE - MeasureText(msg, msgSize)) / 2,
                     GRID_SIZE * TILE / 2 - msgSize / 2, msgSize,
                     enemiesLeft == 0 ? GOLD : RED);
        }
        EndDrawing();
    }
    CloseWindow();
}
