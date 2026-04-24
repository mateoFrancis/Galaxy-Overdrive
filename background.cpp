#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "fonts.h"
#include "enemy.h"
#include <ctime>

int   g_xres = 640;
int   g_yres = 480;
float g_time  = 0.0f;

#include "obstacles.cpp"
//#include "enemy.cpp"
//#include "enemy.h"

#define MAX_BULLETS 50
const int VIRTUAL_W = 640;
const int VIRTUAL_H = 480;

enum GameState {
    STATE_TITLE,
    STATE_LEVEL_INTRO,
    STATE_PLAYING
};

class Image {
public:
    int width, height;
    unsigned char *data;

    Image() : width(0), height(0), data(NULL) {}

    ~Image() { delete[] data; }

    void load(const char *fname) {
        if (!fname || fname[0] == '\0') return;

        char name[256];
        strncpy(name, fname, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';

        int slen = (int)strlen(name);
        if (slen >= 4) name[slen - 4] = '\0';

        char ppmname[512];
        snprintf(ppmname, sizeof(ppmname), "%s.ppm", name);

        char ts[1024];
        snprintf(ts, sizeof(ts), "convert %s %s", fname, ppmname);
        system(ts);

        FILE *fpi = fopen(ppmname, "rb");
        if (!fpi) {
            printf("ERROR opening image: %s\n", ppmname);
            exit(1);
        }

        char line[200];
        fgets(line, 200, fpi);
        fgets(line, 200, fpi);
        while (line[0] == '#')
            fgets(line, 200, fpi);

        sscanf(line, "%i %i", &width, &height);
        fgets(line, 200, fpi);

        int n = width * height * 4;
        data = new unsigned char[n];

        for (int i = 0; i < width * height; i++) {
            unsigned char r = (unsigned char)fgetc(fpi);
            unsigned char g = (unsigned char)fgetc(fpi);
            unsigned char b = (unsigned char)fgetc(fpi);
            data[i*4+0] = r;
            data[i*4+1] = g;
            data[i*4+2] = b;
            data[i*4+3] = (r < 50 && g < 50 && b < 50) ? 0 : 255;
        }
        fclose(fpi);
        unlink(ppmname);
    }
};

struct Texture {
    GLuint backTex;
    GLuint logoTex;
    GLuint ship01Tex;
    GLuint shipSlightTex;
    GLuint shipMidTex;
    GLuint shipVeryTex;
    GLuint cannonTex;
    GLuint rocketsTex;
    GLuint spaceGunTex;
    GLuint zapperTex;
    GLuint bulletCannonTex;
    GLuint bulletRocketTex;
    GLuint bulletSpaceGunTex;
    GLuint bulletZapperTex;
    GLuint raysTex;
    GLuint healthTex;

    int logoW, logoH;
    float xc[2];
    float yc[2];
};

struct Bullet {
    float x, y;
    float xVel, yVel;
    float vel;
    int   active;
    int   type;
    int   frame;
    float frameTimer;
    float angle;
};

struct TitleAnim {
    float timer;
    TitleAnim() : timer(0.0f) {}
};

struct StartButton {
    float x, y;
    float w, h;
    bool  visible;

    StartButton() : x(0), y(0), w(200), h(50), visible(false) {}

    bool contains(float px, float py) const {
        return visible &&
               px >= x - w/2 && px <= x + w/2 &&
               py >= y - h/2 && py <= y + h/2;
    }
};

class Global {
public:
    int xres, yres;
    Texture tex;
    TitleAnim title;

    int mousex, mousey;
    int spacePressed;
    int movSwitch;
    int currentWeapon;

    int   weaponFrame;
    float weaponTimer;

    float shipx, shipy;
    float ShipSpeed;
    float shipAngle;
    int   fps;
    int paused;

    int   keys[512];
    Bullet bullets[MAX_BULLETS];

    int   playerHP;
    int   score;

    float spawnTimer;
    float spawnInterval;

    GameState state;
    float     levelIntroTimer;
    int       currentLevel;
    StartButton startBtn;
    float displayHP;

    Global()
        : xres(640), yres(480),
          mousex(320), mousey(240),
          spacePressed(0), movSwitch(0), currentWeapon(0),
          weaponFrame(0), weaponTimer(0.0f),
          shipx(320.0f), shipy(160.0f),
          ShipSpeed(6.0f), shipAngle(0.0f),
          fps(0), playerHP(10), score(0),
          spawnTimer(0.0f), spawnInterval(2.0f),
          state(STATE_TITLE), levelIntroTimer(0.0f), currentLevel(1)
         
    {
        memset(keys, 0, sizeof(keys));
        displayHP = playerHP;
        paused = 0;
        for (int i = 0; i < MAX_BULLETS; i++)
            bullets[i].active = 0;
    }
} g;

class X11_wrapper {
    Display  *dpy;
    Window    win;
    GLXContext glc;
public:
    X11_wrapper() {
        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        dpy = XOpenDisplay(NULL);
        if (!dpy) { printf("Cannot connect to X server\n"); exit(1); }

        Window root = DefaultRootWindow(dpy);
        XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
        if (!vi) { printf("No appropriate visual\n"); exit(1); }

        Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
        XSetWindowAttributes swa;
        swa.colormap  = cmap;
        swa.event_mask =
            ExposureMask | KeyPressMask | KeyReleaseMask |
            PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
            StructureNotifyMask | SubstructureNotifyMask;

        win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                            vi->depth, InputOutput, vi->visual,
                            CWColormap | CWEventMask, &swa);
        XMapWindow(dpy, win);
        XStoreName(dpy, win, "Galaxy Overdrive");

        glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
        glXMakeCurrent(dpy, win, glc);
    }

    void cleanupXWindows() {
        glXMakeCurrent(dpy, None, NULL);
        glXDestroyContext(dpy, glc);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
    }

    void reshape(int w, int h) {
        g.xres = w; g.yres = h;
        g_xres = w; g_yres = h;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
        glOrtho(0, w, 0, h, -10, 1);
        XStoreName(dpy, win, "Galaxy Overdrive");
    }

    bool getXPending() { return XPending(dpy); }

    XEvent getXNextEvent() {
        XEvent e; XNextEvent(dpy, &e); return e;
    }

    void swapBuffers() { glXSwapBuffers(dpy, win); }

    void check_resize(XEvent *e) {
        if (e->type != ConfigureNotify) return;
        XConfigureEvent xce = e->xconfigure;
        if (xce.width != g.xres || xce.height != g.yres)
            reshape(xce.width, xce.height);
    }
} x11;

static void upload_texture(GLuint *tex, Image *img) {
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, img->width, img->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, img->data);
}

static Image img_back, img_logo;
static Image img_ship01, img_shipSlight, img_shipMid, img_shipVery;
static Image img_cannon, img_rockets, img_spaceGun, img_zapper;
static Image img_bCannon, img_bRocket, img_bSpaceGun, img_bZapper;
static Image img_health;

void init_opengl();
void check_mouse(XEvent *e);
int  check_keys(XEvent *e);
void physics(float dt);
void render();
void renderShip();
void renderBullets();
void title_render(const TitleAnim &t);
void title_physics(TitleAnim &t);
void renderStartButton();
void renderLevelIntro();
void renderPause();

int main()
{
    init_opengl();
    init_enemies();
    obstaclesInit();

    // hide everything during title
    obstaclesRemoveAllAsteroids();
    obstaclesRemoveAllMines();
    for (int i = 0; i < OBS_BARRIER_COUNT; i++) obs_barriers[i].active = false;
    obs_turret.active = false;
    obs_gate.active = false;

    int done = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int nframes = 0;
    time_t secTimer = time(NULL);

    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
        }

        clock_gettime(CLOCK_MONOTONIC, &t1);
        float dt = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9f;
        t0 = t1;
        if (dt > 0.05f) dt = 0.05f;
        g_time += dt;

        if (!g.paused) {
            physics(dt);
        }

        render();

        if (g.paused) {
            renderPause();
        }

        ++nframes;
        time_t now = time(NULL);
        if (now > secTimer) {
            secTimer = now;
            g.fps = nframes;
            nframes = 0;
        }

        x11.swapBuffers();
    }

    x11.cleanupXWindows();
    return 0;
}

void init_opengl()
{
    glViewport(0, 0, g.xres, g.yres);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    initialize_fonts();

    img_back.load("./images/Starfield08.png");
    img_logo.load("./images/galov.png");
    img_ship01.load("./ship_sprites/ship_full.png");
    img_shipSlight.load("./ship_sprites/ship_slight.png");
    img_shipMid.load("./ship_sprites/ship_mid.png");
    img_shipVery.load("./ship_sprites/ship_very.png");
    img_cannon.load("./weapons/cannon.png");
    img_rockets.load("./weapons/rockets.png");
    img_spaceGun.load("./weapons/spaceGun.png");
    img_zapper.load("./weapons/zapper.png");
    img_bCannon.load("./bullets/weapons_cannon.png");
    img_bRocket.load("./bullets/weapons_rocket.png");
    img_bSpaceGun.load("./bullets/weapons_spaceGun.png");
    img_bZapper.load("./bullets/weapons_zapper.png");
    img_health.load("./images/health.png");

    upload_texture(&g.tex.backTex,       &img_back);
    upload_texture(&g.tex.logoTex,       &img_logo);
    upload_texture(&g.tex.ship01Tex,     &img_ship01);
    upload_texture(&g.tex.shipSlightTex, &img_shipSlight);
    upload_texture(&g.tex.shipMidTex,    &img_shipMid);
    upload_texture(&g.tex.shipVeryTex,   &img_shipVery);
    upload_texture(&g.tex.cannonTex,     &img_cannon);
    upload_texture(&g.tex.rocketsTex,    &img_rockets);
    upload_texture(&g.tex.spaceGunTex,   &img_spaceGun);
    upload_texture(&g.tex.zapperTex,     &img_zapper);
    upload_texture(&g.tex.bulletCannonTex,   &img_bCannon);
    upload_texture(&g.tex.bulletRocketTex,   &img_bRocket);
    upload_texture(&g.tex.bulletSpaceGunTex, &img_bSpaceGun);
    upload_texture(&g.tex.bulletZapperTex,   &img_bZapper);
    upload_texture(&g.tex.healthTex, &img_health);


    g.tex.logoW = img_logo.width;
    g.tex.logoH = img_logo.height;

    g.tex.xc[0] = 0.0f; g.tex.xc[1] = 1.0f;
    g.tex.yc[0] = 0.0f; g.tex.yc[1] = 1.0f;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
}

void check_mouse(XEvent *e)
{
    if (e->type == MotionNotify) {
        g.mousex = e->xmotion.x;
        g.mousey = g.yres - e->xmotion.y;
    }
}

int check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease) return 0;

    int key = XLookupKeysym(&e->xkey, 0);

    if (key >= 0 && key < 512) {
        g.keys[key] = (e->type == KeyPress) ? 1 : 0;
    }

    if (e->type == KeyPress) {
        switch (key) {
            case XK_Escape: return 1;

            case XK_equal:
                g.ShipSpeed = fminf(g.ShipSpeed * 2.0f, 96.0f);
                break;

            case XK_space:
                g.spacePressed = 1;
                break;

            case XK_m:
                g.movSwitch = !g.movSwitch;
                break;

            case XK_k:

                // clear zapper
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (g.bullets[i].type == 3)
                        g.bullets[i].active = 0;
                }

                g.currentWeapon = (g.currentWeapon + 1) % 4;
                g.weaponFrame = 0;
                g.weaponTimer = 0.0f;
                break;

            case XK_r:
                if (g.state == STATE_PLAYING) {
                    obstaclesReset();
                    g.playerHP = 10;
                    g.score    = 0;
                }
                break;
            
            case XK_p:
                g.paused = !g.paused;
                break;
        }
    }

    if (e->type == KeyRelease && key == XK_space)
        g.spacePressed = 0;

    return 0;
}
void renderPause()
{
    glDisable(GL_TEXTURE_2D);

    // dark tint
    glColor4f(0, 0, 0, 0.5f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(g.xres, 0);
        glVertex2f(g.xres, g.yres);
        glVertex2f(0, g.yres);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    // text
    Rect r;
    r.bot = g.yres / 2;
    r.left = g.xres / 2;
    r.center = 1;

    ggprint16(&r, 0, 0x00ffffff, "PAUSED");
}

void title_physics(TitleAnim &t)
{
    if (t.timer < 1.0f) t.timer += 0.01f;
}

void title_render(const TitleAnim &t)
{
    float ease = t.timer * t.timer * (3.0f - 2.0f * t.timer);
    float maxW = g.xres * 0.6f;
    float maxH = maxW * ((float)g.tex.logoH / (float)g.tex.logoW);
    float w  = maxW * ease,  h  = maxH * ease;
    float cx = g.xres / 2.0f, cy = g.yres / 2.0f;

    glBindTexture(GL_TEXTURE_2D, g.tex.logoTex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex2f(cx - w/2, cy - h/2);
        glTexCoord2f(0,0); glVertex2f(cx - w/2, cy + h/2);
        glTexCoord2f(1,0); glVertex2f(cx + w/2, cy + h/2);
        glTexCoord2f(1,1); glVertex2f(cx + w/2, cy - h/2);
    glEnd();
}

void renderStartButton()
{
    if (!g.startBtn.visible) return;

    float x = g.startBtn.x;
    float y = g.startBtn.y;
    float w = g.startBtn.w;
    float h = g.startBtn.h;

    float pulse = 0.5f + 0.5f * sinf(g_time * 3.0f);

    glDisable(GL_TEXTURE_2D);

    glColor4f(0.1f, 0.4f, 0.8f, 0.85f);
    glBegin(GL_QUADS);
        glVertex2f(x - w/2, y - h/2);
        glVertex2f(x + w/2, y - h/2);
        glVertex2f(x + w/2, y + h/2);
        glVertex2f(x - w/2, y + h/2);
    glEnd();

    glLineWidth(3.0f);
    glColor4f(0.4f + 0.6f * pulse, 0.8f + 0.2f * pulse, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x - w/2, y - h/2);
        glVertex2f(x + w/2, y - h/2);
        glVertex2f(x + w/2, y + h/2);
        glVertex2f(x - w/2, y + h/2);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    Rect r;
    r.bot    = (int)(y - 6);
    r.left   = (int)x;
    r.center = 1;
    ggprint16(&r, 0, 0x00ffffff, "SHOOT TO START");
}

void renderLevelIntro()
{
    float cx = g.xres / 2.0f;
    float cy = g.yres / 2.0f;

    char buf[32];
    snprintf(buf, sizeof(buf), "LEVEL %d", g.currentLevel);

    Rect r;
    r.bot    = (int)(cy - 8);
    r.left   = (int)cx;
    r.center = 1;
    ggprint16(&r, 0, 0x00ffffff, buf);
}

static float getScaleX()
{
    return (float)g.xres / (float)VIRTUAL_W;
}

static float getScaleY()
{
    return (float)g.yres / (float)VIRTUAL_H;
}

static float getScale()
{
    return fminf(getScaleX(), getScaleY());
}

static const float FIRE_COOLDOWN       = 0.3f;
static const float ANIM_SPEED_MULT     = 2.0f;
static const float SHIP_COLLISION_RAD  = 22.0f;
static const float BULLET_COLLISION_RAD= 8.0f;

void physics(float dt)
{
    g.tex.yc[0] += 0.005f;
    g.tex.yc[1] += 0.005f;


   // g.health =

    if (g.state == STATE_TITLE) {
        title_physics(g.title);

        if (g.title.timer >= 1.0f && !g.startBtn.visible) {
            g.startBtn.visible = true;
            g.startBtn.x = g.xres / 2.0f;
            g.startBtn.y = g.yres * 0.18f;
        }
    }

    if (g.state == STATE_LEVEL_INTRO) {
        g.levelIntroTimer += dt;
        if (g.levelIntroTimer >= 2.0f) {
            g.state = STATE_PLAYING;
            g.levelIntroTimer = 0.0f;
            obstaclesReset();
        }
    }

    float rad    = g.shipAngle * (float)M_PI / 180.0f;
    float cosA   = cosf(rad), sinA = sinf(rad);
    float rotSpd = 600.0f * dt;
    float movSpd = g.ShipSpeed;

    float dx = 0, dy = 0;
    if (g.keys[XK_w]) { dx += cosA * movSpd; dy += sinA * movSpd; }
    if (g.keys[XK_s] && !g.movSwitch) { dx -= cosA * movSpd; dy -= sinA * movSpd; }
    if (g.keys[XK_a]) {
        if (g.movSwitch) { dx -= sinA * movSpd; dy += cosA * movSpd; }
        else g.shipAngle += rotSpd;
    }
    if (g.keys[XK_d]) {
        if (g.movSwitch) { dx += sinA * movSpd; dy -= cosA * movSpd; }
        else g.shipAngle -= rotSpd;
    }

    g.shipx += dx; g.shipy += dy;
    g.shipx = fmaxf(0, fminf(g.shipx, (float)g.xres));
    g.shipy = fmaxf(0, fminf(g.shipy, (float)g.yres));

    if (g.movSwitch) {
        float mx = g.mousex - g.shipx;
        float my = g.mousey - g.shipy;
        g.shipAngle = atan2f(my, mx) * 180.0f / (float)M_PI;
    }

    g.weaponTimer += dt;

    int maxFrames = (g.currentWeapon == 0) ? 7  :
                    (g.currentWeapon == 1) ? 16 :
                    (g.currentWeapon == 2) ? 12 : 14;

    if (g.spacePressed && g.currentWeapon != 3) {
        if (g.weaponTimer >= FIRE_COOLDOWN) {
            g.weaponTimer = 0.0f;
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!g.bullets[i].active) {
                    g.bullets[i].active    = 1;
                    g.bullets[i].type      = g.currentWeapon;
                    g.bullets[i].x         = g.shipx;
                    g.bullets[i].y         = g.shipy;
                    g.bullets[i].vel       = 10.0f;
                    g.bullets[i].frame     = 0;
                    g.bullets[i].frameTimer= 0.0f;
                    g.bullets[i].angle     = g.shipAngle;
                    float rb = g.shipAngle * (float)M_PI / 180.0f;
                    g.bullets[i].xVel = cosf(rb) * g.bullets[i].vel;
                    g.bullets[i].yVel = sinf(rb) * g.bullets[i].vel;
                    break;
                }
            }
            g.weaponFrame = (g.weaponFrame + (int)ANIM_SPEED_MULT) % maxFrames;
            if (g.weaponFrame == 0) g.weaponFrame = 1;
        }
    } else if (g.currentWeapon != 3) {
        g.weaponFrame = 0;
    }

    if (g.currentWeapon == 3) {
        if (g.spacePressed) {
            bool zapActive = false;
            for (int i = 0; i < MAX_BULLETS; i++)
                if (g.bullets[i].active && g.bullets[i].type == 3)
                    { zapActive = true; break; }

            if (!zapActive) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!g.bullets[i].active) {
                        g.bullets[i].active = 1; g.bullets[i].type = 3;
                        g.bullets[i].x = g.shipx; g.bullets[i].y = g.shipy;
                        g.bullets[i].angle = g.shipAngle;
                        g.bullets[i].frame = 0; g.bullets[i].frameTimer = 0;
                        g.bullets[i].xVel = 0; g.bullets[i].yVel = 0;
                        break;
                    }
                }
            }
            g.weaponFrame = (g.weaponFrame + 1) % maxFrames;
            if (g.weaponFrame == 0) g.weaponFrame = 1;
        } else {
            for (int i = 0; i < MAX_BULLETS; i++)
                if (g.bullets[i].type == 3) g.bullets[i].active = 0;
            g.weaponFrame = 0;
        }
    }

    float bulletAnimIntervals[4] = { 0.1f, 0.08f, 0.05f, 0.07f };
    int   bulletMaxFrames[4]     = { 4,    3,      10,    8 };

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g.bullets[i].active) continue;

        g.bullets[i].x += g.bullets[i].xVel;
        g.bullets[i].y += g.bullets[i].yVel;

        int t = g.bullets[i].type;
        if (t != 3 &&
            (g.bullets[i].x < -20 || g.bullets[i].x > g.xres + 20 ||
             g.bullets[i].y < -20 || g.bullets[i].y > g.yres + 20))
            g.bullets[i].active = 0;

        if (!g.bullets[i].active) continue;

        g.bullets[i].frameTimer += dt;
        if (g.bullets[i].frameTimer > bulletAnimIntervals[t]) {
            g.bullets[i].frameTimer = 0.0f;
            g.bullets[i].frame = (g.bullets[i].frame + 1) % bulletMaxFrames[t];
        }

        if (g.state == STATE_TITLE && g.startBtn.visible) {
            if (g.startBtn.contains(g.bullets[i].x, g.bullets[i].y)) {
                g.bullets[i].active = 0;
                g.startBtn.visible = false;
                g.state = STATE_LEVEL_INTRO;
                g.levelIntroTimer = 0.0f;
                continue;
            }
        }

        if (g.state == STATE_PLAYING) {
            int hit = obstaclesCheckBulletAsteroid(
                g.bullets[i].x, g.bullets[i].y, BULLET_COLLISION_RAD);
            if (hit >= 0) {
                g.bullets[i].active = 0;
                g.score += 10;
                continue;
            }

            int bHit = obstaclesCheckBulletBarrier(
                g.bullets[i].x, g.bullets[i].y, BULLET_COLLISION_RAD);
            if (bHit >= 0) {
                g.bullets[i].active = 0;
                continue;
            }

            int eHit = enemy_check_bullet_hit(
                g.bullets[i].x, g.bullets[i].y, BULLET_COLLISION_RAD);
            if (eHit >= 0) {
                g.bullets[i].active = 0;
                g.score += 15;
                continue;
            }
        }
    }

    if (g.state == STATE_PLAYING) {
        // asteroid and ship both damage and explode
        int astDmg = obstaclesCheckPlayerAsteroid(g.shipx, g.shipy, SHIP_COLLISION_RAD);
        if (astDmg > 0) {
            g.playerHP = fmaxf(0, g.playerHP - astDmg);
        }

        int mineHit = obstaclesCheckPlayerMine(g.shipx, g.shipy, SHIP_COLLISION_RAD);
        if (mineHit >= 0) {
            g.playerHP = fmaxf(0, g.playerHP - 2);
        }

        if (obstaclesCheckTurretHitsPlayer(g.shipx, g.shipy, SHIP_COLLISION_RAD)) {
            g.playerHP = fmaxf(0, g.playerHP - 1);
        }

        if (obstaclesCheckPlayerBarrier(g.shipx, g.shipy, SHIP_COLLISION_RAD) >= 0) {
            g.shipx -= dx; g.shipy -= dy;
        }

        // warp gate sends ship to random screen location
        float warpX, warpY;
        if (obstaclesCheckWarpGate(g.shipx, g.shipy, &warpX, &warpY)) {
            g.shipx = warpX;
            g.shipy = warpY;
            g.score += 50;
        }

        if (enemy_check_player_collision(g.shipx, g.shipy, SHIP_COLLISION_RAD)) {
            g.playerHP = fmaxf(0, g.playerHP - 1);
        }

        obstaclesUpdate(dt, g.shipx, g.shipy);

        g.spawnTimer += dt;
        if (g.spawnTimer >= g.spawnInterval) {
            g.spawnTimer = 0.0f;
            spawn_enemy(g.shipx, g.shipy, g.xres, g.yres);
        }
        enemies_physics(g.shipx, g.shipy, g.xres, g.yres, dt);
    }

    float speed = 5.0f; // higher = faster drain

    if (g.displayHP > g.playerHP) {

        g.displayHP -= speed * dt;
        if (g.displayHP < g.playerHP)
            g.displayHP = g.playerHP;
    }
    else if (g.displayHP < g.playerHP) {

        g.displayHP += speed * dt;
        if (g.displayHP > g.playerHP)
            g.displayHP = g.playerHP;
    }
}

void renderShip()
{
    float s = getScale();
    float w = 60.0f * s;
    float h = 60.0f * s;

    glColor4f(1, 1, 1, 1);

    glPushMatrix();
    glTranslatef(g.shipx, g.shipy, 0);
    glRotatef(g.shipAngle - 90.0f, 0, 0, 1);

    glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex2f(-w/2, -h/2);
        glTexCoord2f(0,0); glVertex2f(-w/2,  h/2);
        glTexCoord2f(1,0); glVertex2f( w/2,  h/2);
        glTexCoord2f(1,1); glVertex2f( w/2, -h/2);
    glEnd();
    glPopMatrix();

    GLuint wtex;
    float frameWidth;
    if      (g.currentWeapon == 0) { wtex = g.tex.cannonTex;   frameWidth = 1.0f/7.0f;  }
    else if (g.currentWeapon == 1) { wtex = g.tex.rocketsTex;  frameWidth = 1.0f/16.0f; }
    else if (g.currentWeapon == 2) { wtex = g.tex.spaceGunTex; frameWidth = 1.0f/12.0f; }
    else                           { wtex = g.tex.zapperTex;   frameWidth = 1.0f/14.0f; }

    float tx0 = frameWidth * (float)g.weaponFrame;
    float tx1 = tx0 + frameWidth;
    float ww = 40.0f * s, hh = 40.0f * s;

    glPushMatrix();
    glTranslatef(g.shipx, g.shipy, 0);
    glRotatef(g.shipAngle - 90.0f, 0, 0, 1);
    glBindTexture(GL_TEXTURE_2D, wtex);
    glBegin(GL_QUADS);
        glTexCoord2f(tx0,1); glVertex2f(-ww/2, -hh/2);
        glTexCoord2f(tx0,0); glVertex2f(-ww/2,  hh/2);
        glTexCoord2f(tx1,0); glVertex2f( ww/2,  hh/2);
        glTexCoord2f(tx1,1); glVertex2f( ww/2, -hh/2);
    glEnd();
    glPopMatrix();
}

void renderBullets()
{
    glColor4f(1, 1, 1, 1);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g.bullets[i].active) continue;

        int t = g.bullets[i].type;
        GLuint tex;
        float  frameWidth;
        switch (t) {
            case 0:  tex = g.tex.bulletCannonTex;   frameWidth = 1.0f/4.0f;  break;
            case 1:  tex = g.tex.bulletRocketTex;   frameWidth = 1.0f/3.0f;  break;
            case 2:  tex = g.tex.bulletSpaceGunTex; frameWidth = 1.0f/10.0f; break;
            default: tex = g.tex.bulletZapperTex;   frameWidth = 1.0f/8.0f;  break;
        }

        float tx0 = frameWidth * g.bullets[i].frame;
        float tx1 = tx0 + frameWidth;
        float x = g.bullets[i].x, y = g.bullets[i].y;
        float s = getScale();
        float bw = 20.0f * s;
        float bh = 20.0f * s;

        glBindTexture(GL_TEXTURE_2D, tex);

        if (t == 1) {
            glPushMatrix();
            glTranslatef(x, y, 0);
            glRotatef(g.bullets[i].angle - 90.0f, 0, 0, 1);
            float spacing = 10.0f * s;
            for (int j = 0; j < 4; j++) {
                float off = (j - 1.5f) * spacing;
                glBegin(GL_QUADS);
                    glTexCoord2f(tx0,1); glVertex2f(off-bw, -bh-15);
                    glTexCoord2f(tx0,0); glVertex2f(off-bw,  bh);
                    glTexCoord2f(tx1,0); glVertex2f(off+bw,  bh);
                    glTexCoord2f(tx1,1); glVertex2f(off+bw, -bh-15);
                glEnd();
            }
            glPopMatrix();
        } else if (t == 3) {
            glPushMatrix();
            glTranslatef(g.shipx, g.shipy, 0);
            glRotatef(g.shipAngle - 90.0f, 0, 0, 1);

            float beamLen = sqrtf(g.xres * g.xres + g.yres * g.yres);
            float beamW   = 40.0f * s;
            
            glBegin(GL_QUADS);
                glTexCoord2f(tx0,1); glVertex2f(-beamW/2, 0);
                glTexCoord2f(tx0,0); glVertex2f(-beamW/2, beamLen);
                glTexCoord2f(tx1,0); glVertex2f( beamW/2, beamLen);
                glTexCoord2f(tx1,1); glVertex2f( beamW/2, 0);
            glEnd();
            glPopMatrix();
        } else {
            glPushMatrix();
            glTranslatef(x, y, 0);
            glRotatef(g.bullets[i].angle - 90.0f, 0, 0, 1);
            glBegin(GL_QUADS);
                glTexCoord2f(tx0,1); glVertex2f(-bw, -bh);
                glTexCoord2f(tx0,0); glVertex2f(-bw,  bh);
                glTexCoord2f(tx1,0); glVertex2f( bw,  bh);
                glTexCoord2f(tx1,1); glVertex2f( bw, -bh);
            glEnd();
            glPopMatrix();
        }
    }
}

void renderHealthBar()
{
    float maxHP = 10.0f;
    float target = g.playerHP;

    g.displayHP += (target - g.displayHP) * 0.1f;

    float hpRatio = (float)g.playerHP / maxHP;

    hpRatio = g.displayHP / maxHP;

    if (hpRatio < 0.0f) 
        hpRatio = 0.0f;
  
    float s = getScale();
    float w = 200.0f * s;
    float h = 50.0f * s;

    float x = g.xres - w - (20.0f * s);
    float y = 20.0f * s;

    float split = 0.57f; 

    float contTexY0 = 0.0f;
    float contTexY1 = split;

    float barTexY0 = split;
    float barTexY1 = 1.0f;

    float yOffset = -4.5f; // move bar
    float barOffset = w * 0.3f; 

    glBindTexture(GL_TEXTURE_2D, g.tex.healthTex);
    glColor4f(1,1,1,1);

    // bar
    glBegin(GL_QUADS);
        glTexCoord2f(0.4f, barTexY1); glVertex2f(x + barOffset, y + yOffset);
        glTexCoord2f(0.4f, barTexY0); glVertex2f(x + barOffset, y + h + yOffset);

        glTexCoord2f(0.4f + (0.5f * (g.displayHP / maxHP)), barTexY0);
        glVertex2f(x + barOffset + (w * 0.6f / maxHP) * g.displayHP, y + h + yOffset);

        glTexCoord2f(0.4f + (0.5f * (g.displayHP / maxHP)), barTexY1);
        glVertex2f(x + barOffset + (w * 0.6f / maxHP) * g.displayHP, y + yOffset);
    glEnd();

    // container
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, contTexY1); glVertex2f(x, y);
        glTexCoord2f(0.0f, contTexY0); glVertex2f(x, y + h);
        glTexCoord2f(1.0f, contTexY0); glVertex2f(x + w, y + h);
        glTexCoord2f(1.0f, contTexY1); glVertex2f(x + w, y);
    glEnd();

    char hpText[16];
    snprintf(hpText, sizeof(hpText), "%d", g.playerHP);

    Rect r;
    r.bot = (int)(31.8f * s);
    r.left = (int)(g.xres - 96 * s);     
    r.center = 1;
    ggprint16(&r, 0, 0x00ffffff, hpText);

}

static void renderHUD()
{
    Rect r;
    r.bot    = g.yres - 20;
    r.left   = 10;
    r.center = 0;

    ggprint12(&r, 2, 0x00ffffff, "K - switch weapon");  r.bot -= 20;
    ggprint12(&r, 2, 0x00ffffff, "Spacebar - shoot");    r.bot -= 20;
    ggprint12(&r, 2, 0x00ffffff, "WASD- move");    r.bot -= 20;
    ggprint12(&r, 2, 0x00ffffff, "M - alt movement W + Mouse");   r.bot -= 20;

    if (g.state == STATE_PLAYING) {
        ggprint12(&r, 2, 0x00ffffff, "R - reset obstacles"); r.bot -= 20;
    }

    ggprint12(&r, 2, 0x00ffffff, "fps: %i", g.fps);      r.bot -= 20;

    if (g.state == STATE_PLAYING) {
      //  ggprint12(&r, 2, 0x00ffffff, "HP: %i", g.playerHP);  r.bot -= 20;
        ggprint12(&r, 2, 0x00ffffff, "Score: %i", g.score);
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1, 1, 1, 1);

    glBindTexture(GL_TEXTURE_2D, g.tex.backTex);
    glBegin(GL_QUADS);
        glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0,       0);
        glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0,       g.yres);
        glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres,  g.yres);
        glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres,  0);
    glEnd();

    if (g.state == STATE_TITLE) {
        title_render(g.title);
        renderStartButton();
    }

    if (g.state == STATE_PLAYING) {
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);
        obstaclesDraw();
        glPopAttrib();

        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        render_enemies();
    }

    glColor4f(1, 1, 1, 1);
    renderShip();
    renderBullets();

    if (g.state == STATE_LEVEL_INTRO) {
        renderLevelIntro();
    }

    if (g.state == STATE_PLAYING) 
        renderHealthBar();
    
    renderHUD();
}
