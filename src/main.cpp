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

Font gameFont; 

void drawText(const char* text, int x, int y, int fontSize, Color color)
{
    DrawTextEx(gameFont, text, Vector2{ (float)x, (float)y }, (float)fontSize, 1.0f, color);
}

int measureText(const char* text, int fontSize)
{
    return (int)MeasureTextEx(gameFont, text, (float)fontSize, 1.0f).x;
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


enum class Screen { Menu, Playing };

//draws button, true if clicked this frame
bool menuButton(Rectangle r, const char* text)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, hover ? Color{ 80, 80, 80, 255 } : Color{ 50, 50, 50, 255 });
    DrawRectangleLinesEx(r, 2, hover ? YELLOW : LIGHTGRAY);

    int fontSize = 24;
    drawText(text, (int)(r.x + (r.width - measureText(text, fontSize)) / 2),
             (int)(r.y + (r.height - fontSize) / 2), fontSize, WHITE);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

//puts everything back to start
void resetGame(std::unordered_map<int, Unit>& units, std::unordered_map<int, int>& tileToUnit,
               TurnOrder& turns, int& selected, bool& moved)
{
    units.clear();
    tileToUnit.clear();

    //          id  x  y  player  hp maxHp atk init
    units[0] = { 0, 1, 3, true,   10, 10,   4,  7 };
    units[1] = { 1, 1, 6, true,   10, 10,   4,  4 };
    units[2] = { 2, 8, 3, false,   8,  8,   3,  6 };
    units[3] = { 3, 8, 6, false,   8,  8,   3,  3 };

    for (auto& [id, u] : units)
    {
        tileToUnit[tileKey(u.x, u.y)] = id;
    }
    turns.build(units);
    selected = -1;
    moved = false;
}


int main() {
    InitWindow(GRID_SIZE * TILE + PANEL_WIDTH, GRID_SIZE * TILE, "Arena Tactics");
    SetTargetFPS(60);

    //load big so it stays sharp when drawn smaller
    gameFont = LoadFontEx(TextFormat("%sassets/fonts/Roboto-Regular.ttf", GetApplicationDirectory()), 64, nullptr, 0);
    SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);

    std::unordered_map<int, Unit> units;
    std::unordered_map<int, int> tileToUnit;
    TurnOrder turns;

    int selected = -1; //Nothing.
    bool moved = false; //has moved?

    Screen screen = Screen::Menu;
    bool quit = false;

    while (!WindowShouldClose() && !quit)
    {
        //Draw Menu
        if (screen == Screen::Menu)
        {
            BeginDrawing();
            ClearBackground(BLACK);

            int w = GetScreenWidth();
            const char* title = "ARENA TACTICS";
            int titleSize = 60;
            drawText(title, (w - measureText(title, titleSize)) / 2, 120, titleSize, GOLD);

            Rectangle playBtn = Rectangle{ (float)(w / 2 - 100), 260, 200, 50 };
            Rectangle quitBtn = Rectangle{ (float)(w / 2 - 100), 330, 200, 50 };

            if (menuButton(playBtn, "PLAY") || IsKeyPressed(KEY_ENTER))
            {
                //fresh game every time
                resetGame(units, tileToUnit, turns, selected, moved);
                screen = Screen::Playing;
            }
            if (menuButton(quitBtn, "QUIT"))
            {
                quit = true;
            }

            const char* hint = "click PLAY or press ENTER";
            drawText(hint, (w - measureText(hint, 18)) / 2, 410, 18, GRAY);

            EndDrawing();
            continue; //skip game code while on menu
        }

        int active = turns.currentUnit();
        bool playerTurn = (active != -1) && units.at(active).isPlayer;
        bool gameOver = countSide(units, true) == 0 || countSide(units, false) == 0;

        if (gameOver && IsKeyPressed(KEY_ENTER))
        {
            //back to menu
            screen = Screen::Menu;
        }
        else if (!gameOver && IsKeyPressed(KEY_SPACE))
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
            drawText(hpText, centerX - measureText(hpText, fontSize) / 2, centerY - fontSize / 2, fontSize, WHITE);
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
        drawText("TURN ORDER", px, 12, 20, WHITE);

        int lineY = 44;
        for (int id : turns.order())
        {
            const Unit& u = units.at(id);
            const char* label = TextFormat("%s %s %d (init %d)",
                                        id == activeId ? ">" : " ",
                                        u.isPlayer ? "Player" : "Enemy",
                                        id, u.initiative);
            drawText(label, px, lineY, 18, u.isPlayer ? SKYBLUE : RED);
            lineY += 24;
        }

        lineY += 16;
        if (activeId != -1 && units.at(activeId).isPlayer)
        {
            drawText("Your turn:", px, lineY, 18, LIGHTGRAY);
            drawText(moved ? "click enemy to attack" : "click unit, then a tile", px, lineY + 22, 18, LIGHTGRAY);
        }
        else
        {
            drawText("Enemy turn", px, lineY, 18, LIGHTGRAY);
        }
        drawText("SPACE: end turn", px, lineY + 50, 18, GRAY);

        //game over screen
        int playersLeft = countSide(units, true);
        int enemiesLeft = countSide(units, false);
        if (playersLeft == 0 || enemiesLeft == 0)
        {
            const char* msg = (enemiesLeft == 0) ? "VICTORY" : "DEFEAT";
            int msgSize = 60;
            DrawRectangle(0, 0, GRID_SIZE * TILE, GRID_SIZE * TILE, Color{ 0, 0, 0, 170 });
            drawText(msg, (GRID_SIZE * TILE - measureText(msg, msgSize)) / 2,
                     GRID_SIZE * TILE / 2 - msgSize / 2, msgSize,
                     enemiesLeft == 0 ? GOLD : RED);
            const char* backText = "ENTER: main menu";
            drawText(backText, (GRID_SIZE * TILE - measureText(backText, 20)) / 2,
                     GRID_SIZE * TILE / 2 + 50, 20, LIGHTGRAY);
        }
        EndDrawing();
    }
    UnloadFont(gameFont);
    CloseWindow();
}
