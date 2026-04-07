// SnakeEffect.cpp
// KI-Strategie (4 Stufen):
//   1. BFS-Pfad zum Futter + Sicherheitscheck: nach virtuellem Schritt noch
//      Weg zum eigenen Schwanz erreichbar? -> nur dann Futter ansteuern
//   2. Tail-Chase: BFS Weg zum Schwanzende (Schwanz selbst nicht blockiert,
//      da er sich beim naechsten Schritt frei gibt) -> haelt Schlange am Leben
//   3. Freier-Raum-Maximum: Zug mit den meisten erreichbaren Feldern
//   4. Notfallzug: irgendeinen gueltigen Zug (verhindert sofortigen Reset)

#include "effect_helpers.h"

static bool sSnakeDebugLogged = false;

static bool occupied(const SnakeSegment* body, int len, uint8_t x, uint8_t y, bool ignoreTail) {
    int checkLen = ignoreTail && len > 0 ? len - 1 : len;
    for (int i = 0; i < checkLen; i++) {
        if (body[i].x == x && body[i].y == y) return true;
    }
    return false;
}

static bool inBody(const SnakeSegment* body, int len, uint8_t x, uint8_t y) {
    for (int i = 0; i < len; i++)
        if (body[i].x == x && body[i].y == y) return true;
    return false;
}

static uint8_t toCellIndex(uint8_t x, uint8_t y) {
    return (uint8_t)(y * WIDTH + x);
}

static int manhattanDist(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    return abs((int)x1 - (int)x2) + abs((int)y1 - (int)y2);
}

static bool gHamReady = false;
static SnakeSegment gHamOrder[LED_PIXEL_AMOUNT];
static int16_t gHamIndex[HEIGHT][WIDTH];

static inline int cycleDist(int fromIdx, int toIdx) {
    int d = toIdx - fromIdx;
    if (d < 0) d += LED_PIXEL_AMOUNT;
    return d;
}

static void initHamiltonianCycle() {
    if (gHamReady) return;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            gHamIndex[y][x] = -1;
        }
    }

    int pos = 0;

    // Construction for odd WIDTH + even HEIGHT (true for 11x10):
    // Keep first column for the final ascent to close the loop.
    if ((WIDTH % 2 == 1) && (HEIGHT % 2 == 0)) {
        for (int x = 0; x < WIDTH; x++) {
            gHamOrder[pos++] = { (uint8_t)x, (uint8_t)(HEIGHT - 1) };
        }

        for (int y = HEIGHT - 2; y >= 0; y--) {
            if (((HEIGHT - 2 - y) & 1) == 0) {
                for (int x = WIDTH - 1; x >= 1; x--) {
                    gHamOrder[pos++] = { (uint8_t)x, (uint8_t)y };
                }
            } else {
                for (int x = 1; x < WIDTH; x++) {
                    gHamOrder[pos++] = { (uint8_t)x, (uint8_t)y };
                }
            }
        }

        for (int y = 0; y <= HEIGHT - 2; y++) {
            gHamOrder[pos++] = { 0, (uint8_t)y };
        }
    } else {
        // Generic fallback: serpentine Hamiltonian path (not always cyclic).
        for (int y = 0; y < HEIGHT; y++) {
            if ((y & 1) == 0) {
                for (int x = 0; x < WIDTH; x++) {
                    gHamOrder[pos++] = { (uint8_t)x, (uint8_t)y };
                }
            } else {
                for (int x = WIDTH - 1; x >= 0; x--) {
                    gHamOrder[pos++] = { (uint8_t)x, (uint8_t)y };
                }
            }
        }
    }

    for (int i = 0; i < pos && i < LED_PIXEL_AMOUNT; i++) {
        gHamIndex[gHamOrder[i].y][gHamOrder[i].x] = i;
    }

    gHamReady = true;
}

static uint8_t countRecentVisits(const uint8_t* history, uint8_t count, uint8_t cell) {
    uint8_t hits = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (history[i] == cell) hits++;
    }
    return hits;
}

static void placeFood(const SnakeSegment* body, int len,
                      uint8_t& foodX, uint8_t& foodY) {
    uint8_t fx, fy;
    int tries = 0;
    do {
        fx = (uint8_t)random(0, WIDTH);
        fy = (uint8_t)random(0, HEIGHT);
        tries++;
    } while (inBody(body, len, fx, fy) && tries < 60);
    foodX = fx;
    foodY = fy;
}

// BFS von (sx,sy) nach (tx,ty).
// excludeLastSegment: das hinterste Koerpersegment wird NICHT blockiert gezaehlt
// (es bewegt sich beim naechsten Schritt frei).
struct BfsResult { bool found; int8_t dx, dy; };

static BfsResult bfsTo(const SnakeSegment* body, int bodyLen,
                       uint8_t sx, uint8_t sy,
                       uint8_t tx, uint8_t ty,
                       bool excludeLastSegment = false);

struct PathStep {
    int8_t dx;
    int8_t dy;
};

static bool bfsPath(const SnakeSegment* body, int bodyLen,
                    uint8_t sx, uint8_t sy,
                    uint8_t tx, uint8_t ty,
                    bool excludeLastSegment,
                    PathStep* outSteps,
                    int* outStepCount) {
    bool blocked[HEIGHT][WIDTH];
    memset(blocked, 0, sizeof(blocked));
    for (int i = 0; i < bodyLen; i++) {
        if (excludeLastSegment && i == bodyLen - 1) continue;
        if (body[i].x < WIDTH && body[i].y < HEIGHT)
            blocked[body[i].y][body[i].x] = true;
    }

    uint8_t parent[HEIGHT * WIDTH];
    memset(parent, 0xFF, sizeof(parent));

    uint8_t qx[WIDTH * HEIGHT], qy[WIDTH * HEIGHT];
    const uint8_t startCell = (uint8_t)(sy * WIDTH + sx);
    const uint8_t goalCell = (uint8_t)(ty * WIDTH + tx);

    blocked[sy][sx] = true;
    parent[startCell] = startCell;
    qx[0] = sx;
    qy[0] = sy;
    int head = 0, qtail = 1;

    const int8_t DX[4] = { 1, -1,  0,  0 };
    const int8_t DY[4] = { 0,  0,  1, -1 };

    while (head < qtail) {
        uint8_t cx = qx[head], cy = qy[head];
        head++;
        if ((uint8_t)(cy * WIDTH + cx) == goalCell) break;
        for (int d = 0; d < 4; d++) {
            int nx = (int)cx + DX[d];
            int ny = (int)cy + DY[d];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (blocked[ny][nx]) continue;
            blocked[ny][nx] = true;
            parent[ny * WIDTH + nx] = (uint8_t)(cy * WIDTH + cx);
            qx[qtail] = (uint8_t)nx;
            qy[qtail] = (uint8_t)ny;
            qtail++;
        }
    }

    if (parent[goalCell] == 0xFF) {
        *outStepCount = 0;
        return false;
    }

    PathStep reversed[WIDTH * HEIGHT];
    int revCount = 0;
    uint8_t cur = goalCell;
    while (cur != startCell && revCount < WIDTH * HEIGHT) {
        uint8_t prev = parent[cur];
        int cx = cur % WIDTH;
        int cy = cur / WIDTH;
        int px = prev % WIDTH;
        int py = prev / WIDTH;
        reversed[revCount++] = {
            (int8_t)(cx - px),
            (int8_t)(cy - py)
        };
        cur = prev;
    }

    for (int i = 0; i < revCount; i++) {
        outSteps[i] = reversed[revCount - 1 - i];
    }
    *outStepCount = revCount;
    return revCount > 0;
}

static bool simulatePathKeepsTailReachable(const SnakeSegment* body,
                                           int len,
                                           const PathStep* steps,
                                           int stepCount,
                                           uint8_t foodX,
                                           uint8_t foodY) {
    SnakeSegment sim[LED_PIXEL_AMOUNT];
    for (int i = 0; i < len; i++) sim[i] = body[i];
    int simLen = len;

    for (int s = 0; s < stepCount; s++) {
        int nx = (int)sim[0].x + steps[s].dx;
        int ny = (int)sim[0].y + steps[s].dy;
        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return false;

        bool willEat = ((uint8_t)nx == foodX && (uint8_t)ny == foodY);
        bool collides = occupied(sim, simLen, (uint8_t)nx, (uint8_t)ny, !willEat);
        if (collides) return false;

        if (willEat && simLen < LED_PIXEL_AMOUNT - 1) {
            for (int i = simLen; i > 0; i--) sim[i] = sim[i - 1];
            simLen++;
        } else {
            for (int i = simLen - 1; i > 0; i--) sim[i] = sim[i - 1];
        }
        sim[0].x = (uint8_t)nx;
        sim[0].y = (uint8_t)ny;
    }

    BfsResult toTail = bfsTo(sim, simLen,
                             sim[0].x, sim[0].y,
                             sim[simLen - 1].x, sim[simLen - 1].y,
                             true);
    return toTail.found;
}

static bool simulateSingleMove(const SnakeSegment* body,
                               int len,
                               int8_t dx,
                               int8_t dy,
                               uint8_t foodX,
                               uint8_t foodY,
                               SnakeSegment* outBody,
                               int* outLen,
                               uint8_t* outX,
                               uint8_t* outY,
                               bool* outEat) {
    int nx = (int)body[0].x + dx;
    int ny = (int)body[0].y + dy;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return false;

    bool willEat = ((uint8_t)nx == foodX && (uint8_t)ny == foodY);
    if (occupied(body, len, (uint8_t)nx, (uint8_t)ny, !willEat)) return false;

    for (int i = 0; i < len; i++) outBody[i] = body[i];
    int simLen = len;
    if (willEat && simLen < LED_PIXEL_AMOUNT) {
        for (int i = simLen; i > 0; i--) outBody[i] = outBody[i - 1];
        simLen++;
    } else {
        for (int i = simLen - 1; i > 0; i--) outBody[i] = outBody[i - 1];
    }
    outBody[0].x = (uint8_t)nx;
    outBody[0].y = (uint8_t)ny;

    *outLen = simLen;
    *outX = (uint8_t)nx;
    *outY = (uint8_t)ny;
    *outEat = willEat;
    return true;
}

static BfsResult bfsTo(const SnakeSegment* body, int bodyLen,
                       uint8_t sx, uint8_t sy,
                       uint8_t tx, uint8_t ty,
                       bool excludeLastSegment) {
    // WIDTH=11, HEIGHT=10 -> max 110 Zellen, Indizes passen in uint8_t
    bool blocked[HEIGHT][WIDTH];
    memset(blocked, 0, sizeof(blocked));
    for (int i = 0; i < bodyLen; i++) {
        if (excludeLastSegment && i == bodyLen - 1) continue;
        if (body[i].x < WIDTH && body[i].y < HEIGHT)
            blocked[body[i].y][body[i].x] = true;
    }

    // parent[cell] = Eltern-Cell-Index (y*WIDTH+x); 0xFF = unbesucht
    uint8_t parent[HEIGHT * WIDTH];
    memset(parent, 0xFF, sizeof(parent));

    uint8_t qx[WIDTH * HEIGHT], qy[WIDTH * HEIGHT];
    const uint8_t startCell = (uint8_t)(sy * WIDTH + sx);
    blocked[sy][sx] = true;
    parent[startCell] = startCell;  // Startmarker zeigt auf sich selbst
    qx[0] = sx; qy[0] = sy;
    int head = 0, qtail = 1;
    bool found = false;

    const int8_t DX[4] = { 1, -1,  0,  0 };
    const int8_t DY[4] = { 0,  0,  1, -1 };

    while (head < qtail && !found) {
        uint8_t cx = qx[head], cy = qy[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = (int)cx + DX[d], ny = (int)cy + DY[d];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (blocked[ny][nx]) continue;
            blocked[ny][nx] = true;
            parent[ny * WIDTH + nx] = (uint8_t)(cy * WIDTH + cx);
            qx[qtail] = (uint8_t)nx; qy[qtail] = (uint8_t)ny; qtail++;
            if ((uint8_t)nx == tx && (uint8_t)ny == ty) { found = true; break; }
        }
    }
    if (!found) return { false, 0, 0 };

    // Pfad zurueckverfolgen: ersten Schritt vom Start ermitteln
    uint8_t cur = (uint8_t)(ty * WIDTH + tx);
    while (parent[cur] != startCell) cur = parent[cur];
    // cur ist jetzt der erste Schritt direkt nach Start
    return { true,
             (int8_t)((int)(cur % WIDTH) - (int)sx),
             (int8_t)((int)(cur / WIDTH) - (int)sy) };
}

// Board-Partition: Flood-Fill + prüft ob Futter erreichbar ist
// Gibt Anzahl erreichbarer Felder zurück, setzt *outFoodReachable
static int checkBoardPartition(const SnakeSegment* body, int bodyLen,
                               uint8_t startX, uint8_t startY,
                               uint8_t foodX, uint8_t foodY,
                               bool excludeLastSegment,
                               bool* outFoodReachable) {
    bool blocked[HEIGHT][WIDTH];
    memset(blocked, 0, sizeof(blocked));
    for (int i = 0; i < bodyLen; i++) {
        if (excludeLastSegment && i == bodyLen - 1) continue;
        if (body[i].x < WIDTH && body[i].y < HEIGHT)
            blocked[body[i].y][body[i].x] = true;
    }
    uint8_t qx[WIDTH * HEIGHT], qy[WIDTH * HEIGHT];
    blocked[startY][startX] = true;
    qx[0] = startX; qy[0] = startY;
    int head = 0, tail = 1;
    bool foodFound = (startX == foodX && startY == foodY);
    const int8_t DX[4] = { 1, -1, 0, 0 };
    const int8_t DY[4] = { 0, 0, 1, -1 };
    while (head < tail) {
        uint8_t cx = qx[head], cy = qy[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = (int)cx + DX[d], ny = (int)cy + DY[d];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (blocked[ny][nx]) continue;
            blocked[ny][nx] = true;
            qx[tail] = (uint8_t)nx; qy[tail] = (uint8_t)ny; tail++;
            if ((uint8_t)nx == foodX && (uint8_t)ny == foodY) foodFound = true;
        }
    }
    *outFoodReachable = foodFound;
    return tail;
}

// BFS-Flood-Fill: zaehlt erreichbare freie Felder (inkl. Startfeld)
static int floodFillCount(const SnakeSegment* body, int bodyLen,
                          uint8_t startX, uint8_t startY,
                          bool excludeLastSegment = false) {
    bool blocked[HEIGHT][WIDTH];
    memset(blocked, 0, sizeof(blocked));
    for (int i = 0; i < bodyLen; i++) {
        if (excludeLastSegment && i == bodyLen - 1) continue;
        if (body[i].x < WIDTH && body[i].y < HEIGHT)
            blocked[body[i].y][body[i].x] = true;
    }
    uint8_t qx[WIDTH * HEIGHT], qy[WIDTH * HEIGHT];
    blocked[startY][startX] = true;
    qx[0] = startX; qy[0] = startY;
    int head = 0, tail = 1;
    const int8_t DX[4] = { 1, -1,  0,  0 };
    const int8_t DY[4] = { 0,  0,  1, -1 };
    while (head < tail) {
        uint8_t cx = qx[head], cy = qy[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = (int)cx + DX[d], ny = (int)cy + DY[d];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && !blocked[ny][nx]) {
                blocked[ny][nx] = true;
                qx[tail] = (uint8_t)nx; qy[tail] = (uint8_t)ny; tail++;
            }
        }
    }
    return tail;
}

void SnakeEffect::update() {
    if (!sSnakeDebugLogged) {
        DebugManager::println(DebugCategory::Effects, "[FX] snake active");
        sSnakeDebugLogged = true;
    }

    static uint16_t stepsWithoutFood = 0;
    static constexpr uint8_t VISIT_HISTORY = 24;
    static uint8_t headVisitHistory[VISIT_HISTORY];
    static uint8_t headVisitCount = 0;
    static uint8_t headVisitPos = 0;

    const uint16_t frameDelay = speedToDelay(60, 400);
    unsigned long nowMs = millis();

    // =========================================================
    // DEATH ANIMATION: rotes Blinken und Schrumpfen
    // =========================================================
    if (_deathActive) {
        if (nowMs - _deathLastFrame < 60) return;
        _deathLastFrame = nowMs;

        // Schrumpfe die Schlange von hinten (visuell)
        int remaining = _len - _deathStep;
        if (remaining < 0) remaining = 0;

        // Alles schwarz
        ledMatrix.clear();

        // Zeichne schrumpfende Schlange in Rot
        for (int i = 0; i < remaining; i++) {
            // Blinken: gerade Schritte = rot, ungerade = dunkler rot
            uint8_t bright = (_deathStep % 2 == 0) ? 200 : 80;
            uint32_t c = makeColorWithBrightness(bright, 0, 0);
            ledMatrix.setPixelXYDirect(_body[i].x, _body[i].y, c);
        }

        strip->show();
        _deathStep += 2;

        if (_deathStep >= _len + 6) {
            _deathActive = false;
            // Nach Tod: sofort neue Runde oder auf Highscore-Anim warten
            if (!_celebActive) {
                ledMatrix.clear();
                strip->show();
            }
        }
        return;
    }

    // =========================================================
    // HIGHSCORE ANIMATION: goldenes Funkeln
    // =========================================================
    if (_celebActive) {
        if (nowMs - _celebLastFrame < 40) return;
        _celebLastFrame = nowMs;

        const int sparkleDiv = max(2, (int)densityMap(6, 2));

        // Goldenes Feuerwerk: zufällige Pixel leuchten gold/gelb auf
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t r = (random(0, sparkleDiv) == 0) ? (uint8_t)random(180, 255) : 0;
                uint8_t g = r ? (uint8_t)(r * 0.75f) : 0;
                uint32_t c = r ? makeColorWithBrightness(r, g, 0) : 0;
                ledMatrix.setPixelXYDirect(x, y, c);
            }
        }
        // Highscore-Len als heller Streifen
        int hs = min(_highscore, WIDTH * HEIGHT - 1);
        for (int i = 0; i < hs; i++) {
            int px = i % WIDTH, py = i / WIDTH;
            if (_celebStep % 3 == 0)
                ledMatrix.setPixelXYDirect(px, py, makeColorWithBrightness(255, 200, 0));
        }
        strip->show();
        _celebStep++;

        if (_celebStep >= 30) {
            _celebActive = false;
            ledMatrix.clear();
            strip->show();
        }
        return;
    }

    if (millis() - _lastFrame < frameDelay) return;
    _lastFrame = millis();

    // --- Initialisierung ---
    const int startLen = 2;

    if (!_seeded || _len == 0) {
        _len = startLen;
        _dx  = 1; _dy = 0;
        _hue = (uint16_t)random(0, 65536);
        stepsWithoutFood = 0;
        headVisitCount = 0;
        headVisitPos = 0;
        uint8_t sx = WIDTH / 2, sy = HEIGHT / 2;
        for (int i = 0; i < _len; i++) {
            _body[i].x = (uint8_t)max(0, (int)sx - i);
            _body[i].y = sy;
        }
        placeFood(_body, _len, _foodX, _foodY);
        _seeded = true;
    }

    // =========================================================
    // KI v3: Hamiltonian Endgame + Multi-Faktor Scoring
    // =========================================================
    {
        uint8_t hx = _body[0].x, hy = _body[0].y;
        const int8_t DX[4] = { 1, -1,  0,  0 };
        const int8_t DY[4] = { 0,  0,  1, -1 };

        uint8_t intens = clampIntensity();

        // Längenbasierter Vorsichts-Faktor (wie ein Mensch: je voller das Feld, desto vorsichtiger)
        // Bei 10% Länge: kein Einfluss. Bei 80% Länge: -50 Risikopunkte (auf 0..100 Skala)
        float fillRatio = (float)_len / (float)LED_PIXEL_AMOUNT;  // 0.0..1.0
        // Längenmalus: greift erst ab 20% Füllung, wird bis 80% maximal (50 Punkte Abzug)
        float lenMalus = 0.0f;
        if (fillRatio > 0.2f) {
            lenMalus = ((fillRatio - 0.2f) / 0.6f) * 50.0f;  // linear 0→50 bei 20%→80%
            if (lenMalus > 50.0f) lenMalus = 50.0f;
        }
        // Effektive Intensity: Intensity minus Längenmalus (mindestens 1)
        float effectiveIntens = (float)intens - lenMalus;
        if (effectiveIntens < 1.0f) effectiveIntens = 1.0f;

        float risk = (effectiveIntens - 1.0f) / 99.0f;  // 0.0=defensiv, 1.0=riskant
        bool strictConservative = (effectiveIntens <= 25.0f);

        // Hamiltonian greift erst sehr spät (85%+ Füllung) – davor spielt die KI riskanter
        // Bei extremen Längen (>95%) immer sicher, egal wie hoch die Intensity
        bool endgameSafeMode = (fillRatio >= 0.95f) ||
                               ((fillRatio >= 0.85f) && (effectiveIntens <= 70.0f));

        // Gamble-Würfel: Bei höherer Intensity gelegentlich den sicheren Hamiltonpfad
        // überspringen → sorgt für Abwechslung und verhindert 100%-Win-Rate
        // intens=25 → ~6%,  intens=50 → ~12%,  intens=75 → ~18%,  intens=100 → ~25%
        bool hamGamble = !strictConservative && ((uint8_t)random(100) < (intens / 4));

        int8_t bestDx = _dx, bestDy = _dy;
        int bestScore = -2000000;
        bool anyValid = false;

        // Known robust strategy: Hamiltonian cycle for tight endgame.
        if ((strictConservative || endgameSafeMode) && !hamGamble) {
            initHamiltonianCycle();
            int headIdx = gHamIndex[hy][hx];
            int tailIdx = gHamIndex[_body[_len - 1].y][_body[_len - 1].x];
            int foodIdx = gHamIndex[_foodY][_foodX];

            int nextIdx = (headIdx + 1) % LED_PIXEL_AMOUNT;
            int prevIdx = (headIdx - 1 + LED_PIXEL_AMOUNT) % LED_PIXEL_AMOUNT;

            for (int d = 0; d < 4; d++) {
                int nx = (int)hx + DX[d], ny = (int)hy + DY[d];
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                bool willEat = ((uint8_t)nx == _foodX && (uint8_t)ny == _foodY);
                if (occupied(_body, _len, (uint8_t)nx, (uint8_t)ny, !willEat)) continue;

                int ni = gHamIndex[ny][nx];
                if (ni < 0) continue;

                // Avoid moving backwards on cycle in conservative mode.
                if (strictConservative && ni == prevIdx) continue;

                // Keep tail-space: do not squeeze tail interval too hard.
                int gapToTail = cycleDist(ni, tailIdx);
                if (gapToTail <= 1 && !(ni == tailIdx && !willEat)) continue;

                int score = 0;
                int toFood = cycleDist(ni, foodIdx);

                if (ni == nextIdx) score += 6000;
                if (!strictConservative) {
                    // Safe shortcut only in non-strict mode.
                    score += (LED_PIXEL_AMOUNT - toFood) * 8;
                }

                // Safety reinforcement via tail-reachability.
                SnakeSegment sim[LED_PIXEL_AMOUNT];
                int simLen = _len;
                uint8_t sx = 0, sy = 0;
                bool simEat = false;
                if (!simulateSingleMove(_body, _len, DX[d], DY[d], _foodX, _foodY,
                                        sim, &simLen, &sx, &sy, &simEat)) {
                    continue;
                }
                BfsResult toTail = bfsTo(sim, simLen, sim[0].x, sim[0].y,
                                         sim[simLen - 1].x, sim[simLen - 1].y, true);
                if (!toTail.found) continue;

                anyValid = true;
                if (score > bestScore) { bestScore = score; bestDx = DX[d]; bestDy = DY[d]; }
            }
        }

        // Main scoring mode (or fallback if Hamiltonian did not find move)
        if (!anyValid) {
            bool doTailCheck = (risk < 0.72f);
            int freePenaltyWeight = (int)((1.0f - risk) * 12.0f);
            int foodDist = manhattanDist(hx, hy, _foodX, _foodY);

            for (int d = 0; d < 4; d++) {
                int nx = (int)hx + DX[d], ny = (int)hy + DY[d];
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                bool willEat = ((uint8_t)nx == _foodX && (uint8_t)ny == _foodY);
                if (occupied(_body, _len, (uint8_t)nx, (uint8_t)ny, !willEat)) continue;

                SnakeSegment sim[LED_PIXEL_AMOUNT];
                int simLen = _len;
                uint8_t sx = 0, sy = 0;
                bool simEat = false;
                if (!simulateSingleMove(_body, _len, DX[d], DY[d], _foodX, _foodY,
                                        sim, &simLen, &sx, &sy, &simEat)) {
                    continue;
                }

                int score = 0;
                bool foodReachable = false;
                int freeAfter = checkBoardPartition(sim, simLen,
                                                    sx, sy,
                                                    _foodX, _foodY,
                                                    true, &foodReachable);
                score += freeAfter * 14;

                if (!willEat && !foodReachable) score -= 4200;

                BfsResult toTail = bfsTo(sim, simLen,
                                         sim[0].x, sim[0].y,
                                         sim[simLen - 1].x, sim[simLen - 1].y,
                                         true);
                if (!toTail.found) score -= doTailCheck ? 3200 : 600;

                if (willEat) {
                    score += 10000;
                } else if (foodReachable) {
                    int newDist = manhattanDist(sx, sy, _foodX, _foodY);
                    score += (foodDist - newDist) * (int)(170 + risk * 80.0f);
                    score += 280;
                }

                int freeTotal = LED_PIXEL_AMOUNT - simLen;
                int threshold = freeTotal * (int)(72.0f - risk * 67.0f) / 100;
                if (freeAfter < threshold && freeTotal > 0) {
                    score -= (threshold - freeAfter) * freePenaltyWeight;
                }

                // One-step lookahead: avoid moves that force a narrow tunnel.
                int branch = 0;
                for (int k = 0; k < 4; k++) {
                    int lx = (int)sx + DX[k], ly = (int)sy + DY[k];
                    if (lx < 0 || lx >= WIDTH || ly < 0 || ly >= HEIGHT) continue;
                    bool eat2 = ((uint8_t)lx == _foodX && (uint8_t)ly == _foodY);
                    if (occupied(sim, simLen, (uint8_t)lx, (uint8_t)ly, !eat2)) continue;
                    branch++;
                }
                score += branch * 220;
                if (branch <= 1) score -= 900;

                uint8_t revisits = countRecentVisits(
                    headVisitHistory, headVisitCount, toCellIndex((uint8_t)nx, (uint8_t)ny));
                score -= revisits * 340;

                if (DX[d] == _dx && DY[d] == _dy) score += 35;

                if (risk > 0.5f && foodReachable) {
                    BfsResult fBfs = bfsTo(sim, simLen,
                                           sim[0].x, sim[0].y,
                                           _foodX, _foodY, false);
                    if (fBfs.found && fBfs.dx == DX[d] && fBfs.dy == DY[d]) {
                        score += (int)(risk * 2200);
                    }
                }

                anyValid = true;
                if (score > bestScore) { bestScore = score; bestDx = DX[d]; bestDy = DY[d]; }
            }
        }

        if (!anyValid) {
            for (int d = 0; d < 4; d++) {
                int nx = (int)hx + DX[d], ny = (int)hy + DY[d];
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                if (inBody(_body, _len, (uint8_t)nx, (uint8_t)ny)) continue;
                bestDx = DX[d]; bestDy = DY[d];
                break;
            }
        }

        _dx = bestDx;
        _dy = bestDy;
    }

    // --- Schritt ausfuehren ---
    uint8_t newX = (uint8_t)((int)_body[0].x + _dx);
    uint8_t newY = (uint8_t)((int)_body[0].y + _dy);
    bool ate = (newX == _foodX && newY == _foodY);

    // Wichtig: Das letzte Segment darf betreten werden, wenn NICHT gefressen wird,
    // weil der Schwanz im selben Tick weiterzieht.
    bool hitSelf = occupied(_body, _len, newX, newY, !ate);
    bool died = (newX >= WIDTH || newY >= HEIGHT || hitSelf);

    if (died) {
        // Check for new highscore before dying
        if (_len > _highscore) {
            _highscore = _len;
            _isNewHighscore = true;
            DebugManager::printf(DebugCategory::Effects, "[FX][snake] new highscore=%d\n", _highscore);
            _celebStep = 0;
            _celebActive = true;
            _celebLastFrame = millis();
        } else {
            _isNewHighscore = false;
        }
        DebugManager::printf(DebugCategory::Effects, "[FX][snake] died len=%d at (%u,%u)\n", _len, newX, newY);
        // Trigger death animation
        _deathStep = 0;
        _deathActive = true;
        _deathLastFrame = millis();
        _seeded = false;
        return;
    }

    if (ate && _len < MAX_LEN) {
        for (int i = _len; i > 0; i--) _body[i] = _body[i - 1];
        _len++;
        DebugManager::printf(DebugCategory::Effects, "[FX][snake] food eaten len=%d\n", _len);
    } else {
        for (int i = _len - 1; i > 0; i--) _body[i] = _body[i - 1];
    }
    _body[0].x = newX;
    _body[0].y = newY;

    if (ate) {
        // Perfect Win: Schlange füllt das ganze Feld!
        if (_len >= LED_PIXEL_AMOUNT) {
            _highscore = _len;
            _isNewHighscore = true;
            DebugManager::println(DebugCategory::Effects, "[FX][snake] perfect win reached");
            _celebStep = 0;
            _celebActive = true;
            _celebLastFrame = millis();
            _deathStep = 0;
            _deathActive = true;
            _deathLastFrame = millis();
            _seeded = false;
            return;
        }
        
        placeFood(_body, _len, _foodX, _foodY);
        _hue += 3000;
        stepsWithoutFood = 0;
    } else {
        stepsWithoutFood++;
    }

    headVisitHistory[headVisitPos] = toCellIndex(_body[0].x, _body[0].y);
    headVisitPos = (uint8_t)((headVisitPos + 1) % VISIT_HISTORY);
    if (headVisitCount < VISIT_HISTORY) headVisitCount++;

    // --- Rendern: 3-Farben-System (Schlange / Hintergrund / Futter) ---
    // Basis-Hue aus globalem Color-Setting oder laufendem _hue
    bool     useColor = (color != 0);
    uint16_t baseHue;

    if (useColor) {
        // RGB -> HSV-Hue (0..65535)
        uint8_t r0 = colR(color), g0 = colG(color), b0 = colB(color);
        uint8_t mx = max(r0, max(g0, b0));
        uint8_t mn = min(r0, min(g0, b0));
        if (mx == mn) {
            baseHue = 0;
        } else {
            uint8_t d = mx - mn;
            int32_t h;
            if      (mx == r0) h = (int32_t)(g0 - b0) * 60 / d + (g0 < b0 ? 360 : 0);
            else if (mx == g0) h = (int32_t)(b0 - r0) * 60 / d + 120;
            else               h = (int32_t)(r0 - g0) * 60 / d + 240;
            baseHue = (uint16_t)((uint32_t)((h % 360 + 360) % 360) * 65535 / 360);
        }
    } else {
        baseHue = _hue;
    }

    const uint16_t bgHue   = (uint16_t)(baseHue + 32768U); // Komplementaer  (180 deg)
    const uint16_t foodHue = (uint16_t)(baseHue + 21845U); // Triadisch      (120 deg)
    const uint8_t bgSat = (uint8_t)densityMap(140, 220);
    const uint8_t bgVal = (uint8_t)densityMap(16, 72);
    const uint8_t foodVal = (uint8_t)intensityMap(180, 255);

    // Hintergrund: alle freien Pixel in gedimmter Komplementaerfarbe
    {
        uint32_t bgRaw = ledMatrix.colorHSV(bgHue, bgSat, bgVal);
        uint32_t bgC   = makeColorWithBrightness(
            (bgRaw >> 16) & 0xFF, (bgRaw >> 8) & 0xFF, bgRaw & 0xFF);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (!inBody(_body, _len, (uint8_t)x, (uint8_t)y) &&
                    !((uint8_t)x == _foodX && (uint8_t)y == _foodY)) {
                    ledMatrix.setPixelXYDirect(x, y, bgC);
                }
            }
        }
    }

    // Schlange: Farbverlauf Kopf -> Schwanz; Intensity skaliert Gesamthelligkeit
    const float bodyBright = intensityMapF(0.50f, 1.0f);
    for (int i = 0; i < _len; i++) {
        float    fade = (1.0f - (float)i / (float)_len * 0.75f) * bodyBright;
        uint32_t c;
        if (useColor) {
            c = makeColorWithBrightness(
                (uint8_t)(colR(color) * fade),
                (uint8_t)(colG(color) * fade),
                (uint8_t)(colB(color) * fade));
        } else {
            uint16_t segHue = baseHue + (uint16_t)(i * 800);
            uint32_t raw    = ledMatrix.colorHSV(segHue, 230, (uint8_t)(fade * 255));
            c = makeColorWithBrightness(
                (raw >> 16) & 0xFF, (raw >> 8) & 0xFF, raw & 0xFF);
        }
        ledMatrix.setPixelXYDirect(_body[i].x, _body[i].y, c);
    }

    // Futter: triadische Farbe (120 deg versetzt von Schlange)
    {
        uint32_t fRaw = ledMatrix.colorHSV(foodHue, 255, foodVal);
        ledMatrix.setPixelXYDirect(_foodX, _foodY, makeColorWithBrightness(
            (fRaw >> 16) & 0xFF, (fRaw >> 8) & 0xFF, fRaw & 0xFF));
    }

    strip->show();
}
