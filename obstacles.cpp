#ifndef OBSTACLES_H
#define OBSTACLES_H

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "fonts.h"

#ifndef OBSTACLES_INCLUDED_BY_BACKGROUND
extern int   g_xres;
extern int   g_yres;
extern float g_time;
#endif
#define OBSTACLES_INCLUDED_BY_BACKGROUND

#ifndef OBS_PI
#define OBS_PI 3.14159265358979f
#endif

#define OBS_ASTEROID_COUNT    8   // pool size — actual active count may be less
#define OBS_BARRIER_COUNT     3
#define OBS_MINE_COUNT        6   // pool size
#define OBS_TURRET_BULLET_MAX 4

// ============================================================
//  Primitive helpers
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
//  ASTEROID
// ============================================================
struct Asteroid {
    float x, y;
    float vx, vy;
    float angle, spin;
    float baseR;
    int   sides;
    float radii[14];
    int   hp, maxHp;
    bool  active;

    void spawn(float px, float py, float r, float spd, float sp) {
        x = px; y = py; baseR = r;
        sides = 9 + (rand() % 4);
        float ang = (float)(rand() % 628) / 100.f;
        vx = cosf(ang) * spd; vy = sinf(ang) * spd;
        spin = sp; angle = 0.f;
        hp = maxHp = 3;
        active = true;
        for (int i = 0; i < sides; i++)
            radii[i] = r * (0.72f + (rand() % 28) * 0.01f);
    }

    bool checkHit(float bx, float by, float br) const {
        if (!active) return false;
        float dx = bx - x, dy = by - y;
        return (dx*dx + dy*dy) < (baseR + br) * (baseR + br);
    }

    // Returns true when destroyed
    bool damage(int dmg = 1) {
        if (!active) return false;
        hp -= dmg;
        if (hp <= 0) { active = false; return true; }
        return false;
    }

    void update(float dt) {
        if (!active) return;
        x += vx * dt; y += vy * dt;
        angle += spin * dt;
        if (x < -60)         x = g_xres + 60;
        if (x > g_xres + 60) x = -60;
        if (y < -60)         y = g_yres + 60;
        if (y > g_yres + 60) y = -60;
    }

    void draw() {
        if (!active) return;
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);

        glColor4f(0.38f, 0.32f, 0.28f, 1.f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int i = 0; i <= sides; i++) {
            float a = 2*OBS_PI*i/sides;
            glVertex2f(cosf(a)*radii[i%sides], sinf(a)*radii[i%sides]);
        }
        glEnd();

        glLineWidth(1.8f); glColor4f(0.62f, 0.56f, 0.50f, 0.9f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < sides; i++) {
            float a = 2*OBS_PI*i/sides;
            glVertex2f(cosf(a)*radii[i], sinf(a)*radii[i]);
        }
        glEnd();

        glLineWidth(1.f); glColor4f(0.20f, 0.16f, 0.14f, 0.7f);
        float seed = radii[0];
        glBegin(GL_LINES);
        glVertex2f(0, 0);
        glVertex2f(cosf(seed)*baseR*.55f, sinf(seed)*baseR*.55f);
        glVertex2f(cosf(seed)*baseR*.55f, sinf(seed)*baseR*.55f);
        glVertex2f(cosf(seed+1.1f)*baseR*.85f, sinf(seed+1.1f)*baseR*.85f);
        glVertex2f(0, 0);
        glVertex2f(cosf(seed+2.5f)*baseR*.45f, sinf(seed+2.5f)*baseR*.45f);
        glEnd();

        glPopMatrix();

        // Health pips
        float spacing = 10.f, totalW = maxHp * spacing - 2.f;
        float startX = x - totalW * 0.5f, pipY = y - baseR - 10.f;
        for (int i = 0; i < maxHp; i++) {
            float cx = startX + i * spacing;
            if (i < hp) obs_circle(cx, pipY, 3.5f, 6, 0.3f, 0.9f, 0.4f, 0.9f);
            else        obs_ring  (cx, pipY, 3.5f, 6, 0.3f, 0.9f, 0.4f, 0.5f, 1.f);
        }
    }
};

// ============================================================
//  BARRIER WALL
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

    // Axis-aligned containment test (sufficient for non-rotated barriers)
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

        glLineWidth(1.f); glColor4f(0.15f, 0.25f, 0.45f, 0.6f);
        float step = w / 4.f;
        glBegin(GL_LINES);
        for (int i = 1; i < 4; i++) {
            float lx = -w*.5f + i*step;
            glVertex2f(lx, -h*.5f); glVertex2f(lx, h*.5f);
        }
        glEnd();

        glLineWidth(2.f); glColor4f(0.55f, 0.75f, 1.f, 0.85f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-w*.5f,-h*.5f); glVertex2f( w*.5f,-h*.5f);
        glVertex2f( w*.5f, h*.5f); glVertex2f(-w*.5f, h*.5f);
        glEnd();

        if (hp <= 2) {
            glLineWidth(1.f); glColor4f(0.f,0.f,0.f,0.55f);
            glBegin(GL_LINES);
            glVertex2f(-w*.3f,h*.2f); glVertex2f(w*.1f,-h*.3f);
            glVertex2f(w*.2f,h*.4f);  glVertex2f(-w*.1f,-h*.1f);
            glEnd();
        }
        glPopMatrix();

        // Health bar
        float barW = w * 0.85f, barX = x - barW*0.5f, barY = y - h*0.5f - 10.f;
        obs_quad(barX, barY, barW, 5.f, 0.15f,0.15f,0.15f,0.8f);
        obs_quad(barX, barY, barW*hf, 5.f,
                 0.2f+0.8f*(1.f-hf), 0.8f*hf, 0.2f, 0.9f);
        glLineWidth(1.f); glColor4f(0.5f,0.7f,1.f,0.6f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(barX,barY); glVertex2f(barX+barW,barY);
        glVertex2f(barX+barW,barY+5); glVertex2f(barX,barY+5);
        glEnd();
    }
};

// ============================================================
//  SPINNING MINE
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
//  TURRET
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
        obs_circle(x,y, sc*.68f, 24, 0.08f,0.14f,0.28f,1.f);
        obs_ring  (x,y, sc*.68f, 24, 0.25f,0.45f,0.80f,0.7f,1.5f);

        glColor4f(0.2f,0.5f,1.f,0.12f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x,y);
        for (int i = 0; i <= 16; i++) {
            float a = (aimAngle-.35f) + i*0.70f/16.f;
            glVertex2f(x+sinf(a)*sc*3.5f, y+cosf(a)*sc*3.5f);
        }
        glEnd();

        obs_drawTank(x,y,sc,aimAngle,0.18f,0.40f,0.82f);

        float lx = x + sinf(aimAngle)*sc*3.2f;
        float ly = y + cosf(aimAngle)*sc*3.2f;
        float blink = 0.5f + 0.5f*sinf(g_time*8.f);
        obs_circle(lx,ly, 3.f,8, 1.f,0.3f,0.3f, blink*0.85f);

        for (int i = 0; i < OBS_TURRET_BULLET_MAX; i++) {
            if (!bullets[i].active) continue;
            float bx=bullets[i].x, by=bullets[i].y;
            float bdx=bullets[i].vx/220.f, bdy=bullets[i].vy/220.f;
            for (int t=1; t<=4; t++)
                obs_circle(bx-bdx*t*4.f, by-bdy*t*4.f,
                           4.5f*(1-t*.18f), 6, 1.f,0.85f,0.2f, 0.5f/t);
            obs_circle(bx,by, 5.5f,8, 1.f,0.95f,0.3f,1.f);
        }

        float barW=sc*1.4f, barX=x-barW*.5f, barY=y+sc*.75f+6.f;
        float frac=(float)hp/maxHp;
        obs_quad(barX,barY,barW,        6.f,0.1f,0.1f,0.1f,0.8f);
        obs_quad(barX,barY,barW*frac,   6.f,0.2f+0.8f*(1.f-frac),0.8f*frac,0.2f,0.95f);
        glLineWidth(1.f); glColor4f(0.4f,0.6f,1.f,0.6f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(barX,barY); glVertex2f(barX+barW,barY);
        glVertex2f(barX+barW,barY+6); glVertex2f(barX,barY+6);
        glEnd();
    }
};

// ============================================================
//  WARP GATE
// ============================================================
struct WarpGate {
    float x, y, outerR, innerR, spin;
    bool  active;

    void spawn(float px, float py, float r) {
        x=px; y=py; outerR=r; innerR=r*.65f; spin=0.f; active=true;
    }

    bool playerInside(float px, float py) const {
        if (!active) return false;
        float dx=px-x, dy=py-y;
        return (dx*dx+dy*dy) < innerR*innerR;
    }

    void update(float dt) { if (active) spin += 0.55f*dt; }

    void draw() {
        if (!active) return;
        float pulse = 0.5f + 0.5f*sinf(g_time*2.2f);

        obs_ring(x,y,outerR+8.f+pulse*6.f,48,0.15f,0.6f,1.f,0.18f*pulse,1.f);
        obs_ring(x,y,outerR,              48,0.25f,0.7f,1.f,0.55f+0.2f*pulse,2.5f);

        glPushMatrix();
        glTranslatef(x,y,0); glRotatef(spin*180.f/OBS_PI,0,0,1);

        int segs=16;
        for (int i=0; i<segs; i++) {
            float a0=2*OBS_PI*i/segs, a1=2*OBS_PI*(i+0.72f)/segs;
            float even=(i%2==0)?1.f:0.55f;
            glColor4f(0.1f*even,0.45f*even,0.9f*even,0.75f);
            glBegin(GL_QUADS);
            glVertex2f(cosf(a0)*innerR,sinf(a0)*innerR);
            glVertex2f(cosf(a0)*outerR,sinf(a0)*outerR);
            glVertex2f(cosf(a1)*outerR,sinf(a1)*outerR);
            glVertex2f(cosf(a1)*innerR,sinf(a1)*innerR);
            glEnd();
        }
        glRotatef(-spin*360.f/OBS_PI,0,0,1);
        obs_ring(0,0,innerR,48,0.4f,0.9f,1.f,0.6f+0.3f*pulse,1.8f);
        glPopMatrix();

        obs_circle(x,y,innerR*.85f,32,0.05f,0.15f+0.2f*pulse,0.35f+0.3f*pulse,0.7f);
        obs_circle(x,y,innerR*.4f, 20,0.3f,0.7f+0.3f*pulse,1.f,0.5f*pulse);

        Rect r; r.bot=(int)(y-outerR-18); r.left=(int)(x-38); r.center=0;
        ggprint8b(&r,0,0x00aaddff,"WARP GATE");
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
//  Reset / Init
// ============================================================
static inline void obstaclesReset()
{
    // Deactivate all pooled slots first
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) obs_asteroids[i].active = false;
    for (int i = 0; i < OBS_MINE_COUNT;     i++) obs_mines[i].active     = false;

    // Default wave
    obs_asteroids[0].spawn(160, 440, 38, 22, -0.4f);
    obs_asteroids[1].spawn(300, 200, 24, 30,  0.6f);
    obs_asteroids[2].spawn( 80, 180, 18, 40, -0.9f);

    obs_barriers[0].spawn(520, 480, 120, 22, 0);
    obs_barriers[1].spawn(600, 430, 120, 22, 0);
    obs_barriers[2].spawn(560, 455,  22, 68, OBS_PI/2.f);

    obs_mines[0].spawn(440, 300, 18);
    obs_mines[1].spawn(200, 340, 14);

    obs_turret.spawn(620, 200, 26, 2.5f);
    obs_gate.spawn  (390, 130, 52);
}

static inline void obstaclesInit()
{
    srand((unsigned)time(NULL));
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

// Bullet → Asteroid.  Damages asteroid.  Returns index or -1.
static inline int obstaclesCheckBulletAsteroid(float cx, float cy, float cr)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (obs_asteroids[i].checkHit(cx, cy, cr)) {
            obs_asteroids[i].damage();
            return i;
        }
    }
    return -1;
}

// Player → Mine.  Explodes mine.  Returns index or -1.
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

// Turret bullet → Player.  Deactivates bullet.  Returns true on hit.
static inline bool obstaclesCheckTurretHitsPlayer(float px, float py, float pr)
{
    return obs_turret.bulletHits(px, py, pr);
}

// Player inside warp gate.  No side-effect.
static inline bool obstaclesCheckWarpGate(float px, float py)
{
    return obs_gate.playerInside(px, py);
}

// Player circle → any barrier.  Returns index or -1 (no damage).
static inline int obstaclesCheckPlayerBarrier(float px, float py, float pr)
{
    for (int i = 0; i < OBS_BARRIER_COUNT; i++) {
        if (obs_barriers[i].containsPoint(px, py, pr))
            return i;
    }
    return -1;
}

// Bullet → Barrier.  Damages barrier.  Returns index or -1.
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

// Spawn an extra asteroid in the first inactive pool slot.
// Returns the slot index, or -1 if the pool is full.
static inline int obstaclesAddAsteroid(float x, float y, float r,
                                       float speed, float spin)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (!obs_asteroids[i].active) {
            obs_asteroids[i].spawn(x, y, r, speed, spin);
            return i;
        }
    }
    return -1;
}

// Deactivate a specific asteroid by index.
static inline void obstaclesRemoveAsteroid(int index)
{
    if (index >= 0 && index < OBS_ASTEROID_COUNT)
        obs_asteroids[index].active = false;
}

// Deactivate all asteroids.
static inline void obstaclesRemoveAllAsteroids()
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].active = false;
}

// Spawn an extra mine.  Returns slot index or -1.
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

// Deactivate a specific mine by index.
static inline void obstaclesRemoveMine(int index)
{
    if (index >= 0 && index < OBS_MINE_COUNT)
        obs_mines[index].active = false;
}

// Deactivate all mines.
static inline void obstaclesRemoveAllMines()
{
    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].active = false;
}

#endif // OBSTACLES_H
