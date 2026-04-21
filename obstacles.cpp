#ifndef OBSTACLES_H
#define OBSTACLES_H

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "fonts.h"
#include "enemy.h"

#ifndef OBSTACLES_INCLUDED_BY_BACKGROUND
extern int   g_xres;
extern int   g_yres;
extern float g_time;
#endif
#define OBSTACLES_INCLUDED_BY_BACKGROUND

#ifndef OBS_PI
#define OBS_PI 3.14159265358979f
#endif

#define OBS_ASTEROID_COUNT    8
#define OBS_BARRIER_COUNT     3
#define OBS_MINE_COUNT        6
#define OBS_TURRET_BULLET_MAX 4

// asteroid size tiers
#define ASTEROID_SMALL   0
#define ASTEROID_MEDIUM  1
#define ASTEROID_LARGE   2

static GLuint obs_asteroidTex = 0;
static GLuint obs_portalTex   = 0;

// ============================================================
//  Primitive helpers (kept for mines, barriers, turret)
// ============================================================
static inline void obs_circle(float x, float y, float r, int s,
                               float R, float G, float B, float A = 1.f)
{
    glColor4f(R, G, B, A);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= s; i++) {
        float a = 2*OBS_PI*i/s;
        glVertex2f(x + cosf(a)*r, y + sinf(a)*r);
    }
    glEnd();
}

static inline void obs_ring(float x, float y, float r, int s,
                             float R, float G, float B, float A = 1.f,
                             float lw = 1.5f)
{
    glLineWidth(lw); glColor4f(R, G, B, A);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < s; i++) {
        float a = 2*OBS_PI*i/s;
        glVertex2f(x + cosf(a)*r, y + sinf(a)*r);
    }
    glEnd();
}

static inline void obs_quad(float x, float y, float w, float h,
                             float R, float G, float B, float A = 1.f)
{
    glColor4f(R, G, B, A);
    glBegin(GL_QUADS);
    glVertex2f(x,   y);   glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x,   y+h);
    glEnd();
}

static inline void obs_drawTank(float x, float y, float sc, float ang,
                                 float R, float G, float B, float A = 1.f)
{
    glPushMatrix();
    glTranslatef(x, y, 0); glRotatef(ang * 180.f / OBS_PI, 0, 0, 1);
    float bw = sc * .28f, bh = sc * .85f;
    glColor4f(R*.6f, G*.6f, B*.6f, A);
    glBegin(GL_QUADS);
    glVertex2f(-bw, 0); glVertex2f(bw, 0);
    glVertex2f( bw, bh); glVertex2f(-bw, bh);
    glEnd();
    glLineWidth(1.4f); glColor4f(.8f, .85f, .9f, A * .7f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-bw, 0); glVertex2f(bw, 0);
    glVertex2f( bw, bh); glVertex2f(-bw, bh);
    glEnd();
    glPopMatrix();
    obs_circle(x, y, sc * .52f, 20, R, G, B, A);
    obs_ring  (x, y, sc * .52f, 20, .85f, .9f, 1.f, A * .8f, 2.f);
}

// ============================================================
//  ASTEROID - textured, sized, explodes
// ============================================================
struct Asteroid {
    float x, y;
    float vx, vy;
    float angle, spin;
    float radius;       // hitbox/visual radius
    int   size;         // 0=small, 1=medium, 2=large
    int   hp;
    int   damage;       // damage to player on touch
    bool  active;

    // explosion state
    bool  exploding;
    int   explodeFrame;
    float explodeTimer;

    void spawn(float px, float py, int sizeTier, float spd, float sp) {
        x = px; y = py;
        size = sizeTier;

        if (sizeTier == ASTEROID_SMALL) {
            radius = 16.0f; hp = 1; damage = 1;
        } else if (sizeTier == ASTEROID_MEDIUM) {
            radius = 28.0f; hp = 2; damage = 2;
        } else {
            radius = 40.0f; hp = 3; damage = 3;
        }

        float ang = (float)(rand() % 628) / 100.f;
        vx = cosf(ang) * spd; vy = sinf(ang) * spd;
        spin = sp; angle = 0.f;
        active = true;
        exploding = false;
        explodeFrame = 0;
        explodeTimer = 0.0f;
    }

    bool checkHit(float bx, float by, float br) const {
        if (!active || exploding) return false;
        float dx = bx - x, dy = by - y;
        return (dx*dx + dy*dy) < (radius + br) * (radius + br);
    }

    void startExplode() {
        exploding = true;
        explodeFrame = 0;
        explodeTimer = 0.0f;
        vx = 0; vy = 0;
        spin = 0;
    }

    // 1 dmg to asteroid / returns true if asteroid just started exploding
    bool damageBy(int dmg = 1) {
        if (!active || exploding) return false;
        hp -= dmg;
        if (hp <= 0) {
            startExplode();
            return true;
        }
        return false;
    }

    void update(float dt) {
        if (!active) return;

        if (exploding) {
            explodeTimer += dt;
            if (explodeTimer >= get_explosion_frame_time()) {
                explodeTimer = 0.0f;
                explodeFrame++;
                if (explodeFrame >= get_explosion_frames())
                    active = false;
            }
            return;
        }

        x += vx * dt; y += vy * dt;
        angle += spin * dt;
        if (x < -60)         x = g_xres + 60;
        if (x > g_xres + 60) x = -60;
        if (y < -60)         y = g_yres + 60;
        if (y > g_yres + 60) y = -60;
    }

    void draw() {
        if (!active) return;

        if (exploding) {
            float frameW = 1.0f / (float)get_explosion_frames();
            float tx0 = frameW * (float)explodeFrame;
            float tx1 = tx0 + frameW;
            glEnable(GL_TEXTURE_2D);
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, get_explosion_texture());
            glPushMatrix();
            glTranslatef(x, y, 0);
            float r = radius;
            glBegin(GL_QUADS);
                glTexCoord2f(tx0, 1.0f); glVertex2f(-r, -r);
                glTexCoord2f(tx0, 0.0f); glVertex2f(-r,  r);
                glTexCoord2f(tx1, 0.0f); glVertex2f( r,  r);
                glTexCoord2f(tx1, 1.0f); glVertex2f( r, -r);
            glEnd();
            glPopMatrix();
            return;
        }

        // draw textured asteroid
        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, obs_asteroidTex);
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);
        float r = radius;
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-r, -r);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-r,  r);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( r,  r);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( r, -r);
        glEnd();
        glPopMatrix();
    }
};

// ============================================================
//  BARRIER (unchanged - kept for later levels)
// ============================================================
struct Barrier {
    float x, y, w, h, angle;
    int   hp, maxHp;
    float flashTimer;
    bool  active;

    void spawn(float px, float py, float width, float height, float ang) {
        x = px; y = py; w = width; h = height; angle = ang;
        hp = maxHp = 5; flashTimer = 0.f; active = true;
    }

    void hit(int dmg = 1) {
        if (!active) return;
        hp -= dmg; flashTimer = 0.12f;
        if (hp <= 0) active = false;
    }

    bool containsPoint(float px, float py, float pr = 0.f) const {
        if (!active) return false;
        return px + pr > x - w*.5f && px - pr < x + w*.5f &&
               py + pr > y - h*.5f && py - pr < y + h*.5f;
    }

    void update(float dt) { if (flashTimer > 0) flashTimer -= dt; }

    void draw() {
        if (!active) return;
        float hf    = (float)hp / maxHp;
        float flash = (flashTimer > 0) ? 1.f : 0.f;

        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);

        float R = 0.22f + (1.f - hf)*0.55f + flash*0.4f;
        float G = 0.38f * hf + flash*0.4f;
        float B = 0.62f * hf + flash*0.4f;
        glColor4f(R, G, B, 1.f);
        glBegin(GL_QUADS);
        glVertex2f(-w*.5f,-h*.5f); glVertex2f( w*.5f,-h*.5f);
        glVertex2f( w*.5f, h*.5f); glVertex2f(-w*.5f, h*.5f);
        glEnd();

        glLineWidth(2.f); glColor4f(0.55f, 0.75f, 1.f, 0.85f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-w*.5f,-h*.5f); glVertex2f( w*.5f,-h*.5f);
        glVertex2f( w*.5f, h*.5f); glVertex2f(-w*.5f, h*.5f);
        glEnd();
        glPopMatrix();

        float barW = w * 0.85f, barX = x - barW*0.5f, barY = y - h*0.5f - 10.f;
        obs_quad(barX, barY, barW, 5.f, 0.15f,0.15f,0.15f,0.8f);
        obs_quad(barX, barY, barW*hf, 5.f,
                 0.2f+0.8f*(1.f-hf), 0.8f*hf, 0.2f, 0.9f);
    }
};

// ============================================================
//  MINE (unchanged - still in level 1)
// ============================================================
struct Mine {
    float x, y, vx, vy, angle, r;
    bool  active;

    void spawn(float px, float py, float radius) {
        x = px; y = py; r = radius;
        float ang = (float)(rand()%628)/100.f;
        vx = cosf(ang)*18.f; vy = sinf(ang)*18.f;
        angle = 0.f; active = true;
    }

    bool checkHit(float bx, float by, float br) const {
        if (!active) return false;
        float dx = bx-x, dy = by-y;
        float cr = r*1.75f + br;
        return (dx*dx+dy*dy) < cr*cr;
    }

    void explode() { active = false; }

    void update(float dt) {
        if (!active) return;
        x += vx*dt; y += vy*dt; angle += 0.8f*dt;
        if (x < -r*2)         x = g_xres + r*2;
        if (x > g_xres + r*2) x = -r*2;
        if (y < -r*2)         y = g_yres + r*2;
        if (y > g_yres + r*2) y = -r*2;
    }

    void draw() {
        if (!active) return;
        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        glTranslatef(x, y, 0); glRotatef(angle*180.f/OBS_PI, 0,0,1);

        int sides = 5;
        glColor4f(0.55f,0.18f,0.22f,1.f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0,0);
        for (int i = 0; i <= sides; i++) {
            float a = 2*OBS_PI*i/sides - OBS_PI/2.f;
            glVertex2f(cosf(a)*r, sinf(a)*r);
        }
        glEnd();

        glLineWidth(2.f); glColor4f(1.f,0.35f,0.4f,0.9f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < sides; i++) {
            float a = 2*OBS_PI*i/sides - OBS_PI/2.f;
            glVertex2f(cosf(a)*r, sinf(a)*r);
        }
        glEnd();

        glColor4f(0.85f,0.25f,0.30f,0.95f);
        for (int i = 0; i < sides; i++) {
            float a = 2*OBS_PI*i/sides - OBS_PI/2.f;
            float bx1=cosf(a-.28f)*r, by1=sinf(a-.28f)*r;
            float bx2=cosf(a+.28f)*r, by2=sinf(a+.28f)*r;
            float tx=cosf(a)*r*1.75f, ty=sinf(a)*r*1.75f;
            glBegin(GL_TRIANGLES);
            glVertex2f(bx1,by1); glVertex2f(bx2,by2); glVertex2f(tx,ty);
            glEnd();
        }
        float pulse = 0.4f + 0.35f*sinf(g_time*5.f);
        obs_circle(0,0, r*0.45f, 10, 1.f,0.3f,0.35f, pulse);
        glPopMatrix();
    }
};

// ============================================================
//  TURRET (unchanged - kept for later levels)
// ============================================================
struct Turret {
    float x, y, aimAngle, sc, shootTimer, shootInterval;
    int   hp, maxHp;
    bool  active;

    struct TBullet { float x,y,vx,vy; bool active; }
    bullets[OBS_TURRET_BULLET_MAX];

    void spawn(float px, float py, float scale, float interval = 2.5f) {
        x=px; y=py; sc=scale;
        aimAngle = OBS_PI/2.f;
        shootInterval = interval; shootTimer = interval;
        hp = maxHp = 6; active = true;
        for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++)
            bullets[i].active = false;
    }

    void hit(int dmg = 1) {
        if (!active) return;
        hp -= dmg;
        if (hp <= 0) {
            active = false;
            for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++)
                bullets[i].active = false;
        }
    }

    bool bulletHits(float px, float py, float pr) {
        for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++) {
            if (!bullets[i].active) continue;
            float dx = bullets[i].x - px, dy = bullets[i].y - py;
            if (dx*dx + dy*dy < (5.5f+pr)*(5.5f+pr)) {
                bullets[i].active = false; return true;
            }
        }
        return false;
    }

    void update(float dt, float px, float py) {
        if (!active) return;
        float dx = px-x, dy = py-y;
        float target = atan2f(dy,dx) - OBS_PI/2.f;
        float diff = target - aimAngle;
        while (diff >  OBS_PI) diff -= 2*OBS_PI;
        while (diff < -OBS_PI) diff += 2*OBS_PI;
        aimAngle += diff * 2.5f * dt;

        shootTimer -= dt;
        if (shootTimer <= 0) {
            shootTimer = shootInterval;
            for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = x + sinf(aimAngle)*sc;
                    bullets[i].y = y + cosf(aimAngle)*sc;
                    float spd = 220.f;
                    bullets[i].vx = sinf(aimAngle)*spd;
                    bullets[i].vy = cosf(aimAngle)*spd;
                    break;
                }
            }
        }
        for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++) {
            if (!bullets[i].active) continue;
            bullets[i].x += bullets[i].vx*dt;
            bullets[i].y += bullets[i].vy*dt;
            if (bullets[i].x < -10 || bullets[i].x > g_xres+10 ||
                bullets[i].y < -10 || bullets[i].y > g_yres+10)
                bullets[i].active = false;
        }
    }

    void draw() {
        if (!active) return;
        glDisable(GL_TEXTURE_2D);
        obs_circle(x,y, sc*.68f, 24, 0.08f,0.14f,0.28f,1.f);
        obs_ring  (x,y, sc*.68f, 24, 0.25f,0.45f,0.80f,0.7f,1.5f);
        obs_drawTank(x,y,sc,aimAngle,0.18f,0.40f,0.82f);

        for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++) {
            if (!bullets[i].active) continue;
            float bx=bullets[i].x, by=bullets[i].y;
            obs_circle(bx,by, 5.5f,8, 1.f,0.95f,0.3f,1.f);
        }
    }
};

// ============================================================
//  WARP GATE - textured, drifting, teleports to random location
// ============================================================
struct WarpGate {
    float x, y;
    float vx, vy;
    float radius;
    bool  active;

    void spawn(float px, float py, float r) {
        x = px; y = py; radius = r;
        float ang = (float)(rand() % 628) / 100.f;
        float spd = 25.0f;
        vx = cosf(ang) * spd;
        vy = sinf(ang) * spd;
        active = true;
    }

    bool playerInside(float px, float py) const {
        if (!active) return false;
        float dx = px - x, dy = py - y;
        return (dx*dx + dy*dy) < radius*radius;
    }

    void update(float dt) {
        if (!active) return;
        x += vx * dt; y += vy * dt;
        if (x < -radius*2)         x = g_xres + radius*2;
        if (x > g_xres + radius*2) x = -radius*2;
        if (y < -radius*2)         y = g_yres + radius*2;
        if (y > g_yres + radius*2) y = -radius*2;
    }

    void draw() {
        if (!active) return;
        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, obs_portalTex);
        glPushMatrix();
        glTranslatef(x, y, 0);
        float r = radius;
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-r, -r);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-r,  r);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( r,  r);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( r, -r);
        glEnd();
        glPopMatrix();
    }
};

// ============================================================
//  Module-level arrays
// ============================================================
static Asteroid obs_asteroids[OBS_ASTEROID_COUNT];
static Barrier  obs_barriers [OBS_BARRIER_COUNT];
static Mine     obs_mines    [OBS_MINE_COUNT];
static Turret   obs_turret;
static WarpGate obs_gate;

// ============================================================
//  Texture loading
// ============================================================
static inline void obs_loadTextures()
{
    // asteroid with black background
    {
        int w, h;
        unsigned char *data = NULL;
        // same alphapreserving loader from enemy.cpp
        // dark pixels as transparent for asteroid
        char ppmname[160];
        snprintf(ppmname, sizeof(ppmname), "asteroid.ppm");
        char ts[512];
        snprintf(ts, sizeof(ts), "convert asteroid.png %s", ppmname);
        system(ts);

        FILE *fpi = fopen(ppmname, "rb");
        if (fpi) {
            char line[200];
            fgets(line, 200, fpi);
            fgets(line, 200, fpi);
            while (line[0] == '#') fgets(line, 200, fpi);
            sscanf(line, "%i %i", &w, &h);
            fgets(line, 200, fpi);

            data = new unsigned char[w*h*4];
            for (int i = 0; i < w*h; i++) {
                unsigned char r = fgetc(fpi);
                unsigned char g = fgetc(fpi);
                unsigned char b = fgetc(fpi);
                data[i*4+0] = r;
                data[i*4+1] = g;
                data[i*4+2] = b;
                data[i*4+3] = (r < 50 && g < 50 && b < 50) ? 0 : 255;
            }
            fclose(fpi);
            unlink(ppmname);

            glGenTextures(1, &obs_asteroidTex);
            glBindTexture(GL_TEXTURE_2D, obs_asteroidTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
            delete[] data;
        }
    }

    // portal with true alpha
    {
        int w = 0, h = 0;
        unsigned char *data = NULL;
        load_png_with_alpha("portal.png", &w, &h, &data);
        if (data) {
            glGenTextures(1, &obs_portalTex);
            glBindTexture(GL_TEXTURE_2D, obs_portalTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
            delete[] data;
        }
    }
}

// ============================================================
//  Reset / Init
// ============================================================
static inline void obstaclesReset()
{
    // deactivate not used
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) obs_asteroids[i].active = false;
    for (int i = 0; i < OBS_MINE_COUNT;     i++) obs_mines[i].active     = false;
    for (int i = 0; i < OBS_BARRIER_COUNT;  i++) obs_barriers[i].active  = false;
    obs_turret.active = false;

    // level 1
    // asteroids go around at dif sizes
    obs_asteroids[0].spawn(120, 380, ASTEROID_LARGE,  35, -0.4f);
    obs_asteroids[1].spawn(500, 200, ASTEROID_MEDIUM, 45,  0.6f);
    obs_asteroids[2].spawn(300, 100, ASTEROID_SMALL,  55, -0.9f);
    obs_asteroids[3].spawn(550, 380, ASTEROID_SMALL,  50,  0.7f);
    obs_asteroids[4].spawn(200, 250, ASTEROID_MEDIUM, 40, -0.5f);

    // mines
    obs_mines[0].spawn(440, 300, 16);
    obs_mines[1].spawn(200, 340, 14);
    obs_mines[2].spawn(350, 420, 15);

    // warp gate
    obs_gate.spawn(g_xres / 2.0f, g_yres / 2.0f, 45.0f);
}

static inline void obstaclesInit()
{
    srand((unsigned)time(NULL));
    obs_loadTextures();
    obstaclesReset();
}

// ============================================================
//  Update / Draw
// ============================================================
static inline void obstaclesUpdate(float dt, float playerX, float playerY)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) obs_asteroids[i].update(dt);
    for (int i = 0; i < OBS_BARRIER_COUNT;  i++) obs_barriers [i].update(dt);
    for (int i = 0; i < OBS_MINE_COUNT;     i++) obs_mines    [i].update(dt);
    obs_turret.update(dt, playerX, playerY);
    obs_gate.update(dt);
}

static inline void obstaclesDraw()
{
    for (int i = 0; i < OBS_BARRIER_COUNT;  i++) obs_barriers [i].draw();
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) obs_asteroids[i].draw();
    for (int i = 0; i < OBS_MINE_COUNT;     i++) obs_mines    [i].draw();
    obs_gate.draw();
    obs_turret.draw();
}

// ============================================================
//  Collision helpers
// ============================================================

// bullet hits asteroid and gives index and damage amount
// outputs -1 if no hit
static inline int obstaclesCheckBulletAsteroid(float cx, float cy, float cr)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (obs_asteroids[i].checkHit(cx, cy, cr)) {
            obs_asteroids[i].damageBy(1);
            return i;
        }
    }
    return -1;
}

// player hits asteroid and returns damage amount 1/2/3 or 0 if no hit.
// also triggers explosion
static inline int obstaclesCheckPlayerAsteroid(float px, float py, float pr)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (obs_asteroids[i].checkHit(px, py, pr)) {
            int dmg = obs_asteroids[i].damage;
            obs_asteroids[i].startExplode();
            return dmg;
        }
    }
    return 0;
}

static inline int obstaclesCheckPlayerMine(float px, float py, float pr)
{
    for (int i = 0; i < OBS_MINE_COUNT; i++) {
        if (obs_mines[i].checkHit(px, py, pr)) {
            obs_mines[i].explode();
            return i;
        }
    }
    return -1;
}

static inline bool obstaclesCheckTurretHitsPlayer(float px, float py, float pr)
{
    return obs_turret.bulletHits(px, py, pr);
}

// warp gate returns random warp destination
static inline bool obstaclesCheckWarpGate(float px, float py,
                                           float *outX, float *outY)
{
    if (obs_gate.playerInside(px, py)) {
        *outX = 40.0f + (float)(rand() % (g_xres - 80));
        *outY = 40.0f + (float)(rand() % (g_yres - 80));
        return true;
    }
    return false;
}

static inline int obstaclesCheckPlayerBarrier(float px, float py, float pr)
{
    for (int i = 0; i < OBS_BARRIER_COUNT; i++) {
        if (obs_barriers[i].containsPoint(px, py, pr))
            return i;
    }
    return -1;
}

static inline int obstaclesCheckBulletBarrier(float cx, float cy, float cr)
{
    for (int i = 0; i < OBS_BARRIER_COUNT; i++) {
        if (obs_barriers[i].containsPoint(cx, cy, cr)) {
            obs_barriers[i].hit(1);
            return i;
        }
    }
    return -1;
}

// ============================================================
//  Add / Remove API
// ============================================================
static inline int obstaclesAddAsteroid(float x, float y, int sizeTier,
                                       float speed, float spin)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (!obs_asteroids[i].active) {
            obs_asteroids[i].spawn(x, y, sizeTier, speed, spin);
            return i;
        }
    }
    return -1;
}

static inline void obstaclesRemoveAsteroid(int index)
{
    if (index >= 0 && index < OBS_ASTEROID_COUNT)
        obs_asteroids[index].active = false;
}

static inline void obstaclesRemoveAllAsteroids()
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].active = false;
}

static inline int obstaclesAddMine(float x, float y, float r)
{
    for (int i = 0; i < OBS_MINE_COUNT; i++) {
        if (!obs_mines[i].active) {
            obs_mines[i].spawn(x, y, r);
            return i;
        }
    }
    return -1;
}

static inline void obstaclesRemoveMine(int index)
{
    if (index >= 0 && index < OBS_MINE_COUNT)
        obs_mines[index].active = false;
}

static inline void obstaclesRemoveAllMines()
{
    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].active = false;
}

#endif
