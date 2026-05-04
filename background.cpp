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
float g_time = 0.0f;

#include "obstacles.cpp"

#define MAX_BULLETS 50
#define POWERUP_COUNT 10

const int VIRTUAL_W = 640;
const int VIRTUAL_H = 480;
const float TARGET_FPS = 30.0f;
const float FRAME_TIME = 1.0f / TARGET_FPS; // target frame time

enum GameState {
    STATE_TITLE,
    STATE_LEVEL_INTRO,
    STATE_PLAYING,
    STATE_POWERUP,
    STATE_LEVEL_COMPLETE,
    STATE_GAME_WON,
    STATE_DEAD
};

class Image {
public:
    int width, height;
    unsigned char *data;

    Image() : width(0), height(0), data(NULL) {}

    ~Image() {
        delete [] data;
    }

    void load(const char *fname) {
        if (!fname || fname[0] == '\0')
            return;

        char name[256];
        strncpy(name, fname, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';

        int slen = (int)strlen(name);
        if (slen >= 4)
            name[slen - 4] = '\0';

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

        data = new unsigned char[width * height * 4];

        for (int i = 0; i < width * height; i++) {
            unsigned char r = (unsigned char)fgetc(fpi);
            unsigned char g = (unsigned char)fgetc(fpi);
            unsigned char b = (unsigned char)fgetc(fpi);

            data[i * 4]     = r;
            data[i * 4 + 1] = g;
            data[i * 4 + 2] = b;
            data[i * 4 + 3] = (r < 20 && g < 20 && b < 20) ? 0 : 255;
        }

        fclose(fpi);
        unlink(ppmname);
    }
};

struct Texture {
    GLuint backTex, logoTex, ship01Tex, shipSlightTex, shipMidTex, shipVeryTex;
    GLuint cannonTex, rocketsTex, spaceGunTex, zapperTex;
    GLuint bulletCannonTex, bulletRocketTex, bulletSpaceGunTex, bulletZapperTex;
    GLuint raysTex, healthTex;

    int logoW, logoH;
    float xc[2], yc[2];
};

struct Bullet {
    float x, y;
    float xVel, yVel;
    float vel;
    int active, damage, type, frame;
    float frameTimer, angle;
    int bounces;
};

struct Powerups {
    int fireRateLevel, speedLevel, damageLevel;
    bool homing, pierce;
};

struct TitleAnim {
    float timer;

    TitleAnim() : timer(0) {}
};

struct StartButton {
    float x, y, w, h;
    bool visible;

    StartButton() : x(0), y(0), w(200), h(50), visible(false) {}

    bool contains(float px, float py) const {
        return visible &&
               px >= x - w / 2 && px <= x + w / 2 &&
               py >= y - h / 2 && py <= y + h / 2;
    }
};

class Global {
public:
    int xres, yres;
    Texture tex;
    TitleAnim title;

    int mousex, mousey, spacePressed, movSwitch, currentWeapon;
    int weaponFrame;
    float weaponTimer;

    float shipx, shipy, ShipSpeed, shipAngle;
    int fps, paused;

    float powerupFill[2];
    int selectedPowerup;

    int keys[512];
    Bullet bullets[MAX_BULLETS];

    int playerHP;
    float spawnTimer, spawnInterval;

    GameState state;
    float levelIntroTimer;
    int currentLevel;

    StartButton startBtn;
    float displayHP;

    Powerups powerups;

    int availablePowerups[16];
    int availableCount;
    int currentChoices[2];

    Global()
        : xres(640), yres(480),
          mousex(320), mousey(240),
          spacePressed(0), movSwitch(1), currentWeapon(0),
          weaponFrame(0), weaponTimer(0),
          shipx(320), shipy(160),
          ShipSpeed(6), shipAngle(0),
          fps(0), playerHP(10),
          spawnTimer(0), spawnInterval(2),
          state(STATE_TITLE), levelIntroTimer(0), currentLevel(1)
    {
        memset(keys, 0, sizeof(keys));

        for (int i = 0; i < MAX_BULLETS; i++)
            bullets[i].active = 0;

        displayHP = playerHP;
        paused = 0;

        powerupFill[0] = powerupFill[1] = 0;
        selectedPowerup = -1;

        powerups.fireRateLevel = 0;
        powerups.speedLevel = 0;
        powerups.damageLevel = 0;
        powerups.homing = false;
        powerups.pierce = false;

        availableCount = 0;
    }
} g;

// levels.h needs g and gamestate to exist first
#include "levels.h"

class X11_wrapper {
    Display *dpy;
    Window win;
    GLXContext glc;

public:
    X11_wrapper() {
        GLint att[] = {
            GLX_RGBA,
            GLX_DEPTH_SIZE,
            24,
            GLX_DOUBLEBUFFER,
            None
        };

        dpy = XOpenDisplay(NULL);
        if (!dpy) {
            printf("Cannot connect to X server\n");
            exit(1);
        }

        Window root = DefaultRootWindow(dpy);

        XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
        if (!vi) {
            printf("No appropriate visual\n");
            exit(1);
        }

        Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

        XSetWindowAttributes swa;
        swa.colormap = cmap;
        swa.event_mask =
            ExposureMask |
            KeyPressMask |
            KeyReleaseMask |
            PointerMotionMask |
            ButtonPressMask |
            ButtonReleaseMask |
            StructureNotifyMask |
            SubstructureNotifyMask;

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
        g.xres = w;
        g.yres = h;
        g_xres = w;
        g_yres = h;

        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glOrtho(0, w, 0, h, -10, 1);
        XStoreName(dpy, win, "Galaxy Overdrive");
    }

    bool getXPending() {
        return XPending(dpy);
    }

    XEvent getXNextEvent() {
        XEvent e;
        XNextEvent(dpy, &e);
        return e;
    }

    void swapBuffers() {
        glXSwapBuffers(dpy, win);
    }

    void check_resize(XEvent *e) {
        if (e->type != ConfigureNotify)
            return;

        XConfigureEvent xce = e->xconfigure;

        if (xce.width != g.xres || xce.height != g.yres)
            reshape(xce.width, xce.height);
    }
} x11;

static void upload_texture(GLuint *tex, Image *img)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, 4,
                 img->width, img->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
}

static void upload_alpha_texture(GLuint *tex, const char *fname)
{
    int w = 0, h = 0;
    unsigned char *data = NULL;

    load_png_with_alpha(fname, &w, &h, &data);

    if (data) {
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     w, h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        delete [] data;
    }
}

static Image img_back, img_logo;
static Image img_ship01, img_shipSlight, img_shipMid, img_shipVery;
static Image img_cannon, img_rockets, img_spaceGun, img_zapper;
static Image img_bCannon, img_bRocket, img_bSpaceGun, img_bZapper;

void init_opengl();
void check_mouse(XEvent *);
int check_keys(XEvent *);
void physics(float);
void render();
void renderShip();
void renderShipBreakup();
void renderBullets();
void title_render(const TitleAnim &);
void title_physics(TitleAnim &);
void renderStartButton();
void renderLevelIntro();
void renderPause();
void initPowerups();
void generatePowerups();
void pickPowerup(int);
void levelsRenderGameWon();

int main()
{
    init_opengl();
    init_enemies();
    obstaclesInit();
    initPowerups();
    levelsInit(); // initialize level system

    // title screen starts with only asteroids visible
    obstaclesRemoveAllMines();

    for (int i = 0; i < OBS_BARRIER_COUNT; i++)
        obs_barriers[i].active = false;

    obs_gate.active = false;
    obstaclesSpawnTitleAsteroids();

    int done = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int nframes = 0;
    time_t secTimer = time(NULL);

    while (!done) {
        struct timespec frameStart;
        clock_gettime(CLOCK_MONOTONIC, &frameStart);

        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
        }

        clock_gettime(CLOCK_MONOTONIC, &t1);

        float dt = (t1.tv_sec - t0.tv_sec) +
                   (t1.tv_nsec - t0.tv_nsec) * 1e-9f;

        t0 = t1;

        if (dt > 0.05f)
            dt = 0.05f;

        g_time += dt;

        if (!g.paused)
            physics(dt);

        render();

        if (g.paused &&
            g.state != STATE_DEAD &&
            g.state != STATE_LEVEL_COMPLETE &&
            g.state != STATE_GAME_WON) {
            renderPause();
        }

        ++nframes;

        time_t now = time(NULL);
        if (now > secTimer) {
            secTimer = now;
            g.fps = nframes;
            nframes = 0;
        }

        struct timespec frameEnd;
        clock_gettime(CLOCK_MONOTONIC, &frameEnd);

        float frameDuration =
            (frameEnd.tv_sec - frameStart.tv_sec) +
            (frameEnd.tv_nsec - frameStart.tv_nsec) * 1e-9f;

        if (frameDuration < FRAME_TIME) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = (long)((FRAME_TIME - frameDuration) * 1e9);
            nanosleep(&ts, NULL);
        }

        x11.swapBuffers();
    }

    x11.cleanupXWindows();
    return 0;
}

void init_opengl()
{
    glViewport(0, 0, g.xres, g.yres);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    glClearColor(0, 0, 0, 1);

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

    upload_texture(&g.tex.backTex, &img_back);
    upload_texture(&g.tex.logoTex, &img_logo);

    upload_texture(&g.tex.ship01Tex, &img_ship01);
    upload_texture(&g.tex.shipSlightTex, &img_shipSlight);
    upload_texture(&g.tex.shipMidTex, &img_shipMid);
    upload_texture(&g.tex.shipVeryTex, &img_shipVery);

    upload_texture(&g.tex.cannonTex, &img_cannon);
    upload_texture(&g.tex.rocketsTex, &img_rockets);
    upload_texture(&g.tex.spaceGunTex, &img_spaceGun);
    upload_texture(&g.tex.zapperTex, &img_zapper);

    upload_texture(&g.tex.bulletCannonTex, &img_bCannon);
    upload_texture(&g.tex.bulletRocketTex, &img_bRocket);
    upload_texture(&g.tex.bulletSpaceGunTex, &img_bSpaceGun);
    upload_texture(&g.tex.bulletZapperTex, &img_bZapper);

    upload_alpha_texture(&g.tex.healthTex, "./images/health.png");

    g.tex.logoW = img_logo.width;
    g.tex.logoH = img_logo.height;

    g.tex.xc[0] = 0;
    g.tex.xc[1] = 1;
    g.tex.yc[0] = 0;
    g.tex.yc[1] = 1;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
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
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;

    int key = XLookupKeysym(&e->xkey, 0);

    if (key >= 0 && key < 512)
        g.keys[key] = (e->type == KeyPress) ? 1 : 0;

    if (e->type == KeyPress) {
        // let level screens handle their own keys first
        if (levelsHandleKey(key))
            return 0;

        switch (key) {
            case XK_Escape:
                return 1;

            case XK_equal:
                g.ShipSpeed = fminf(g.ShipSpeed * 2, 96);
                break;

            case XK_space:
                g.spacePressed = 1;
                break;

            case XK_m:
                g.movSwitch = !g.movSwitch;
                break;

            case XK_k:
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (g.bullets[i].type == 3)
                        g.bullets[i].active = 0;
                }

                g.currentWeapon = (g.currentWeapon + 1) % 4;
                g.weaponFrame = 0;
                g.weaponTimer = 0;
                break;

            case XK_r:
                if (g.state == STATE_PLAYING) {
                    obstaclesReset();
                    g.playerHP = 10;
                }
                break;

            case XK_p:
                if (g.state != STATE_DEAD &&
                    g.state != STATE_LEVEL_COMPLETE &&
                    g.state != STATE_GAME_WON) {
                    g.paused = !g.paused;

                    if (!g.paused)
                        lv_selectOpen = false;
                }
                break;
        }
    }

    if (e->type == KeyRelease && key == XK_space)
        g.spacePressed = 0;

    return 0;
}

// pause overlay with level-select hint
void renderPause()
{
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.52f);

    glBegin(GL_QUADS);
    glVertex2f(0,      0);
    glVertex2f(g.xres, 0);
    glVertex2f(g.xres, g.yres);
    glVertex2f(0,      g.yres);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    float pw = 340;
    float ph = 200;
    float px = (g.xres - pw) * 0.5f;
    float py = (g.yres - ph) * 0.5f;

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.03f, 0.06f, 0.14f, 0.92f);

    glBegin(GL_QUADS);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    float pulse = 0.5f + 0.5f * sinf(g_time * 2.5f);

    glLineWidth(2);
    glColor4f(0.25f + 0.4f * pulse,
              0.55f + 0.3f * pulse,
              1,
              0.9f);

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

    r.bot = (int)(py + ph - 40);
    ggprint16(&r, 0, 0x00ffffff, "PAUSED");

    r.bot -= 30;
    ggprint12(&r, 0, 0x0088ffff, "%s  -  %s",
              LEVEL_DEFS[lv_current].title,
              LEVEL_DEFS[lv_current].subtitle);

    r.bot -= 28;
    ggprint12(&r, 0, 0x00aaaaaa, "P  -  Resume");

    r.bot -= 20;
    ggprint12(&r, 0, 0x00aaaaaa, "N  -  Next Level");

    r.bot -= 20;
    ggprint12(&r, 0, 0x00aaaaaa, "1-6  -  Jump to Level");

    r.bot -= 20;
    ggprint12(&r, 0, 0x00aaaaaa, "Esc  -  Quit");

    levelsRenderSelectMenu();
}

void title_physics(TitleAnim &t)
{
    if (t.timer < 1)
        t.timer += 0.01f;
}

void title_render(const TitleAnim &t)
{
    float ease = t.timer * t.timer * (3 - 2 * t.timer);
    float maxW = g.xres * 0.6f;
    float maxH = maxW * ((float)g.tex.logoH / (float)g.tex.logoW);
    float w = maxW * ease;
    float h = maxH * ease;
    float cx = g.xres / 2.0f;
    float cy = g.yres / 2.0f;

    glBindTexture(GL_TEXTURE_2D, g.tex.logoTex);
    glColor4f(1, 1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(cx - w / 2, cy - h / 2);
    glTexCoord2f(0, 0); glVertex2f(cx - w / 2, cy + h / 2);
    glTexCoord2f(1, 0); glVertex2f(cx + w / 2, cy + h / 2);
    glTexCoord2f(1, 1); glVertex2f(cx + w / 2, cy - h / 2);
    glEnd();
}

void renderStartButton()
{
    if (!g.startBtn.visible)
        return;

    float x = g.startBtn.x;
    float y = g.startBtn.y;
    float w = g.startBtn.w;
    float h = g.startBtn.h;

    float pulse = 0.5f + 0.5f * sinf(g_time * 3);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.1f, 0.4f, 0.8f, 0.85f);

    glBegin(GL_QUADS);
    glVertex2f(x - w / 2, y - h / 2);
    glVertex2f(x + w / 2, y - h / 2);
    glVertex2f(x + w / 2, y + h / 2);
    glVertex2f(x - w / 2, y + h / 2);
    glEnd();

    glLineWidth(3);
    glColor4f(0.4f + 0.6f * pulse,
              0.8f + 0.2f * pulse,
              1,
              1);

    glBegin(GL_LINE_LOOP);
    glVertex2f(x - w / 2, y - h / 2);
    glVertex2f(x + w / 2, y - h / 2);
    glVertex2f(x + w / 2, y + h / 2);
    glVertex2f(x - w / 2, y + h / 2);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    Rect r;
    r.bot = (int)(y - 6);
    r.left = (int)x;
    r.center = 1;

    ggprint16(&r, 0, 0x00ffffff, "SHOOT TO START");
}

void renderLevelIntro()
{
    float cx = g.xres / 2.0f;
    float cy = g.yres / 2.0f;

    const LevelDesc &ld = LEVEL_DEFS[lv_current];

    Rect r;
    r.center = 1;
    r.left = (int)cx;

    r.bot = (int)(cy + 16);
    ggprint16(&r, 0, 0x00ffffff, "%s", ld.title);

    r.bot = (int)(cy - 16);
    ggprint12(&r, 0, 0x0088ffff, "%s", ld.subtitle);
}

static float getScale()
{
    return fminf((float)g.xres / 640.0f,
                 (float)g.yres / 480.0f);
}

static const float FIRE_COOLDOWN        = 0.5f;
static const float ANIM_SPEED_MULT      = 2.0f;
static const float SHIP_COLLISION_RAD   = 22.0f;
static const float BULLET_COLLISION_RAD = 8.0f;

const char* POWERUPS[] = {
    "Fire Rate+",
    "Fire Rate++",
    "Speed+",
    "Speed++",
    "Damage+",
    "Damage++",
    "Homing Tech",
    "Piercing Tech",
    "Space Gun",
    "Rockets"
};

void initPowerups()
{
    g.availableCount = POWERUP_COUNT;

    for (int i = 0; i < POWERUP_COUNT; i++)
        g.availablePowerups[i] = i;
}

bool isPowerupUnlocked(int id)
{
    switch (id) {
        case 1:
            return g.powerups.fireRateLevel >= 1;

        case 3:
            return g.powerups.speedLevel >= 1;

        case 5:
            return g.powerups.damageLevel >= 1;
    }

    return true;
}

void generatePowerups()
{
    int validPool[16];
    int validCount = 0;

    for (int i = 0; i < g.availableCount; i++) {
        int id = g.availablePowerups[i];

        if (isPowerupUnlocked(id))
            validPool[validCount++] = id;
    }

    if (validCount == 0) {
        g.currentChoices[0] = g.currentChoices[1] = -1;
        return;
    }

    if (validCount == 1) {
        g.currentChoices[0] = g.currentChoices[1] = validPool[0];
        return;
    }

    for (int i = 0; i < 2; i++) {
        int r = rand() % validCount;
        g.currentChoices[i] = validPool[r];
        validPool[r] = validPool[--validCount];
    }
}

void pickPowerup(int ci)
{
    int id = g.currentChoices[ci];

    switch (id) {
        case 0:
            g.powerups.fireRateLevel += 1;
            break;

        case 1:
            g.powerups.fireRateLevel += 2;
            break;

        case 2:
            g.powerups.speedLevel += 1;
            break;

        case 3:
            g.powerups.speedLevel += 2;
            break;

        case 4:
            g.powerups.damageLevel += 1;
            break;

        case 5:
            g.powerups.damageLevel += 2;
            break;

        case 6:
            g.powerups.homing = true;
            break;

        case 7:
            g.powerups.pierce = true;
            break;

        case 8:
            g.currentWeapon = 2;
            break;

        case 9:
            g.currentWeapon = 1;
            break;
    }

    for (int i = 0; i < g.availableCount; i++) {
        if (g.availablePowerups[i] == id) {
            g.availablePowerups[i] = g.availablePowerups[--g.availableCount];
            break;
        }
    }
}

void physics(float dt)
{
    g.tex.yc[0] += 0.005f;
    g.tex.yc[1] += 0.005f;

    if (g.state == STATE_TITLE) {
        title_physics(g.title);
        obstaclesUpdateAsteroidsOnly(dt);

        if (g.title.timer >= 1 && !g.startBtn.visible) {
            g.startBtn.visible = true;
            g.startBtn.x = g.xres / 2.0f;
            g.startBtn.y = g.yres * 0.18f;
        }
    }

    if (g.state == STATE_LEVEL_INTRO) {
        g.levelIntroTimer += dt;

        if (g.levelIntroTimer >= 2) {
            g.playerHP = 10;
            g.displayHP = 10;
            g.state = STATE_PLAYING;
            g.levelIntroTimer = 0;

            lv_loadLevel(lv_current);
        }
    }

    if (g.state == STATE_LEVEL_COMPLETE) {
        lv_completeTimer += dt;

        if (lv_completeTimer >= 3) {
            for (int i = 0; i < MAX_BULLETS; i++)
                g.bullets[i].active = 0;

            g.powerupFill[0] = g.powerupFill[1] = 0;

            if (lv_current >= TOTAL_LEVELS - 1) {
                obstaclesRemoveAllAsteroids();
                obstaclesRemoveAllMines();

                for (int i = 0; i < OBS_BARRIER_COUNT; i++)
                    obs_barriers[i].active = false;

                obs_gate.active = false;
                g.state = STATE_GAME_WON;
            } else {
                generatePowerups();
                obstaclesReset();
                g.state = STATE_POWERUP;
            }
        }
    }

    if (g.state == STATE_DEAD)
        levelsUpdateDeath(dt);

    float rad = g.shipAngle * (float)M_PI / 180;
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    float rotSpd = 600 * dt;
    float movSpd = g.ShipSpeed * (1 + 0.3f * g.powerups.speedLevel);
    float fireCooldown = FIRE_COOLDOWN / (1 + 0.25f * g.powerups.fireRateLevel);

    float dx = 0;
    float dy = 0;

    float mx = g.mousex - g.shipx;
    float my = g.mousey - g.shipy;
    float distSq = mx * mx + my * my;

    if (g.keys[XK_w]) {
        if (!g.movSwitch || distSq > 25) {
            dx += cosA * movSpd;
            dy += sinA * movSpd;
        }
    }

    if (g.keys[XK_s] && !g.movSwitch) {
        dx -= cosA * movSpd;
        dy -= sinA * movSpd;
    }

    if (g.keys[XK_a])
        g.shipAngle += rotSpd;

    if (g.keys[XK_d])
        g.shipAngle -= rotSpd;

    g.shipx += dx;
    g.shipy += dy;

    g.shipx = fmaxf(0, fminf(g.shipx, (float)g.xres));
    g.shipy = fmaxf(0, fminf(g.shipy, (float)g.yres));

    if (g.movSwitch && distSq > 4)
        g.shipAngle = atan2f(my, mx) * 180 / (float)M_PI;

    g.weaponTimer += dt;

    int maxFrames =
        (g.currentWeapon == 0) ? 7 :
        (g.currentWeapon == 1) ? 16 :
        (g.currentWeapon == 2) ? 12 : 14;

    if (g.spacePressed && g.currentWeapon != 3) {
        if (g.weaponTimer >= fireCooldown) {
            g.weaponTimer = 0;

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!g.bullets[i].active) {
                    g.bullets[i].active = 1;
                    g.bullets[i].type = g.currentWeapon;
                    g.bullets[i].damage = 1 + g.powerups.damageLevel;
                    g.bullets[i].x = g.shipx;
                    g.bullets[i].y = g.shipy;
                    g.bullets[i].vel = 10;
                    g.bullets[i].frame = 0;
                    g.bullets[i].frameTimer = 0;
                    g.bullets[i].angle = g.shipAngle;
                    g.bullets[i].bounces = 0;

                    float rb = g.shipAngle * (float)M_PI / 180;
                    g.bullets[i].xVel = cosf(rb) * g.bullets[i].vel;
                    g.bullets[i].yVel = sinf(rb) * g.bullets[i].vel;

                    break;
                }
            }

            g.weaponFrame = (g.weaponFrame + (int)ANIM_SPEED_MULT) % maxFrames;

            if (g.weaponFrame == 0)
                g.weaponFrame = 1;
        }
    } else if (g.currentWeapon != 3) {
        g.weaponFrame = 0;
    }

    if (g.currentWeapon == 3) {
        if (g.spacePressed) {
            bool z = false;

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (g.bullets[i].active && g.bullets[i].type == 3) {
                    z = true;
                    break;
                }
            }

            if (!z) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!g.bullets[i].active) {
                        g.bullets[i].active = 1;
                        g.bullets[i].type = 3;
                        g.bullets[i].x = g.shipx;
                        g.bullets[i].y = g.shipy;
                        g.bullets[i].angle = g.shipAngle;
                        g.bullets[i].frame = 0;
                        g.bullets[i].frameTimer = 0;
                        g.bullets[i].xVel = g.bullets[i].yVel = 0;
                        break;
                    }
                }
            }

            g.weaponFrame = (g.weaponFrame + 1) % maxFrames;

            if (g.weaponFrame == 0)
                g.weaponFrame = 1;
        } else {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (g.bullets[i].type == 3)
                    g.bullets[i].active = 0;
            }

            g.weaponFrame = 0;
        }
    }

    float bai[4] = { 0.1f, 0.08f, 0.05f, 0.07f };
    int bmf[4] = { 4, 3, 10, 8 };

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g.bullets[i].active)
            continue;

        g.bullets[i].x += g.bullets[i].xVel;
        g.bullets[i].y += g.bullets[i].yVel;

        bool onScreen =
            g.bullets[i].x >= 0 &&
            g.bullets[i].x <= g.xres &&
            g.bullets[i].y >= 0 &&
            g.bullets[i].y <= g.yres;

        if (g.powerups.homing && g.bullets[i].type != 3 && onScreen) {
            float tx, ty;

            if (find_nearest_enemy(g.bullets[i].x, g.bullets[i].y, tx, ty) >= 0) {
                float ca = atan2f(g.bullets[i].yVel, g.bullets[i].xVel);
                float ta = atan2f(ty - g.bullets[i].y, tx - g.bullets[i].x);
                float diff = ta - ca;

                while (diff > M_PI)
                    diff -= 2 * M_PI;

                while (diff < -M_PI)
                    diff += 2 * M_PI;

                float ts2 = 0.05f;

                if (diff > ts2)
                    diff = ts2;

                if (diff < -ts2)
                    diff = -ts2;

                float na = ca + diff;
                float spd = g.bullets[i].vel;

                g.bullets[i].xVel = cosf(na) * spd;
                g.bullets[i].yVel = sinf(na) * spd;
                g.bullets[i].angle = na * 180 / M_PI;
            }
        }

        int t = g.bullets[i].type;

        if (t != 3 &&
            (g.bullets[i].x < -20 ||
             g.bullets[i].x > g.xres + 20 ||
             g.bullets[i].y < -20 ||
             g.bullets[i].y > g.yres + 20)) {
            g.bullets[i].active = 0;
        }

        if (!g.bullets[i].active)
            continue;

        g.bullets[i].frameTimer += dt;

        if (g.bullets[i].frameTimer > bai[t]) {
            g.bullets[i].frameTimer = 0;
            g.bullets[i].frame = (g.bullets[i].frame + 1) % bmf[t];
        }

        if (g.state == STATE_TITLE && g.startBtn.visible) {
            if (g.startBtn.contains(g.bullets[i].x, g.bullets[i].y)) {
                g.bullets[i].active = 0;
                g.startBtn.visible = false;
                g.state = STATE_LEVEL_INTRO;
                g.levelIntroTimer = 0;

                lv_current = 0;
                continue;
            }
        }

        if (g.state == STATE_PLAYING) {
            int hit = obstaclesCheckBulletAsteroid(
                g.bullets[i].x,
                g.bullets[i].y,
                BULLET_COLLISION_RAD);

            if (hit >= 0) {
                if (!g.powerups.pierce)
                    g.bullets[i].active = 0;

                continue;
            }

            int bHit = obstaclesCheckBulletBarrier(
                g.bullets[i].x,
                g.bullets[i].y,
                BULLET_COLLISION_RAD);

            if (bHit >= 0) {
                if (!g.powerups.pierce)
                    g.bullets[i].active = 0;

                continue;
            }

            int eHit = enemy_check_bullet_hit(
                g.bullets[i].x,
                g.bullets[i].y,
                BULLET_COLLISION_RAD,
                g.currentWeapon);

            if (eHit >= 0) {
                if (g.bullets[i].type == 2) {
                    g.bullets[i].bounces++;

                    if (g.bullets[i].bounces >= 3) {
                        g.bullets[i].active = 0;
                    } else {
                        g.bullets[i].xVel *= -1;
                        g.bullets[i].yVel *= -1;

                        float ang = atan2f(g.bullets[i].yVel, g.bullets[i].xVel)
                                  + ((rand() % 21) - 10) * 0.02f;

                        float spd = g.bullets[i].vel;

                        g.bullets[i].xVel = cosf(ang) * spd;
                        g.bullets[i].yVel = sinf(ang) * spd;

                        g.bullets[i].x += g.bullets[i].xVel * 2;
                        g.bullets[i].y += g.bullets[i].yVel * 2;
                    }
                } else {
                    if (!g.powerups.pierce)
                        g.bullets[i].active = 0;
                }

                if (eHit == 1)
                    levelsOnEnemyKilled();

                continue;
            }
        }
    }

    if (g.state == STATE_PLAYING) {
        int ad = obstaclesCheckPlayerAsteroid(
            g.shipx,
            g.shipy,
            SHIP_COLLISION_RAD);

        if (ad > 0)
            g.playerHP = fmaxf(0, g.playerHP - ad);

        int mh = obstaclesCheckPlayerMine(
            g.shipx,
            g.shipy,
            SHIP_COLLISION_RAD);

        if (mh >= 0)
            g.playerHP = fmaxf(0, g.playerHP - 2);

        if (obstaclesCheckPlayerBarrier(g.shipx, g.shipy, SHIP_COLLISION_RAD) >= 0) {
            g.shipx -= dx;
            g.shipy -= dy;
        }

        float wX, wY;

        if (obstaclesCheckWarpGate(g.shipx, g.shipy, &wX, &wY)) {
            g.shipx = wX;
            g.shipy = wY;
        }

        if (enemy_check_player_collision(g.shipx, g.shipy, SHIP_COLLISION_RAD))
            g.playerHP = fmaxf(0, g.playerHP - 1);

        obstaclesUpdate(dt, g.shipx, g.shipy);
        levelsUpdate(dt);

        if (g.playerHP <= 0 && g.state == STATE_PLAYING) {
            g.state = STATE_DEAD;
            levelsInitDeath(g.shipx, g.shipy);
        }

        g.spawnTimer += dt;

        if (g.spawnTimer >= g.spawnInterval) {
            g.spawnTimer = 0;
            spawn_enemy(g.shipx, g.shipy, g.xres, g.yres);
        }

        enemies_physics(g.shipx, g.shipy, g.xres, g.yres, dt);
    }

    float hpSpeed = 5;

    if (g.displayHP > g.playerHP) {
        g.displayHP -= hpSpeed * dt;

        if (g.displayHP < g.playerHP)
            g.displayHP = g.playerHP;
    } else if (g.displayHP < g.playerHP) {
        g.displayHP += hpSpeed * dt;

        if (g.displayHP > g.playerHP)
            g.displayHP = g.playerHP;
    }

    if (g.state == STATE_POWERUP) {
        float boxW = 200;
        float boxH = g.yres * 0.6f;
        float gap = 140;
        float cx = g.xres / 2.0f;
        float cy = g.yres / 2.0f;
        float totalW = boxW * 2 + gap;

        float boxesX[2] = {
            cx - totalW / 2 + boxW / 2,
            cx + totalW / 2 - boxW / 2
        };

        for (int i = 0; i < 2; i++) {
            bool inside =
                g.shipx > boxesX[i] - boxW / 2 &&
                g.shipx < boxesX[i] + boxW / 2 &&
                g.shipy > cy - boxH / 2 &&
                g.shipy < cy + boxH / 2;

            if (inside) {
                g.powerupFill[i] += dt / 2.5f;

                if (g.powerupFill[i] > 1)
                    g.powerupFill[i] = 1;

                if (g.powerupFill[i] >= 1) {
                    g.selectedPowerup = i;
                    pickPowerup(i);

                    if (lv_current < TOTAL_LEVELS - 1) {
                        lv_current++;
                        g.currentLevel++;

                        g.state = STATE_LEVEL_INTRO;
                        g.levelIntroTimer = 0;
                    } else {
                        g.state = STATE_GAME_WON;
                    }

                    g.powerupFill[0] = g.powerupFill[1] = 0;
                }
            } else {
                g.powerupFill[i] -= dt * 0.5f;

                if (g.powerupFill[i] < 0)
                    g.powerupFill[i] = 0;
            }
        }
    }
}

void renderShipBreakup()
{
    float s = getScale();
    int cols = 3;
    int rows = 2;

    float fullW = 60 * s;
    float fullH = 60 * s;

    float w = fullW / cols;
    float h = fullH / rows;

    float tileW = 1.0f / cols;
    float tileH = 1.0f / rows;

    glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);

    int idx = 0;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float tx0 = x * tileW;
            float tx1 = tx0 + tileW;
            float ty0 = y * tileH;
            float ty1 = ty0 + tileH;

            float offX = (x - (cols - 1) * 0.5f) * w;
            float offY = (y - (rows - 1) * 0.5f) * h;

            glPushMatrix();
            glTranslatef(g.shipx + offX, g.shipy + offY, 0);
            glRotatef(g.shipAngle - 90, 0, 0, 1);

            glBegin(GL_QUADS);
            glTexCoord2f(tx0, ty1); glVertex2f(-w / 2, -h / 2);
            glTexCoord2f(tx0, ty0); glVertex2f(-w / 2,  h / 2);
            glTexCoord2f(tx1, ty0); glVertex2f( w / 2,  h / 2);
            glTexCoord2f(tx1, ty1); glVertex2f( w / 2, -h / 2);
            glEnd();

            glPopMatrix();

            idx++;

            if (idx >= 6)
                return;
        }
    }
}

void renderShip()
{
    float s = getScale();
    float w = 60 * s;
    float h = 60 * s;

    glColor4f(1, 1, 1, 1);

    glPushMatrix();
    glTranslatef(g.shipx, g.shipy, 0);
    glRotatef(g.shipAngle - 90, 0, 0, 1);
    glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-w / 2, -h / 2);
    glTexCoord2f(0, 0); glVertex2f(-w / 2,  h / 2);
    glTexCoord2f(1, 0); glVertex2f( w / 2,  h / 2);
    glTexCoord2f(1, 1); glVertex2f( w / 2, -h / 2);
    glEnd();

    glPopMatrix();

    GLuint wtex;
    float fw;

    if (g.currentWeapon == 0) {
        wtex = g.tex.cannonTex;
        fw = 1.0f / 7;
    } else if (g.currentWeapon == 1) {
        wtex = g.tex.rocketsTex;
        fw = 1.0f / 16;
    } else if (g.currentWeapon == 2) {
        wtex = g.tex.spaceGunTex;
        fw = 1.0f / 12;
    } else {
        wtex = g.tex.zapperTex;
        fw = 1.0f / 14;
    }

    float tx0 = fw * (float)g.weaponFrame;
    float tx1 = tx0 + fw;
    float ww = 40 * s;
    float hh = 40 * s;

    glPushMatrix();
    glTranslatef(g.shipx, g.shipy, 0);
    glRotatef(g.shipAngle - 90, 0, 0, 1);
    glBindTexture(GL_TEXTURE_2D, wtex);

    glBegin(GL_QUADS);
    glTexCoord2f(tx0, 1); glVertex2f(-ww / 2, -hh / 2);
    glTexCoord2f(tx0, 0); glVertex2f(-ww / 2,  hh / 2);
    glTexCoord2f(tx1, 0); glVertex2f( ww / 2,  hh / 2);
    glTexCoord2f(tx1, 1); glVertex2f( ww / 2, -hh / 2);
    glEnd();

    glPopMatrix();
}

void renderBullets()
{
    glColor4f(1, 1, 1, 1);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g.bullets[i].active)
            continue;

        int t = g.bullets[i].type;

        GLuint tex;
        float fw;

        switch (t) {
            case 0:
                tex = g.tex.bulletCannonTex;
                fw = 1.0f / 4;
                break;

            case 1:
                tex = g.tex.bulletRocketTex;
                fw = 1.0f / 3;
                break;

            case 2:
                tex = g.tex.bulletSpaceGunTex;
                fw = 1.0f / 10;
                break;

            default:
                tex = g.tex.bulletZapperTex;
                fw = 1.0f / 8;
                break;
        }

        float tx0 = fw * g.bullets[i].frame;
        float tx1 = tx0 + fw;
        float x = g.bullets[i].x;
        float y = g.bullets[i].y;

        float s = getScale();
        float bw = 20 * s;
        float bh = 20 * s;

        glBindTexture(GL_TEXTURE_2D, tex);

        if (t == 1) {
            glPushMatrix();
            glTranslatef(x, y, 0);
            glRotatef(g.bullets[i].angle - 90, 0, 0, 1);

            float sp = 10 * s;

            for (int j = 0; j < 4; j++) {
                float off = (j - 1.5f) * sp;

                glBegin(GL_QUADS);
                glTexCoord2f(tx0, 1); glVertex2f(off - bw, -bh - 15);
                glTexCoord2f(tx0, 0); glVertex2f(off - bw,  bh);
                glTexCoord2f(tx1, 0); glVertex2f(off + bw,  bh);
                glTexCoord2f(tx1, 1); glVertex2f(off + bw, -bh - 15);
                glEnd();
            }

            glPopMatrix();
        } else if (t == 3) {
            glPushMatrix();
            glTranslatef(g.shipx, g.shipy, 0);
            glRotatef(g.shipAngle - 90, 0, 0, 1);

            float bl = sqrtf((float)(g.xres * g.xres + g.yres * g.yres));
            float bW = 40 * s;

            glBegin(GL_QUADS);
            glTexCoord2f(tx0, 1); glVertex2f(-bW / 2, 0);
            glTexCoord2f(tx0, 0); glVertex2f(-bW / 2, bl);
            glTexCoord2f(tx1, 0); glVertex2f( bW / 2, bl);
            glTexCoord2f(tx1, 1); glVertex2f( bW / 2, 0);
            glEnd();

            glPopMatrix();
        } else {
            glPushMatrix();
            glTranslatef(x, y, 0);
            glRotatef(g.bullets[i].angle - 90, 0, 0, 1);

            glBegin(GL_QUADS);
            glTexCoord2f(tx0, 1); glVertex2f(-bw, -bh);
            glTexCoord2f(tx0, 0); glVertex2f(-bw,  bh);
            glTexCoord2f(tx1, 0); glVertex2f( bw,  bh);
            glTexCoord2f(tx1, 1); glVertex2f( bw, -bh);
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

    float yOffset = -4.5f; // align bar inside texture
    float barOffset = w * 0.3f;

    glBindTexture(GL_TEXTURE_2D, g.tex.healthTex);
    glColor4f(1, 1, 1, 1);

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

static void renderPowerups()
{
    float boxW = 200;
    float boxH = g.yres * 0.6f;
    float gap = 40;
    float cx = g.xres / 2.0f;
    float cy = g.yres / 2.0f;
    float totalW = boxW * 2 + gap;

    float boxesX[2] = {
        cx - totalW / 2 + boxW / 2,
        cx + totalW / 2 - boxW / 2
    };

    const char* names[2] = {
        (g.currentChoices[0] >= 0) ? POWERUPS[g.currentChoices[0]] : "None",
        (g.currentChoices[1] >= 0) ? POWERUPS[g.currentChoices[1]] : "None"
    };

    for (int i = 0; i < 2; i++) {
        float x = boxesX[i];
        float y = cy;
        float fill = g.powerupFill[i];

        float rC = 0.2f;
        float gC = (i == 0) ? 0.6f : 0.9f;
        float bC = (i == 0) ? 1.0f : 0.2f;

        glDisable(GL_TEXTURE_2D);
        glLineWidth(3);
        glColor4f(rC, gC, bC, 1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(x - boxW / 2, y - boxH / 2);
        glVertex2f(x + boxW / 2, y - boxH / 2);
        glVertex2f(x + boxW / 2, y + boxH / 2);
        glVertex2f(x - boxW / 2, y + boxH / 2);
        glEnd();

        glColor4f(rC, gC, bC, 0.35f);

        glBegin(GL_QUADS);
        glVertex2f(x - boxW / 2, y - boxH / 2);
        glVertex2f(x + boxW / 2, y - boxH / 2);
        glVertex2f(x + boxW / 2, y - boxH / 2 + boxH * fill);
        glVertex2f(x - boxW / 2, y - boxH / 2 + boxH * fill);
        glEnd();

        glEnable(GL_TEXTURE_2D);

        Rect r;
        r.bot = (int)(y - 8);
        r.left = (int)x;
        r.center = 1;

        ggprint16(&r, 0, 0x00ffffff, names[i]);
    }
}

static void renderHUD()
{
    Rect r;
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;

    ggprint12(&r, 2, 0x00ffffff, "Spacebar - shoot");
    r.bot -= 20;

    ggprint12(&r, 2, 0x00ffffff, "P - pause");
    r.bot -= 20;

    ggprint12(&r, 2, 0x00ffffff, "Mouse + W - movement");
    r.bot -= 20;

    ggprint12(&r, 2, 0x00ffffff, "M - alt move (WASD)");
    r.bot -= 20;

    if (g.state == STATE_PLAYING) {
        ggprint12(&r, 2, 0x00ffffff, "R - reset obstacles");
        r.bot -= 20;
    }

    ggprint12(&r, 2, 0x00ffffff, "fps: %i", g.fps);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1, 1, 1, 1);

    // scrolling starfield background
    glBindTexture(GL_TEXTURE_2D, g.tex.backTex);

    glBegin(GL_QUADS);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0,      0);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0,      g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
    glEnd();

    if (g.state == STATE_TITLE) {
        obstaclesDrawAsteroidsOnly();
        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
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

    if (g.state == STATE_DEAD)
        renderShipBreakup();
    else if (g.state != STATE_GAME_WON)
        renderShip();

    renderBullets();

    if (g.state == STATE_LEVEL_INTRO)
        renderLevelIntro();

    if (g.state == STATE_LEVEL_COMPLETE)
        levelsRenderComplete();

    if (g.state == STATE_DEAD)
        levelsRenderDeath();

    if (g.state == STATE_GAME_WON)
        levelsRenderGameWon();

    if (g.state == STATE_PLAYING) {
        renderHealthBar();
        levelsRenderHUD();
    }

    if (g.state == STATE_POWERUP)
        renderPowerups();

    if (g.state != STATE_GAME_WON)
        renderHUD();
}
