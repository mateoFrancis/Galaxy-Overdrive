#ifndef LEVELS_H
#define LEVELS_H



#include <math.h>
#include <stdio.h>

#define TOTAL_LEVELS 6

struct LevelDesc {
    const char *title;          // shown on intro card
    const char *subtitle;       // flavour line
    int   asteroidCount;        // how many to place
    int   mineCount;
    bool  barriersEnabled;
    bool  turretEnabled;
    bool  warpGateEnabled;
    int   enemyType0Count;      // type-0 enemies pre-spawned
    int   enemyType1Count;      // type-1 enemies pre-spawned
    float enemySpawnInterval;   // ongoing spawn rate (seconds)
    float asteroidSpeedMult;    // multiplier on base asteroid speed
    float asteroidSpinMult;
};

static const LevelDesc LEVEL_DEFS[TOTAL_LEVELS] = {
    // Level 1: Asteroid Field 
    {
        "LEVEL 1",
        "Asteroid Field Watch your hull",
        5, 3,
        false, false, false,
        0, 0, 9999.f,   // no enemies
        1.0f, 1.0f
    },
    //  Level 2: 
    {
        "LEVEL 2",
        "Minefield They're everywhere",
        8, 6,
        false, false, false,
        2, 0, 6.0f,     // a couple of basic enemies
        1.25f, 1.4f
    },
    // level 3:
    {
        "LEVEL 3",
        "Fortified Zone The turret never sleeps",
        6, 4,
        true, true, false,
        3, 1, 4.5f,
        1.4f, 1.6f
    },
    // Level 4: Warp Chaos 
    {
        "LEVEL 4",
        "Warp Chaos  Nothing stays where you left it",
        8, 5,
        true, true, true,
        2, 2, 3.5f,
        1.6f, 1.9f
    },
    // Level 5: Enemy Swarm 
    {
        "LEVEL 5",
        "Enemy Swarm No time to breathe",
        4, 3,
        false, false, true,
        4, 4, 2.0f,
        1.5f, 1.2f
    },
    // Level 6: The Gauntlet 
    {
        "LEVEL 6",
        "The Gauntlet Good luck, pilot",
        8, 6,
        true, true, true,
        4, 4, 1.5f,
        2.0f, 2.2f
    }
};

// ============================================================
//  Level state
// ============================================================
static int   lv_current      = 0;   // 0-indexed
static bool  lv_complete     = false;
static float lv_completeTimer = 0.f;

// pause-menu level select
bool  lv_selectOpen   = false;
static int   lv_selectCursor = 0;

// score tracking per level
static int   lv_scoreAtStart = 0;

// asteroid layout tables per level (x,y,size,speed,spin)
struct AstSpec { float x, y; int size; float spd, spin; };

// obstaclesAddAsteroid clamps automatically
static const AstSpec AST_L1[5] = {
    { 120, 380, ASTEROID_LARGE,  35.f, -0.40f },
    { 500, 200, ASTEROID_MEDIUM, 45.f,  0.60f },
    { 300, 100, ASTEROID_SMALL,  55.f, -0.90f },
    { 550, 380, ASTEROID_SMALL,  50.f,  0.70f },
    { 200, 250, ASTEROID_MEDIUM, 40.f, -0.50f },
};

static const AstSpec AST_L2[8] = {
    {  80, 400, ASTEROID_LARGE,  40.f, -0.50f },
    { 560, 380, ASTEROID_LARGE,  42.f,  0.55f },
    { 320, 430, ASTEROID_MEDIUM, 52.f, -0.80f },
    { 160, 160, ASTEROID_MEDIUM, 48.f,  0.75f },
    { 480, 120, ASTEROID_SMALL,  65.f, -1.10f },
    {  60, 260, ASTEROID_SMALL,  60.f,  0.95f },
    { 400, 300, ASTEROID_SMALL,  58.f, -0.85f },
    { 240,  80, ASTEROID_MEDIUM, 50.f,  0.65f },
};

static const AstSpec AST_L3[6] = {
    {  90, 420, ASTEROID_LARGE,  44.f, -0.55f },
    { 540, 120, ASTEROID_LARGE,  46.f,  0.60f },
    { 310, 350, ASTEROID_MEDIUM, 55.f, -0.90f },
    { 150, 200, ASTEROID_SMALL,  68.f,  1.00f },
    { 460, 300, ASTEROID_SMALL,  63.f, -1.05f },
    { 280, 130, ASTEROID_MEDIUM, 52.f,  0.70f },
};

static const AstSpec AST_L4[8] = {
    {  70, 370, ASTEROID_LARGE,  50.f, -0.70f },
    { 570, 100, ASTEROID_LARGE,  54.f,  0.80f },
    { 320, 440, ASTEROID_MEDIUM, 62.f, -1.05f },
    { 180, 280, ASTEROID_SMALL,  75.f,  1.20f },
    { 440, 200, ASTEROID_SMALL,  72.f, -1.15f },
    { 100, 130, ASTEROID_MEDIUM, 58.f,  0.85f },
    { 500, 360, ASTEROID_MEDIUM, 60.f, -0.90f },
    { 260,  90, ASTEROID_SMALL,  80.f,  1.30f },
};

static const AstSpec AST_L5[4] = {
    { 100, 400, ASTEROID_MEDIUM, 55.f, -0.80f },
    { 540, 350, ASTEROID_MEDIUM, 58.f,  0.75f },
    { 320, 120, ASTEROID_SMALL,  70.f, -1.10f },
    { 200, 200, ASTEROID_SMALL,  65.f,  0.90f },
};

static const AstSpec AST_L6[8] = {
    {  60, 420, ASTEROID_LARGE,  60.f, -0.90f },
    { 580, 400, ASTEROID_LARGE,  64.f,  1.00f },
    { 300, 460, ASTEROID_LARGE,  58.f, -0.80f },
    { 160, 160, ASTEROID_MEDIUM, 72.f,  1.10f },
    { 480, 160, ASTEROID_MEDIUM, 70.f, -1.05f },
    {  90, 290, ASTEROID_SMALL,  88.f,  1.40f },
    { 550, 260, ASTEROID_SMALL,  85.f, -1.35f },
    { 330, 230, ASTEROID_SMALL,  90.f,  1.50f },
};

static const AstSpec *AST_TABLES[TOTAL_LEVELS] = {
    AST_L1, AST_L2, AST_L3, AST_L4, AST_L5, AST_L6
};
static const int AST_TABLE_COUNT[TOTAL_LEVELS] = { 5, 8, 6, 8, 4, 8 };

//mine positions per level 

struct MineSpec { float x, y, r; };

static const MineSpec MINE_L1[3] = {
    { 440, 300, 16.f }, { 200, 340, 14.f }, { 350, 420, 15.f }
};
static const MineSpec MINE_L2[6] = {
    {  80, 200, 16.f }, { 200, 350, 15.f }, { 380, 410, 14.f },
    { 520, 280, 16.f }, { 310, 160, 15.f }, { 140, 430, 14.f }
};
static const MineSpec MINE_L3[4] = {
    { 100, 120, 16.f }, { 540, 440, 16.f },
    { 320, 380, 15.f }, { 430, 150, 14.f }
};
static const MineSpec MINE_L4[5] = {
    {  70, 300, 16.f }, { 570, 180, 16.f }, { 240, 430, 15.f },
    { 420, 380, 15.f }, { 310,  90, 14.f }
};
static const MineSpec MINE_L5[3] = {
    { 160, 320, 16.f }, { 480, 300, 16.f }, { 300, 200, 15.f }
};
static const MineSpec MINE_L6[6] = {
    {  90, 200, 16.f }, { 540, 350, 16.f }, { 200, 420, 15.f },
    { 440, 130, 15.f }, { 310, 310, 14.f }, { 160,  80, 14.f }
};

static const MineSpec *MINE_TABLES[TOTAL_LEVELS] = {
    MINE_L1, MINE_L2, MINE_L3, MINE_L4, MINE_L5, MINE_L6
};
static const int MINE_TABLE_COUNT[TOTAL_LEVELS] = { 3, 6, 4, 5, 3, 6 };

// barrier layouts (used L3, L4, L6) 
// spawn() signature: (x, y, width, height, angle_radians)
struct BarSpec { float x, y, w, h, ang; };

// L3: two vertical walls forming a corridor
static const BarSpec BAR_L3[3] = {
    { 210.f, 240.f, 18.f, 200.f, 0.f   },
    { 430.f, 240.f, 18.f, 200.f, 0.f   },
    { 320.f, 100.f, 220.f, 18.f, 0.f   }   // roof of corridor
};
// L4: diagonal blockers
static const BarSpec BAR_L4[3] = {
    { 180.f, 300.f, 20.f, 160.f, 0.4f  },
    { 460.f, 300.f, 20.f, 160.f, -0.4f },
    { 320.f, 420.f, 240.f, 18.f, 0.f   }
};
// L6: full cross
static const BarSpec BAR_L6[3] = {
    { 320.f, 240.f, 18.f, 260.f, 0.f   },   // vertical spine
    { 180.f, 320.f, 18.f, 160.f, 0.52f },
    { 460.f, 160.f, 18.f, 160.f,-0.52f }
};

// ============================================================
//  Helper: clear the board, then load a level
// ============================================================
static void lv_loadLevel(int idx)
{
    if (idx < 0 || idx >= TOTAL_LEVELS) return;

    lv_current      = idx;
    lv_complete     = false;
    lv_completeTimer = 0.f;
    lv_scoreAtStart  = g.score;

    const LevelDesc &ld = LEVEL_DEFS[idx];

    obstaclesRemoveAllAsteroids();
    obstaclesRemoveAllMines();
    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].active = false;
    obs_turret.active  = false;
    obs_gate.active    = false;

    //asteroids
    const AstSpec *atab = AST_TABLES[idx];
    int            acnt = AST_TABLE_COUNT[idx];
    float sm = ld.asteroidSpeedMult;
    float rm = ld.asteroidSpinMult;
    for (int i = 0; i < acnt && i < ld.asteroidCount; i++)
        obstaclesAddAsteroid(atab[i].x, atab[i].y,
                             atab[i].size,
                             atab[i].spd  * sm,
                             atab[i].spin * rm);

    //mines
    const MineSpec *mtab = MINE_TABLES[idx];
    int             mcnt = MINE_TABLE_COUNT[idx];
    for (int i = 0; i < mcnt && i < ld.mineCount; i++)
        obstaclesAddMine(mtab[i].x, mtab[i].y, mtab[i].r);

    //mines
    if (ld.barriersEnabled) {
        const BarSpec *btab = nullptr;
        if (idx == 2) btab = BAR_L3;
        else if (idx == 3) btab = BAR_L4;
        else if (idx == 5) btab = BAR_L6;

        if (btab) {
            for (int i = 0; i < OBS_BARRIER_COUNT; i++)
                obs_barriers[i].spawn(btab[i].x, btab[i].y,
                                      btab[i].w,  btab[i].h, btab[i].ang);
        }
    }

    //turret
    if (ld.turretEnabled) {
        // Place turret at centre-bottom for L3, top-right for L4/L6
        float tx = (idx == 2) ? 320.f : (idx == 3) ? 500.f : 420.f;
        float ty = (idx == 2) ?  60.f : (idx == 3) ? 400.f : 400.f;
        float interval = (idx >= 5) ? 1.8f : 2.5f;
        obs_turret.spawn(tx, ty, 34.f, interval);
    }

    //warp gate
    if (ld.warpGateEnabled)
        obs_gate.spawn(g_xres * 0.5f, g_yres * 0.5f, 45.f);

    for (int i = 0; i < ld.enemyType0Count; i++) {
        float ex = 40.f + (float)(rand() % (g_xres - 80));
        float ey = 40.f + (float)(rand() % (g_yres - 80));
        spawn_enemy(ex, ey, g_xres, g_yres);
    }
    // type-1 spawns same way (spawn_enemy picks type internally;
    // call a second batch so the pool gets varied types)
    for (int i = 0; i < ld.enemyType1Count; i++) {
        float ex = 40.f + (float)(rand() % (g_xres - 80));
        float ey = 40.f + (float)(rand() % (g_yres - 80));
        spawn_enemy(ex, ey, g_xres, g_yres);
    }

    //spawn rate 
    g.spawnInterval = ld.enemySpawnInterval;
    g.spawnTimer    = 0.f;
}

// ============================================================
//  Init called once after init_enemies() / obstaclesInit()
// ============================================================
void levelsInit()
{
    lv_current      = 0;
    lv_selectOpen   = false;
    lv_selectCursor = 0;
}

// ============================================================
//  Update call from physics() while STATE_PLAYING
// ============================================================
void levelsUpdate(float /*dt*/)
{
    // nothing automatic right now; level advance is manual
    // (shoot the "NEXT LEVEL" button, or press N)
    // You can add win conditions here, e.g. all asteroids dead.
}

// ============================================================
//  HUD call from render() inside STATE_PLAYING
// ============================================================
void levelsRenderHUD()
{
    const LevelDesc &ld = LEVEL_DEFS[lv_current];

    // level number chip — top centre
    Rect r;
    r.bot    = g.yres - 18;
    r.left   = g.xres / 2;
    r.center = 1;
    ggprint12(&r, 0, 0x0088ffff, "%s", ld.title);
}

// ============================================================
//  Pause-menu level select overlay
//  Call from renderPause() AFTER your existing pause drawing.
// ============================================================
void levelsRenderSelectMenu()
{
    if (!lv_selectOpen) {
        // show hint at bottom of pause screen
        Rect r;
        r.bot    = 60;
        r.left   = g.xres / 2;
        r.center = 1;
        ggprint12(&r, 0, 0x00aaaaaa, "[ L ] Level Select");
        return;
    }

    //overlay
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.f, 0.f, 0.08f, 0.78f);
    glBegin(GL_QUADS);
        glVertex2f(0,       0);
        glVertex2f(g.xres,  0);
        glVertex2f(g.xres,  g.yres);
        glVertex2f(0,       g.yres);
    glEnd();

    //panel box
    float pw = 420.f, ph = 340.f;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glColor4f(0.04f, 0.08f, 0.18f, 0.95f);
    glBegin(GL_QUADS);
        glVertex2f(px,    py);
        glVertex2f(px+pw, py);
        glVertex2f(px+pw, py+ph);
        glVertex2f(px,    py+ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 2.5f);
    glLineWidth(2.f);
    glColor4f(0.2f + 0.4f*pulse, 0.5f + 0.3f*pulse, 1.f, 0.9f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(px,    py);
        glVertex2f(px+pw, py);
        glVertex2f(px+pw, py+ph);
        glVertex2f(px,    py+ph);
    glEnd();
    glEnable(GL_TEXTURE_2D);

    //header
    Rect r;
    r.bot    = (int)(py + ph - 28);
    r.left   = (int)(px + pw * 0.5f);
    r.center = 1;
    ggprint16(&r, 0, 0x00ffffff, "SELECT LEVEL");

    //lvl rows
    float rowH   = 42.f;
    float startY = py + ph - 70.f;

    for (int i = 0; i < TOTAL_LEVELS; i++) {
        float ry = startY - i * rowH;
        bool  sel = (i == lv_selectCursor);
        bool  cur = (i == lv_current);

        // highlight bar for selected cursor row
        if (sel) {
            glDisable(GL_TEXTURE_2D);
            glColor4f(0.10f, 0.28f, 0.62f, 0.75f);
            glBegin(GL_QUADS);
                glVertex2f(px+10,    ry-4);
                glVertex2f(px+pw-10, ry-4);
                glVertex2f(px+pw-10, ry+24);
                glVertex2f(px+10,    ry+24);
            glEnd();
            glEnable(GL_TEXTURE_2D);
        }

        // arrow indicator
        unsigned int col = sel ? 0x0055ccff : 0x00445577;
        if (cur) col = 0x0000ff99;   // green = currently loaded

        r.bot    = (int)ry;
        r.left   = (int)(px + 28);
        r.center = 0;

        const char *arrow = sel ? ">" : " ";
        const char *tag   = cur ? " [ACTIVE]" : "";
        ggprint12(&r, 0, col, "%s  %-9s  %s%s",
                  arrow,
                  LEVEL_DEFS[i].title,
                  LEVEL_DEFS[i].subtitle,
                  tag);
    }

    r.bot    = (int)(py + 14);
    r.left   = (int)(px + pw * 0.5f);
    r.center = 1;
    ggprint10(&r, 0, 0x00667788, "UP/DOWN  navigate     ENTER  load     L  close");
}

// ============================================================
//  Key handler call from check_keys() KeyPress block
//  Returns true if it consumed the key (so you skip default).
// ============================================================
bool levelsHandleKey(int key)
{
    //lvl select
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
            g.paused      = false;
            g.state       = STATE_PLAYING;
            g.playerHP    = 10;
            g.displayHP   = 10.f;
            g.score       = 0;
            init_enemies();
            return true;
        }
        if (key == XK_l || key == XK_L) {
            lv_selectOpen = false;
            return true;
        }
        return true;   // eat all keys while menu is open
    }

    //toggle menu
    if (g.paused && (key == XK_l || key == XK_L)) {
        lv_selectOpen   = true;
        lv_selectCursor = lv_current;
        return true;
    }
    
    if (key >= XK_1 && key <= XK_6) {
        int lvIdx = key - XK_1;    
        lv_loadLevel(lvIdx);
        g.paused   = false;
        g.state    = STATE_PLAYING;
        g.playerHP = 10;
        g.displayHP = 10.f;
        g.score    = 0;
        init_enemies();
        return true;
    }

    //level advance
    if (key == XK_n) {
        int next = (lv_current + 1) % TOTAL_LEVELS;
        lv_loadLevel(next);
        g.playerHP  = 10;
        g.displayHP = 10.f;
        g.score     = 0;
        g.state     = STATE_PLAYING;
        g.paused    = false;
        init_enemies();
        return true;
    }

    return false;
}

#endif // LEVELS_H

