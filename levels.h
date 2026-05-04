#ifndef LEVELS_H
#define LEVELS_H

#include <math.h>
#include <stdio.h>

#define TOTAL_LEVELS 6

struct LevelDesc {
    const char *title, *subtitle;
    int asteroidCount, mineCount;
    bool barriersEnabled, warpGateEnabled;
    int enemyType0Count, enemyType1Count;
    float enemySpawnInterval, asteroidSpeedMult, asteroidSpinMult;
    int killGoal;
    float surviveTime;
    const char *objLine1, *objLine2;
};

static const LevelDesc LEVEL_DEFS[TOTAL_LEVELS] = {
    {
        "LEVEL 1", "Asteroids",
        5, 3, false, false,
        0, 0, 9999.f, 1.0f, 1.0f,
        0, 20.f,
        "Survive 20 seconds", ""
    },
    {
        "LEVEL 2", "Minefield",
        8, 6, false, false,
        2, 0, 6.0f, 1.25f, 1.4f,
        6, 0.f,
        "Destroy 6 enemies", ""
    },
    {
        "LEVEL 3", "Zone",
        6, 4, true, false,
        3, 1, 4.5f, 1.4f, 1.6f,
        10, 0.f,
        "Destroy 10 enemies", ""
    },
    {
        "LEVEL 4", "Warp",
        8, 5, true, true,
        2, 2, 3.5f, 1.6f, 1.9f,
        12, 0.f,
        "Destroy 12 enemies", ""
    },
    {
        "LEVEL 5", "Swarm",
        4, 3, false, true,
        4, 4, 2.0f, 1.5f, 1.2f,
        18, 0.f,
        "Destroy 18 enemies", ""
    },
    {
        "LEVEL 6", "Finale",
        8, 6, true, true,
        4, 4, 1.5f, 2.0f, 2.2f,
        25, 0.f,
        "Destroy 25 enemies", ""
    }
};

// level runtime state
static int   lv_current       = 0;
static bool  lv_complete      = false;
static float lv_completeTimer = 0;

// pause-menu level select
bool lv_selectOpen = false;

static int lv_selectCursor = 0;

// objective tracking
static int   lv_killCount    = 0;
static float lv_surviveTimer = 0;

// asteroid, mine, and barrier layout templates
struct AstSpec {
    float x, y;
    int size;
    float spd, spin;
};

static const AstSpec AST_L1[5] = {
    { 0, 0, ASTEROID_LARGE,  35, -0.4f },
    { 0, 0, ASTEROID_MEDIUM, 45,  0.6f },
    { 0, 0, ASTEROID_SMALL,  55, -0.9f },
    { 0, 0, ASTEROID_SMALL,  50,  0.7f },
    { 0, 0, ASTEROID_MEDIUM, 40, -0.5f }
};

static const AstSpec AST_L2[8] = {
    { 0, 0, ASTEROID_LARGE,  40, -0.5f },
    { 0, 0, ASTEROID_LARGE,  42,  0.55f },
    { 0, 0, ASTEROID_MEDIUM, 52, -0.8f },
    { 0, 0, ASTEROID_MEDIUM, 48,  0.75f },
    { 0, 0, ASTEROID_SMALL,  65, -1.1f },
    { 0, 0, ASTEROID_SMALL,  60,  0.95f },
    { 0, 0, ASTEROID_SMALL,  58, -0.85f },
    { 0, 0, ASTEROID_MEDIUM, 50,  0.65f }
};

static const AstSpec AST_L3[6] = {
    { 0, 0, ASTEROID_LARGE,  44, -0.55f },
    { 0, 0, ASTEROID_LARGE,  46,  0.6f },
    { 0, 0, ASTEROID_MEDIUM, 55, -0.9f },
    { 0, 0, ASTEROID_SMALL,  68,  1.0f },
    { 0, 0, ASTEROID_SMALL,  63, -1.05f },
    { 0, 0, ASTEROID_MEDIUM, 52,  0.7f }
};

static const AstSpec AST_L4[8] = {
    { 0, 0, ASTEROID_LARGE,  50, -0.7f },
    { 0, 0, ASTEROID_LARGE,  54,  0.8f },
    { 0, 0, ASTEROID_MEDIUM, 62, -1.05f },
    { 0, 0, ASTEROID_SMALL,  75,  1.2f },
    { 0, 0, ASTEROID_SMALL,  72, -1.15f },
    { 0, 0, ASTEROID_MEDIUM, 58,  0.85f },
    { 0, 0, ASTEROID_MEDIUM, 60, -0.9f },
    { 0, 0, ASTEROID_SMALL,  80,  1.3f }
};

static const AstSpec AST_L5[4] = {
    { 0, 0, ASTEROID_MEDIUM, 55, -0.8f },
    { 0, 0, ASTEROID_MEDIUM, 58,  0.75f },
    { 0, 0, ASTEROID_SMALL,  70, -1.1f },
    { 0, 0, ASTEROID_SMALL,  65,  0.9f }
};

static const AstSpec AST_L6[8] = {
    { 0, 0, ASTEROID_LARGE,  60, -0.9f },
    { 0, 0, ASTEROID_LARGE,  64,  1.0f },
    { 0, 0, ASTEROID_LARGE,  58, -0.8f },
    { 0, 0, ASTEROID_MEDIUM, 72,  1.1f },
    { 0, 0, ASTEROID_MEDIUM, 70, -1.05f },
    { 0, 0, ASTEROID_SMALL,  88,  1.4f },
    { 0, 0, ASTEROID_SMALL,  85, -1.35f },
    { 0, 0, ASTEROID_SMALL,  90,  1.5f }
};

static const AstSpec *AST_TABLES[TOTAL_LEVELS] = {
    AST_L1, AST_L2, AST_L3, AST_L4, AST_L5, AST_L6
};

static const int AST_TABLE_COUNT[TOTAL_LEVELS] = {
    5, 8, 6, 8, 4, 8
};

struct MineSpec {
    float x, y, r;
};

static const MineSpec MINE_L1[3] = {
    { 0, 0, 16 },
    { 0, 0, 14 },
    { 0, 0, 15 }
};

static const MineSpec MINE_L2[6] = {
    { 0, 0, 16 },
    { 0, 0, 15 },
    { 0, 0, 14 },
    { 0, 0, 16 },
    { 0, 0, 15 },
    { 0, 0, 14 }
};

static const MineSpec MINE_L3[4] = {
    { 0, 0, 16 },
    { 0, 0, 16 },
    { 0, 0, 15 },
    { 0, 0, 14 }
};

static const MineSpec MINE_L4[5] = {
    { 0, 0, 16 },
    { 0, 0, 16 },
    { 0, 0, 15 },
    { 0, 0, 15 },
    { 0, 0, 14 }
};

static const MineSpec MINE_L5[3] = {
    { 0, 0, 16 },
    { 0, 0, 16 },
    { 0, 0, 15 }
};

static const MineSpec MINE_L6[6] = {
    { 0, 0, 16 },
    { 0, 0, 16 },
    { 0, 0, 15 },
    { 0, 0, 15 },
    { 0, 0, 14 },
    { 0, 0, 14 }
};

static const MineSpec *MINE_TABLES[TOTAL_LEVELS] = {
    MINE_L1, MINE_L2, MINE_L3, MINE_L4, MINE_L5, MINE_L6
};

static const int MINE_TABLE_COUNT[TOTAL_LEVELS] = {
    3, 6, 4, 5, 3, 6
};

struct BarSpec {
    float x, y, w, h, ang;
};

static const BarSpec BAR_L3[3] = {
    { 210, 240, 18, 200, 0 },
    { 430, 240, 18, 200, 0 },
    { 320, 100, 220, 18, 0 }
};

static const BarSpec BAR_L4[3] = {
    { 180, 300, 20, 160,  0.4f },
    { 460, 300, 20, 160, -0.4f },
    { 320, 420, 240, 18, 0 }
};

static const BarSpec BAR_L6[3] = {
    { 320, 240, 18, 260,  0 },
    { 180, 320, 18, 160,  0.52f },
    { 460, 160, 18, 160, -0.52f }
};

// pick a spawn point just outside the screen
static inline void lv_offscreenSpawn(float &outX, float &outY, float margin = 80)
{
    int side = rand() % 4;

    if (side == 0) {
        outX = -margin;
        outY = (float)(rand() % g_yres);
    } else if (side == 1) {
        outX = g_xres + margin;
        outY = (float)(rand() % g_yres);
    } else if (side == 2) {
        outX = (float)(rand() % g_xres);
        outY = g_yres + margin;
    } else {
        outX = (float)(rand() % g_xres);
        outY = -margin;
    }
}

// load a level
static void lv_loadLevel(int idx)
{
    if (idx < 0 || idx >= TOTAL_LEVELS)
        return;

    lv_current = idx;
    lv_complete = false;
    lv_completeTimer = 0;

    // reset objective counters
    lv_killCount = 0;
    lv_surviveTimer = 0;

    const LevelDesc &ld = LEVEL_DEFS[idx];

    obstaclesRemoveAllAsteroids();
    obstaclesRemoveAllMines();

    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].active = false;

    obs_gate.active = false;

    const AstSpec *atab = AST_TABLES[idx];
    int acnt = AST_TABLE_COUNT[idx];
    float sm = ld.asteroidSpeedMult;
    float rm = ld.asteroidSpinMult;

    for (int i = 0; i < acnt && i < ld.asteroidCount; i++) {
        float sx, sy;
        lv_offscreenSpawn(sx, sy);
        obstaclesAddAsteroid(sx, sy, atab[i].size, atab[i].spd * sm, atab[i].spin * rm);
    }

    const MineSpec *mtab = MINE_TABLES[idx];
    int mcnt = MINE_TABLE_COUNT[idx];

    for (int i = 0; i < mcnt && i < ld.mineCount; i++) {
        float sx, sy;
        lv_offscreenSpawn(sx, sy);
        obstaclesAddMine(sx, sy, mtab[i].r);
    }

    if (ld.barriersEnabled) {
        const BarSpec *btab = nullptr;

        if (idx == 2)
            btab = BAR_L3;
        else if (idx == 3)
            btab = BAR_L4;
        else if (idx == 5)
            btab = BAR_L6;

        if (btab) {
            for (int i = 0; i < OBS_BARRIER_COUNT; i++) {
                obs_barriers[i].spawn(btab[i].x, btab[i].y,
                                      btab[i].w, btab[i].h,
                                      btab[i].ang);
            }
        }
    }

    if (ld.warpGateEnabled)
        obs_gate.spawn(g_xres * 0.5f, g_yres * 0.5f, 45);

    for (int i = 0; i < ld.enemyType0Count; i++) {
        float ex, ey;
        lv_offscreenSpawn(ex, ey);
        spawn_enemy(ex, ey, g_xres, g_yres);
    }

    for (int i = 0; i < ld.enemyType1Count; i++) {
        float ex, ey;
        lv_offscreenSpawn(ex, ey);
        spawn_enemy(ex, ey, g_xres, g_yres);
    }

    g.spawnInterval = ld.enemySpawnInterval;
    g.spawnTimer = 0;
}

// initialize level state
void levelsInit()
{
    lv_current = 0;
    lv_selectOpen = false;
    lv_selectCursor = 0;
    lv_killCount = 0;
    lv_surviveTimer = 0;
}

// called when an enemy actually dies
void levelsOnEnemyKilled()
{
    lv_killCount++;
}

// update level objective progress
void levelsUpdate(float dt)
{
    if (lv_complete)
        return;

    const LevelDesc &ld = LEVEL_DEFS[lv_current];

    lv_surviveTimer += dt;

    // levels use either kills or survival time
    bool done = false;

    if (ld.killGoal > 0) {
        if (lv_killCount >= ld.killGoal)
            done = true;
    } else if (ld.surviveTime > 0) {
        if (lv_surviveTimer >= ld.surviveTime)
            done = true;
    }

    if (done) {
        lv_complete = true;
        lv_completeTimer = 0;
        g.state = STATE_LEVEL_COMPLETE;
    }
}

// draw level hud
void levelsRenderHUD()
{
    const LevelDesc &ld = LEVEL_DEFS[lv_current];

    Rect r;
    r.bot = g.yres - 18;
    r.left = g.xres / 2;
    r.center = 1;

    ggprint12(&r, 0, 0x0088ffff, "%s", ld.title);

    r.bot -= 18;

    if (ld.killGoal > 0) {
        int k = lv_killCount;

        if (k > ld.killGoal)
            k = ld.killGoal;

        ggprint12(&r, 0, 0x00ffcc44, "Enemies: %d / %d", k, ld.killGoal);
    } else if (ld.surviveTime > 0) {
        int rem = (int)ceilf(ld.surviveTime - lv_surviveTimer);

        if (rem < 0)
            rem = 0;

        ggprint12(&r, 0, 0x00ffcc44, "Survive: %ds", rem);
    }
}

// level complete screen
void levelsRenderComplete()
{
    const LevelDesc &ld = LEVEL_DEFS[lv_current];
    float t = lv_completeTimer;
    float alpha = (t < 0.4f) ? (t / 0.4f) : 1.0f;

    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.70f * alpha);

    glBegin(GL_QUADS);
    glVertex2f(0,      0);
    glVertex2f(g.xres, 0);
    glVertex2f(g.xres, g.yres);
    glVertex2f(0,      g.yres);
    glEnd();

    float pw = 400;
    float ph = 240;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glColor4f(0.02f, 0.10f, 0.04f, 0.94f * alpha);

    glBegin(GL_QUADS);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 3.5f);

    glLineWidth(2.5f);
    glColor4f(0.1f + 0.4f * pulse,
              0.8f + 0.2f * pulse,
              0.2f,
              0.95f * alpha);

    glBegin(GL_LINE_LOOP);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    int cx = g.xres / 2;

    Rect r;
    r.center = 1;
    r.left = cx;

    r.bot = (int)(py + ph - 44);
    ggprint16(&r, 0, 0x0044ff66, "LEVEL CLEAR");

    r.bot -= 30;
    r.left = cx;
    r.center = 1;
    ggprint12(&r, 0, 0x00aaffcc, "%s", ld.title);

    r.bot -= 24;
    r.left = cx;
    r.center = 1;
    ggprint12(&r, 0, 0x00888888, "%s", ld.subtitle);

    int sl = 3 - (int)t;

    if (sl < 1)
        sl = 1;

    r.bot -= 32;
    r.left = cx;
    r.center = 1;
    ggprint12(&r, 0, 0x00666666, "Powerup selection in %d...", sl);
}

// game won screen
void levelsRenderGameWon()
{
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.72f);

    glBegin(GL_QUADS);
    glVertex2f(0,      0);
    glVertex2f(g.xres, 0);
    glVertex2f(g.xres, g.yres);
    glVertex2f(0,      g.yres);
    glEnd();

    float pw = 420;
    float ph = 240;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glColor4f(0.02f, 0.10f, 0.04f, 0.94f);

    glBegin(GL_QUADS);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 3.5f);

    glLineWidth(2.5f);
    glColor4f(0.1f + 0.4f * pulse,
              0.8f + 0.2f * pulse,
              0.2f,
              0.95f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    Rect r;
    r.center = 1;
    r.left = g.xres / 2;

    r.bot = (int)(py + ph * 0.5f - 8);
    ggprint16(&r, 0, 0x0044ff66, "Galaxy Overdrive Complete");
}

// death screen
#define DEBRIS_COUNT 24

struct DebrisParticle {
    float x, y, vx, vy, angle, spin, life, maxLife, r, g, b;
    bool active;
};

// debris for death animation
static DebrisParticle s_debris[DEBRIS_COUNT];
static bool s_debrisInited = false;
static float s_deathTimer = 0;
static float s_flashAlpha = 1.0f;

void levelsInitDeath(float shipX, float shipY)
{
    s_deathTimer = 0;
    s_flashAlpha = 1.0f;
    s_debrisInited = true;

    for (int i = 0; i < DEBRIS_COUNT; i++) {
        float angle = ((float)i / DEBRIS_COUNT) * 2 * 3.14159f
                    + ((float)(rand() % 100) / 100.f) * 0.8f;
        float speed = 40 + (float)(rand() % 120);

        s_debris[i].x = shipX;
        s_debris[i].y = shipY;
        s_debris[i].vx = cosf(angle) * speed;
        s_debris[i].vy = sinf(angle) * speed;
        s_debris[i].angle = (float)(rand() % 360);
        s_debris[i].spin = ((float)(rand() % 400) - 200) * 0.01f;
        s_debris[i].maxLife = 1.2f + (float)(rand() % 80) / 100.f;
        s_debris[i].life = s_debris[i].maxLife;
        s_debris[i].active = true;

        float heat = (float)(rand() % 100) / 100.f;
        s_debris[i].r = 1.0f;
        s_debris[i].g = heat * 0.6f;
        s_debris[i].b = 0;
    }
}

void levelsUpdateDeath(float dt)
{
    s_deathTimer += dt;
    s_flashAlpha -= dt * 3;

    if (s_flashAlpha < 0)
        s_flashAlpha = 0;

    for (int i = 0; i < DEBRIS_COUNT; i++) {
        if (!s_debris[i].active)
            continue;

        s_debris[i].x += s_debris[i].vx * dt;
        s_debris[i].y += s_debris[i].vy * dt;
        s_debris[i].angle += s_debris[i].spin * dt * 60;
        s_debris[i].life -= dt;
        s_debris[i].vx *= 0.98f;
        s_debris[i].vy *= 0.98f;

        if (s_debris[i].life <= 0)
            s_debris[i].active = false;
    }
}

void levelsRenderDeath()
{
    // brief white flash
    if (s_flashAlpha > 0) {
        glDisable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, s_flashAlpha);

        glBegin(GL_QUADS);
        glVertex2f(0,      0);
        glVertex2f(g.xres, 0);
        glVertex2f(g.xres, g.yres);
        glVertex2f(0,      g.yres);
        glEnd();

        glEnable(GL_TEXTURE_2D);
    }

    // draw debris
    glDisable(GL_TEXTURE_2D);

    for (int i = 0; i < DEBRIS_COUNT; i++) {
        if (!s_debris[i].active)
            continue;

        float frac = s_debris[i].life / s_debris[i].maxLife;
        float alpha = frac * frac;
        float sz = 6 * frac + 2;

        glPushMatrix();
        glTranslatef(s_debris[i].x, s_debris[i].y, 0);
        glRotatef(s_debris[i].angle, 0, 0, 1);

        glColor4f(s_debris[i].r, s_debris[i].g, s_debris[i].b, alpha);

        glBegin(GL_TRIANGLES);
        glVertex2f(0,   sz * 2);
        glVertex2f(-sz, -sz);
        glVertex2f(sz,  -sz);
        glEnd();

        glColor4f(1, 0.9f, 0.4f, alpha * 0.7f);

        glBegin(GL_TRIANGLES);
        glVertex2f(0,          sz);
        glVertex2f(-sz * 0.5f, -sz * 0.4f);
        glVertex2f(sz * 0.5f,  -sz * 0.4f);
        glEnd();

        glPopMatrix();
    }

    glEnable(GL_TEXTURE_2D);

    // delay death panel
    if (s_deathTimer < 1.5f)
        return;

    float pa = (s_deathTimer - 1.5f) / 0.6f;

    if (pa > 1)
        pa = 1;

    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.72f * pa);

    glBegin(GL_QUADS);
    glVertex2f(0,      0);
    glVertex2f(g.xres, 0);
    glVertex2f(g.xres, g.yres);
    glVertex2f(0,      g.yres);
    glEnd();

    float pw = 380;
    float ph = 220;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glColor4f(0.12f, 0.01f, 0.01f, 0.95f * pa);

    glBegin(GL_QUADS);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 2.8f);

    glLineWidth(2);
    glColor4f(0.8f + 0.2f * pulse, 0.05f, 0.05f, 0.95f * pa);

    glBegin(GL_LINE_LOOP);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    int cx = g.xres / 2;

    Rect r;
    r.center = 1;
    r.left = cx;

    r.bot = (int)(py + ph - 44);
    ggprint16(&r, 0, 0x00ff2222, "YOU DIED");

    r.bot -= 36;
    ggprint12(&r, 0, 0x00cccccc, "R  -  Restart Level");

    r.bot -= 22;
    ggprint12(&r, 0, 0x00cccccc, "M  -  Main Menu");
}

// level select overlay
void levelsRenderSelectMenu()
{
    if (!lv_selectOpen) {
        Rect r;
        r.bot = 60;
        r.left = g.xres / 2;
        r.center = 1;

        ggprint12(&r, 0, 0x00aaaaaa, "[ L ] Level Select");
        return;
    }

    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0.08f, 0.78f);

    glBegin(GL_QUADS);
    glVertex2f(0,      0);
    glVertex2f(g.xres, 0);
    glVertex2f(g.xres, g.yres);
    glVertex2f(0,      g.yres);
    glEnd();

    float pw = 420;
    float ph = 340;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glColor4f(0.04f, 0.08f, 0.18f, 0.95f);

    glBegin(GL_QUADS);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 2.5f);

    glLineWidth(2);
    glColor4f(0.2f + 0.4f * pulse,
              0.5f + 0.3f * pulse,
              1,
              0.9f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    Rect r;
    r.bot = (int)(py + ph - 28);
    r.left = (int)(px + pw * 0.5f);
    r.center = 1;

    ggprint16(&r, 0, 0x00ffffff, "SELECT LEVEL");

    float rowH = 42;
    float startY = py + ph - 70;

    for (int i = 0; i < TOTAL_LEVELS; i++) {
        float ry = startY - i * rowH;
        bool sel = (i == lv_selectCursor);
        bool cur = (i == lv_current);

        if (sel) {
            glDisable(GL_TEXTURE_2D);
            glColor4f(0.10f, 0.28f, 0.62f, 0.75f);

            glBegin(GL_QUADS);
            glVertex2f(px + 10,      ry - 4);
            glVertex2f(px + pw - 10, ry - 4);
            glVertex2f(px + pw - 10, ry + 24);
            glVertex2f(px + 10,      ry + 24);
            glEnd();

            glEnable(GL_TEXTURE_2D);
        }

        unsigned int col = sel ? 0x0055ccff : 0x00445577;

        if (cur)
            col = 0x0000ff99;

        r.bot = (int)ry;
        r.left = (int)(px + 28);
        r.center = 0;

        ggprint12(&r, 0, col, "%s  %-9s  %s%s",
                  sel ? ">" : "  ",
                  LEVEL_DEFS[i].title,
                  LEVEL_DEFS[i].subtitle,
                  cur ? " [ACTIVE]" : "");
    }

    r.bot = (int)(py + 14);
    r.left = (int)(px + pw * 0.5f);
    r.center = 1;

    ggprint10(&r, 0, 0x00667788,
              "UP/DOWN  navigate     ENTER  load     L  close");
}

// handle level, death, and win screen keys
bool levelsHandleKey(int key)
{
    // stay on win screen
    if (g.state == STATE_GAME_WON) {
        return true;
    }

    // death screen keys
    if (g.state == STATE_DEAD && s_deathTimer >= 1.5f) {
        // restart current level
        if (key == XK_r) {
            lv_loadLevel(lv_current);

            g.playerHP = 10;
            g.displayHP = 10;
            g.state = STATE_LEVEL_INTRO;
            g.levelIntroTimer = 0;

            s_debrisInited = false;
            init_enemies();

            return true;
        }

        // return to main menu
        if (key == XK_m) {
            g.state = STATE_TITLE;
            g.playerHP = 10;
            g.displayHP = 10;
            lv_current = 0;
            s_debrisInited = false;
            g.title.timer = 0;
            g.startBtn.visible = false;

            obstaclesRemoveAllAsteroids();
            obstaclesRemoveAllMines();

            for (int i = 0; i < OBS_BARRIER_COUNT; i++)
                obs_barriers[i].active = false;

            obs_gate.active = false;

            obstaclesSpawnTitleAsteroids();
            init_enemies();

            return true;
        }

        return true;
    }

    // level-select menu
    if (g.paused && lv_selectOpen) {
        if (key == XK_Up || key == XK_w) {
            lv_selectCursor = (lv_selectCursor - 1 + TOTAL_LEVELS) % TOTAL_LEVELS;
            return true;
        }

        if (key == XK_Down || key == XK_s) {
            lv_selectCursor = (lv_selectCursor + 1) % TOTAL_LEVELS;
            return true;
        }

        if (key == XK_Return || key == XK_KP_Enter) {
            lv_loadLevel(lv_selectCursor);

            lv_selectOpen = false;
            g.paused = false;
            g.state = STATE_PLAYING;
            g.playerHP = 10;
            g.displayHP = 10;

            init_enemies();

            return true;
        }

        if (key == XK_l || key == XK_L) {
            lv_selectOpen = false;
            return true;
        }

        return true;
    }

    if (g.paused && (key == XK_l || key == XK_L)) {
        lv_selectOpen = true;
        lv_selectCursor = lv_current;
        return true;
    }

    if (key >= XK_1 && key <= XK_6) {
        int lv = key - XK_1;

        lv_loadLevel(lv);

        g.paused = false;
        g.state = STATE_PLAYING;
        g.playerHP = 10;
        g.displayHP = 10;

        init_enemies();

        return true;
    }

    if (key == XK_n) {
        int next = (lv_current + 1) % TOTAL_LEVELS;

        lv_loadLevel(next);

        g.playerHP = 10;
        g.displayHP = 10;
        g.state = STATE_PLAYING;
        g.paused = false;

        init_enemies();

        return true;
    }

    return false;
}

#endif // levels_h
