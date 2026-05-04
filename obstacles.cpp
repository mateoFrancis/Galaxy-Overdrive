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

#define OBS_ASTEROID_COUNT 8
#define OBS_BARRIER_COUNT  3
#define OBS_MINE_COUNT     6

// asteroid size tiers
#define ASTEROID_SMALL  0
#define ASTEROID_MEDIUM 1
#define ASTEROID_LARGE  2

static GLuint obs_asteroidTex = 0;
static GLuint obs_portalTex   = 0;

// primitive drawing helpers
static inline void obs_circle(float x, float y, float r, int s,
                              float R, float G, float B, float A = 1.f)
{
    glColor4f(R, G, B, A);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= s; i++) {
        float a = 2 * OBS_PI * i / s;
        glVertex2f(x + cosf(a) * r, y + sinf(a) * r);
    }

    glEnd();
}

static inline void obs_quad(float x, float y, float w, float h,
                            float R, float G, float B, float A = 1.f)
{
    glColor4f(R, G, B, A);

    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

// asteroid obstacle
struct Asteroid {
    float x, y, vx, vy, angle, spin, radius;
    int size, hp, damage;
    bool active;

    bool exploding;
    int explodeFrame;
    float explodeTimer;

    void spawn(float px, float py, int sizeTier, float spd, float sp) {
        x = px;
        y = py;
        size = sizeTier;

        if (sizeTier == ASTEROID_SMALL) {
            radius = 16;
            hp = 1;
            damage = 1;
        } else if (sizeTier == ASTEROID_MEDIUM) {
            radius = 28;
            hp = 2;
            damage = 2;
        } else {
            radius = 40;
            hp = 3;
            damage = 3;
        }

        float ang = (float)(rand() % 628) / 100.f;

        vx = cosf(ang) * spd;
        vy = sinf(ang) * spd;
        spin = sp;
        angle = 0;

        active = true;
        exploding = false;
        explodeFrame = 0;
        explodeTimer = 0;
    }

    bool checkHit(float bx, float by, float br) const {
        if (!active || exploding)
            return false;

        float dx = bx - x;
        float dy = by - y;

        return (dx * dx + dy * dy) < (radius + br) * (radius + br);
    }

    void startExplode() {
        exploding = true;
        explodeFrame = 0;
        explodeTimer = 0;
        vx = 0;
        vy = 0;
        spin = 0;
    }

    bool damageBy(int dmg = 1) {
        if (!active || exploding)
            return false;

        hp -= dmg;

        if (hp <= 0) {
            startExplode();
            return true;
        }

        return false;
    }

    void update(float dt) {
        if (!active)
            return;

        if (exploding) {
            explodeTimer += dt;

            if (explodeTimer >= get_explosion_frame_time()) {
                explodeTimer = 0;
                explodeFrame++;

                if (explodeFrame >= get_explosion_frames())
                    active = false;
            }

            return;
        }

        x += vx * dt;
        y += vy * dt;
        angle += spin * dt;

        if (x < -60)
            x = g_xres + 60;

        if (x > g_xres + 60)
            x = -60;

        if (y < -60)
            y = g_yres + 60;

        if (y > g_yres + 60)
            y = -60;
    }

    void draw() {
        if (!active)
            return;

        if (exploding) {
            float fw = 1.f / (float)get_explosion_frames();
            float t0 = fw * (float)explodeFrame;
            float t1 = t0 + fw;
            float r = radius;

            glEnable(GL_TEXTURE_2D);
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, get_explosion_texture());

            glPushMatrix();
            glTranslatef(x, y, 0);

            glBegin(GL_QUADS);
            glTexCoord2f(t0, 1); glVertex2f(-r, -r);
            glTexCoord2f(t0, 0); glVertex2f(-r,  r);
            glTexCoord2f(t1, 0); glVertex2f( r,  r);
            glTexCoord2f(t1, 1); glVertex2f( r, -r);
            glEnd();

            glPopMatrix();

            return;
        }

        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, obs_asteroidTex);

        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);

        float r = radius;

        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(-r, -r);
        glTexCoord2f(0, 0); glVertex2f(-r,  r);
        glTexCoord2f(1, 0); glVertex2f( r,  r);
        glTexCoord2f(1, 1); glVertex2f( r, -r);
        glEnd();

        glPopMatrix();
    }
};

// barrier obstacle
struct Barrier {
    float x, y, w, h, angle;
    int hp, maxHp;
    float flashTimer;
    bool active;

    void spawn(float px, float py, float width, float height, float ang) {
        x = px;
        y = py;
        w = width;
        h = height;
        angle = ang;

        hp = maxHp = 5;
        flashTimer = 0;
        active = true;
    }

    void hit(int dmg = 1) {
        if (!active)
            return;

        hp -= dmg;
        flashTimer = 0.12f;

        if (hp <= 0)
            active = false;
    }

    bool containsPoint(float px, float py, float pr = 0) const {
        if (!active)
            return false;

        return px + pr > x - w * 0.5f &&
               px - pr < x + w * 0.5f &&
               py + pr > y - h * 0.5f &&
               py - pr < y + h * 0.5f;
    }

    void update(float dt) {
        if (flashTimer > 0)
            flashTimer -= dt;
    }

    void draw() {
        if (!active)
            return;

        float hf = (float)hp / maxHp;
        float flash = (flashTimer > 0) ? 1.f : 0.f;

        glDisable(GL_TEXTURE_2D);

        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);

        glColor4f(0.22f + (1 - hf) * 0.55f + flash * 0.4f,
                  0.38f * hf + flash * 0.4f,
                  0.62f * hf + flash * 0.4f,
                  1);

        glBegin(GL_QUADS);
        glVertex2f(-w * 0.5f, -h * 0.5f);
        glVertex2f( w * 0.5f, -h * 0.5f);
        glVertex2f( w * 0.5f,  h * 0.5f);
        glVertex2f(-w * 0.5f,  h * 0.5f);
        glEnd();

        glLineWidth(2);
        glColor4f(0.55f, 0.75f, 1, 0.85f);

        glBegin(GL_LINE_LOOP);
        glVertex2f(-w * 0.5f, -h * 0.5f);
        glVertex2f( w * 0.5f, -h * 0.5f);
        glVertex2f( w * 0.5f,  h * 0.5f);
        glVertex2f(-w * 0.5f,  h * 0.5f);
        glEnd();

        glPopMatrix();

        float barW = w * 0.85f;
        float barX = x - barW * 0.5f;
        float barY = y - h * 0.5f - 10;

        obs_quad(barX, barY, barW, 5, 0.15f, 0.15f, 0.15f, 0.8f);
        obs_quad(barX, barY, barW * hf, 5,
                 0.2f + 0.8f * (1 - hf),
                 0.8f * hf,
                 0.2f,
                 0.9f);
    }
};

// mine obstacle
struct Mine {
    float x, y, vx, vy, angle, r;
    bool active;

    void spawn(float px, float py, float radius) {
        x = px;
        y = py;
        r = radius;

        float ang = (float)(rand() % 628) / 100.f;

        vx = cosf(ang) * 18;
        vy = sinf(ang) * 18;
        angle = 0;
        active = true;
    }

    bool checkHit(float bx, float by, float br) const {
        if (!active)
            return false;

        float dx = bx - x;
        float dy = by - y;
        float cr = r * 1.75f + br;

        return (dx * dx + dy * dy) < cr * cr;
    }

    void explode() {
        active = false;
    }

    void update(float dt) {
        if (!active)
            return;

        x += vx * dt;
        y += vy * dt;
        angle += 0.8f * dt;

        if (x < -r * 2)
            x = g_xres + r * 2;

        if (x > g_xres + r * 2)
            x = -r * 2;

        if (y < -r * 2)
            y = g_yres + r * 2;

        if (y > g_yres + r * 2)
            y = -r * 2;
    }

    void draw() {
        if (!active)
            return;

        glDisable(GL_TEXTURE_2D);

        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(angle * 180.f / OBS_PI, 0, 0, 1);

        int sides = 5;

        glColor4f(0.55f, 0.18f, 0.22f, 1);

        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);

        for (int i = 0; i <= sides; i++) {
            float a = 2 * OBS_PI * i / sides - OBS_PI / 2;
            glVertex2f(cosf(a) * r, sinf(a) * r);
        }

        glEnd();

        glLineWidth(2);
        glColor4f(1, 0.35f, 0.4f, 0.9f);

        glBegin(GL_LINE_LOOP);

        for (int i = 0; i < sides; i++) {
            float a = 2 * OBS_PI * i / sides - OBS_PI / 2;
            glVertex2f(cosf(a) * r, sinf(a) * r);
        }

        glEnd();

        glColor4f(0.85f, 0.25f, 0.30f, 0.95f);

        for (int i = 0; i < sides; i++) {
            float a = 2 * OBS_PI * i / sides - OBS_PI / 2;

            float bx1 = cosf(a - 0.28f) * r;
            float by1 = sinf(a - 0.28f) * r;
            float bx2 = cosf(a + 0.28f) * r;
            float by2 = sinf(a + 0.28f) * r;
            float tx = cosf(a) * r * 1.75f;
            float ty = sinf(a) * r * 1.75f;

            glBegin(GL_TRIANGLES);
            glVertex2f(bx1, by1);
            glVertex2f(bx2, by2);
            glVertex2f(tx,  ty);
            glEnd();
        }

        float pulse = 0.4f + 0.35f * sinf(g_time * 5);

        obs_circle(0, 0, r * 0.45f, 10, 1, 0.3f, 0.35f, pulse);

        glPopMatrix();
    }
};

// warp gate obstacle
struct WarpGate {
    float x, y, vx, vy, radius;
    bool active;

    void spawn(float px, float py, float r) {
        x = px;
        y = py;
        radius = r;

        float ang = (float)(rand() % 628) / 100.f;

        vx = cosf(ang) * 25;
        vy = sinf(ang) * 25;
        active = true;
    }

    bool playerInside(float px, float py) const {
        if (!active)
            return false;

        float dx = px - x;
        float dy = py - y;

        return (dx * dx + dy * dy) < radius * radius;
    }

    void update(float dt) {
        if (!active)
            return;

        x += vx * dt;
        y += vy * dt;

        if (x < -radius * 2)
            x = g_xres + radius * 2;

        if (x > g_xres + radius * 2)
            x = -radius * 2;

        if (y < -radius * 2)
            y = g_yres + radius * 2;

        if (y > g_yres + radius * 2)
            y = -radius * 2;
    }

    void draw() {
        if (!active)
            return;

        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, obs_portalTex);

        glPushMatrix();
        glTranslatef(x, y, 0);

        float r = radius;

        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(-r, -r);
        glTexCoord2f(0, 0); glVertex2f(-r,  r);
        glTexCoord2f(1, 0); glVertex2f( r,  r);
        glTexCoord2f(1, 1); glVertex2f( r, -r);
        glEnd();

        glPopMatrix();
    }
};

// obstacle arrays
static Asteroid obs_asteroids[OBS_ASTEROID_COUNT];
static Barrier  obs_barriers[OBS_BARRIER_COUNT];
static Mine     obs_mines[OBS_MINE_COUNT];
static WarpGate obs_gate;

// texture loading
static inline void obs_loadTextures()
{
    { // asteroid texture
        int w, h;
        unsigned char *data = NULL;

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

            while (line[0] == '#')
                fgets(line, 200, fpi);

            sscanf(line, "%i %i", &w, &h);
            fgets(line, 200, fpi);

            data = new unsigned char[w * h * 4];

            for (int i = 0; i < w * h; i++) {
                unsigned char r = fgetc(fpi);
                unsigned char g = fgetc(fpi);
                unsigned char b = fgetc(fpi);

                data[i * 4]     = r;
                data[i * 4 + 1] = g;
                data[i * 4 + 2] = b;
                data[i * 4 + 3] = (r < 20 && g < 20 && b < 20) ? 0 : 255;
            }

            fclose(fpi);
            unlink(ppmname);

            glGenTextures(1, &obs_asteroidTex);
            glBindTexture(GL_TEXTURE_2D, obs_asteroidTex);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         w, h,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            delete [] data;
        }
    }

    { // portal texture
        int w = 0;
        int h = 0;
        unsigned char *data = NULL;

        load_png_with_alpha("portal.png", &w, &h, &data);

        if (data) {
            glGenTextures(1, &obs_portalTex);
            glBindTexture(GL_TEXTURE_2D, obs_portalTex);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         w, h,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            delete [] data;
        }
    }
}

// reset and init
static inline void obstaclesReset()
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].active = false;

    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].active = false;

    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].active = false;

    obs_asteroids[0].spawn(120, 380, ASTEROID_LARGE,  35, -0.4f);
    obs_asteroids[1].spawn(500, 200, ASTEROID_MEDIUM, 45,  0.6f);
    obs_asteroids[2].spawn(300, 100, ASTEROID_SMALL,  55, -0.9f);
    obs_asteroids[3].spawn(550, 380, ASTEROID_SMALL,  50,  0.7f);
    obs_asteroids[4].spawn(200, 250, ASTEROID_MEDIUM, 40, -0.5f);

    obs_mines[0].spawn(440, 300, 16);
    obs_mines[1].spawn(200, 340, 14);
    obs_mines[2].spawn(350, 420, 15);

    obs_gate.spawn(g_xres / 2.0f, g_yres / 2.0f, 45.0f);
}

static inline void obstaclesInit()
{
    srand((unsigned)time(NULL));
    obs_loadTextures();
    obstaclesReset();
}

// update and draw
static inline void obstaclesUpdate(float dt, float playerX, float playerY)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].update(dt);

    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].update(dt);

    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].update(dt);

    obs_gate.update(dt);
}

static inline void obstaclesDraw()
{
    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].draw();

    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].draw();

    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].draw();

    obs_gate.draw();
}

// collision helpers
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

static inline int obstaclesCheckPlayerAsteroid(float px, float py, float pr)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (obs_asteroids[i].checkHit(px, py, pr)) {
            int d = obs_asteroids[i].damage;
            obs_asteroids[i].startExplode();
            return d;
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

static inline bool obstaclesCheckWarpGate(float px, float py, float *outX, float *outY)
{
    if (obs_gate.playerInside(px, py)) {
        *outX = 40 + (float)(rand() % (g_xres - 80));
        *outY = 40 + (float)(rand() % (g_yres - 80));
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

// add and remove helpers
static inline int obstaclesAddAsteroid(float x, float y, int sizeTier, float speed, float spin)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++) {
        if (!obs_asteroids[i].active) {
            obs_asteroids[i].spawn(x, y, sizeTier, speed, spin);
            return i;
        }
    }

    return -1;
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

static inline void obstaclesRemoveAllMines()
{
    for (int i = 0; i < OBS_MINE_COUNT; i++)
        obs_mines[i].active = false;
}

static inline void obstaclesSpawnTitleAsteroids()
{
    obstaclesRemoveAllAsteroids();

    int sizes[] = {
        ASTEROID_LARGE,
        ASTEROID_MEDIUM,
        ASTEROID_SMALL,
        ASTEROID_MEDIUM,
        ASTEROID_SMALL,
        ASTEROID_LARGE
    };

    float speeds[] = {
        30, 40, 55, 45, 50, 35
    };

    float spins[] = {
        -0.4f, 0.6f, -0.9f, 0.7f, -0.5f, 0.8f
    };

    for (int i = 0; i < 6 && i < OBS_ASTEROID_COUNT; i++) {
        float rx = (float)(rand() % g_xres);
        float ry = (float)(rand() % g_yres);

        obstaclesAddAsteroid(rx, ry, sizes[i], speeds[i], spins[i]);
    }
}

static inline void obstaclesUpdateAsteroidsOnly(float dt)
{
    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].update(dt);
}

static inline void obstaclesDrawAsteroidsOnly()
{
    glEnable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);

    for (int i = 0; i < OBS_ASTEROID_COUNT; i++)
        obs_asteroids[i].draw();
}

#endif // obstacles_h
