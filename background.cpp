

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


class Image {
public:
    int width, height;
    unsigned char *data;
    ~Image() { delete [] data; }

    Image(const char *fname) {
        if (fname[0] == '\0')
            return;

        char name[40];
        strcpy(name, fname);
        int slen = strlen(name);
        name[slen-4] = '\0';

        char ppmname[80];
        sprintf(ppmname, "%s.ppm", name);

        // Keep alpha when converting PNG to PPM
        char ts[200];
        sprintf(ts, "convert %s %s", fname, ppmname); 
        system(ts);

        FILE *fpi = fopen(ppmname, "rb");
        if (fpi) {
            char line[200];
            fgets(line, 200, fpi); 
            fgets(line, 200, fpi); 
            while (line[0] == '#')
                fgets(line, 200, fpi);

            sscanf(line, "%i %i", &width, &height);
            fgets(line, 200, fpi);

            int n = width * height * 4; // RGBA
            data = new unsigned char[n];

            for (int i = 0; i < width*height; i++) {
                   
                unsigned char r = fgetc(fpi);
                    unsigned char g = fgetc(fpi);
                    unsigned char b = fgetc(fpi);
                    data[i*4+0] = r;
                    data[i*4+1] = g;
                    data[i*4+2] = b;

                    // Alpha = 0 if the pixel is background, else 255

                    if (r < 50 && g < 50 && b < 50) 
                        data[i*4+3] = 0;   // transparent
                    else
                        data[i*4+3] = 255; // opaque
             }

            fclose(fpi);
        } else {
            printf("ERROR opening image: %s\n", ppmname);
            exit(0);
        }

        unlink(ppmname);
    }
};
Image img[] = {
    "./images/Starfield08.png",
    "./images/galov.png",

    "./ship_sprites/ship_full.png",
    "./ship_sprites/ship_slight.png",
    "./ship_sprites/ship_mid.png",
    "./ship_sprites/ship_very.png",

    "./weapons/cannon.png",
    "./weapons/rockets.png",
    "./weapons/spaceGun.png",
    "./weapons/zapper.png",

    "./bullets/weapons_cannon.png",
    "./bullets/weapons_rocket.png",
    "./bullets/weapons_spaceGun.png",
    "./bullets/weapons_zapper.png"
};

class Texture {
public:
    Image *backImage;
    Image *ship01Image;

    Image *shipSlightImage;
    Image *shipMidImage;
    Image *shipVeryImage;

    Image *cannonImage;
    Image *rocketsImage;
    Image *spaceGunImage;
    Image *zapperImage;

    Image *bulletCannonImage;
    Image *bulletRocketImage;
    Image *bulletSpaceGunImage;
    Image *bulletZapperImage;

    GLuint backTexture;
    GLuint logoTexture;
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

    int logoW, logoH;
    float xc[2];
    float yc[2];
};

struct Bullet {
    float x, y;
    float xVel, yVel;   // velocity vector
    float vel;          // speed magnitude
    int active;
    int type;           // weapon type
    int frame;
    float frameTimer;
    float angle;

};

class TitleAnim {
public:
        float timer;
        TitleAnim() : timer(0.0f) {}
};

class Global {
public:
        int xres, yres;
        Texture tex;

        TitleAnim title;

        int mousex, mousey;

        int spacePressed;
        int currentWeapon;

        int weaponFrame;
        float weaponTimer;

        float shipx, shipy;
        float ShipSpeed;
        float shipAngle;

        int keys[256]; 

        Bullet bullets[30];

        Global() {
                xres=640, yres=480;
                mousex = xres/2;
                mousey = yres/2;
                spacePressed = 0;
                currentWeapon = 0; // 0=cannon, 1=rockets, 2=spaceGun, 3=zapper

                weaponFrame = 0;
                weaponTimer = 0.0f;

                shipx = xres / 2;
                shipy = yres / 3;
                shipAngle = 0.0f;
                ShipSpeed = 6.0f;

                memset(keys, 0, sizeof(keys)); // all keys = 0

                for (int i=0; i<30; i++) {

                        bullets[i].active = 0;
                }
        }
} g;

class X11_wrapper {
private:
        Display *dpy;
        Window win;
        GLXContext glc;
public:
        X11_wrapper() {
                GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
                setup_screen_res(640, 480);
                dpy = XOpenDisplay(NULL);
                if(dpy == NULL) {
                        printf("\n\tcannot connect to X server\n\n");
                        exit(EXIT_FAILURE);
                }
                Window root = DefaultRootWindow(dpy);
                XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
                if(vi == NULL) {
                        printf("\n\tno appropriate visual found\n\n");
                        exit(EXIT_FAILURE);
                }
                Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
                XSetWindowAttributes swa;
                swa.colormap = cmap;
                swa.event_mask =
                        ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                        ButtonPressMask | ButtonReleaseMask |
                        StructureNotifyMask | SubstructureNotifyMask;
                win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                                                                vi->depth, InputOutput, vi->visual,
                                                                CWColormap | CWEventMask, &swa);
                set_title();
                glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
                glXMakeCurrent(dpy, win, glc);
        }
        void cleanupXWindows() {
                XDestroyWindow(dpy, win);
                XCloseDisplay(dpy);
        }
        void setup_screen_res(const int w, const int h) {
                g.xres = w;
                g.yres = h;
        }
        void reshape_window(int width, int height) {
                setup_screen_res(width, height);
                glViewport(0, 0, (GLint)width, (GLint)height);
                glMatrixMode(GL_PROJECTION); glLoadIdentity();
                glMatrixMode(GL_MODELVIEW); glLoadIdentity();
                glOrtho(0, g.xres, 0, g.yres, -10, 1);
                set_title();
        }
        void set_title() {
                XMapWindow(dpy, win);
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
                if (xce.width != g.xres || xce.height != g.yres) {
                        reshape_window(xce.width, xce.height);
                }
        }
} x11;

void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);
void title_physics(TitleAnim &t);
void title_render(const TitleAnim &t);

int main()
{
        init_opengl();
        int done=0;
        while (!done) {
                while (x11.getXPending()) {
                        XEvent e = x11.getXNextEvent();
                        x11.check_resize(&e);
                        check_mouse(&e);
                        done = check_keys(&e);
                }
                physics();
                render();
                x11.swapBuffers();
        }
        return 0;
}

void init_opengl(void)
{
        glViewport(0, 0, g.xres, g.yres);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glOrtho(0, g.xres, 0, g.yres, -1, 1);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glEnable(GL_TEXTURE_2D);
        initialize_fonts();

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glEnable(GL_BLEND);
//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        g.tex.backImage = &img[0];
        glGenTextures(1, &g.tex.backTexture);
        int w = g.tex.backImage->width;
        int h = g.tex.backImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                                        GL_RGBA, GL_UNSIGNED_BYTE, g.tex.backImage->data);
        g.tex.xc[0] = 0.0;
        g.tex.xc[1] = 1.0;
        g.tex.yc[0] = 0.0;
        g.tex.yc[1] = 1.0;


        Image *logo = &img[1];
        g.tex.logoW = logo->width;
        g.tex.logoH = logo->height;
        glGenTextures(1, &g.tex.logoTexture);
        glBindTexture(GL_TEXTURE_2D, g.tex.logoTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 4, logo->width, logo->height,
                                        GL_RGBA, GL_UNSIGNED_BYTE, logo->data);

        // ship_full
        g.tex.ship01Image = &img[2];
        glGenTextures(1, &g.tex.ship01Tex);
        w = g.tex.ship01Image->width;
        h = g.tex.ship01Image->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                                        GL_RGBA, GL_UNSIGNED_BYTE, g.tex.ship01Image->data);

        // ship_slight
        g.tex.shipSlightImage = &img[3];
        glGenTextures(1, &g.tex.shipSlightTex);
        w = g.tex.shipSlightImage->width;
        h = g.tex.shipSlightImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.shipSlightTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.shipSlightImage->data);

        // ship_mid
        g.tex.shipMidImage = &img[4];
        glGenTextures(1, &g.tex.shipMidTex);
        w = g.tex.shipMidImage->width;
        h = g.tex.shipMidImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.shipMidTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.shipMidImage->data);

        // ship_very
        g.tex.shipVeryImage = &img[5];
        glGenTextures(1, &g.tex.shipVeryTex);
        w = g.tex.shipVeryImage->width;
        h = g.tex.shipVeryImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.shipVeryTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.shipVeryImage->data);


        // cannon
        g.tex.cannonImage = &img[6];
        glGenTextures(1, &g.tex.cannonTex);
        w = g.tex.cannonImage->width;
        h = g.tex.cannonImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.cannonTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.cannonImage->data);

        // rockets
        g.tex.rocketsImage = &img[7];
        glGenTextures(1, &g.tex.rocketsTex);
        w = g.tex.rocketsImage->width;
        h = g.tex.rocketsImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.rocketsTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.rocketsImage->data);

        // spaceGun
        g.tex.spaceGunImage = &img[8];
        glGenTextures(1, &g.tex.spaceGunTex);
        w = g.tex.spaceGunImage->width;
        h = g.tex.spaceGunImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.spaceGunTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.spaceGunImage->data);

        // zapper
        g.tex.zapperImage = &img[9];
        glGenTextures(1, &g.tex.zapperTex);
        w = g.tex.zapperImage->width;
        h = g.tex.zapperImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.zapperTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, g.tex.zapperImage->data);

        // cannon bullet
        g.tex.bulletCannonImage = &img[10];
        glGenTextures(1, &g.tex.bulletCannonTex);
        glBindTexture(GL_TEXTURE_2D, g.tex.bulletCannonTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4,
            g.tex.bulletCannonImage->width,
            g.tex.bulletCannonImage->height,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            g.tex.bulletCannonImage->data);

        // rocket bullet
        g.tex.bulletRocketImage = &img[11];
        glGenTextures(1, &g.tex.bulletRocketTex);
        glBindTexture(GL_TEXTURE_2D, g.tex.bulletRocketTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4,
            g.tex.bulletRocketImage->width,
            g.tex.bulletRocketImage->height,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            g.tex.bulletRocketImage->data);
        
        // spaceGun bullet
        g.tex.bulletSpaceGunImage = &img[12];
        glGenTextures(1, &g.tex.bulletSpaceGunTex);
        glBindTexture(GL_TEXTURE_2D, g.tex.bulletSpaceGunTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4,
            g.tex.bulletSpaceGunImage->width,
            g.tex.bulletSpaceGunImage->height,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            g.tex.bulletSpaceGunImage->data);
        
        // zapper bullet
        g.tex.bulletZapperImage = &img[13];
        glGenTextures(1, &g.tex.bulletZapperTex);
        glBindTexture(GL_TEXTURE_2D, g.tex.bulletZapperTex);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4,
            g.tex.bulletZapperImage->width,
            g.tex.bulletZapperImage->height,
            0, GL_RGBA, GL_UNSIGNED_BYTE,
            g.tex.bulletZapperImage->data);
}

void check_mouse(XEvent *e)
{
        static int savex = 0;
        static int savey = 0;

        if (e->type == MotionNotify) {
                g.mousex = e->xmotion.x;
                g.mousey = g.yres - e->xmotion.y; 
        }

        if (e->type == ButtonRelease) {
                return;
        }
        if (e->type == ButtonPress) {
                if (e->xbutton.button==1) {
                }
                if (e->xbutton.button==3) {
                }
        }
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
                savex = e->xbutton.x;
                savey = e->xbutton.y;
        }
}

int check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;

    int key = XLookupKeysym(&e->xkey, 0);

    if (e->type == KeyPress)
        g.keys[key] = 1;
    else
        g.keys[key] = 0;

    if (e->type == KeyPress) {
        switch (key) {
            case XK_Escape:
                return 1;
            
            case XK_equal:
                g.ShipSpeed += g.ShipSpeed;
                break;

            case XK_space:
                g.spacePressed = 1;
                break;

            case XK_s:
                g.currentWeapon++;
                if (g.currentWeapon > 3)
                    g.currentWeapon = 0;
                g.weaponFrame = 0;
                g.weaponTimer = 0.0f;
                break;
        }
    }

    if (e->type == KeyRelease) {
        if (key == XK_space)
            g.spacePressed = 0;
    }

    return 0;
}

void title_physics(TitleAnim &t)
{
        if (t.timer < 1.0f)
                t.timer += 0.01f;
}

void title_render(const TitleAnim &t)
{
        float ease = t.timer * t.timer * (3.0f - 2.0f * t.timer);
        float maxW = (float)g.xres * 0.6f;
        float maxH = maxW * ((float)g.tex.logoH / (float)g.tex.logoW);
        float w = maxW * ease;
        float h = maxH * ease;
        float cx = g.xres / 2.0f;
        float cy = g.yres / 2.0f;
        float x0 = cx - w / 2.0f;
        float x1 = cx + w / 2.0f;
        float y0 = cy - h / 2.0f;
        float y1 = cy + h / 2.0f;
        glEnable(GL_BLEND);
//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        glBindTexture(GL_TEXTURE_2D, g.tex.logoTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y0);
                glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y1);
                glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y1);
                glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y0);
        glEnd();
        glDisable(GL_BLEND);
}

const float FRAME_TIME = 0.16f;

// fire cooldown in seconds
float FIRE_COOLDOWN = 1.5f;  // fire rate (lower = faster)

// animation speed multiplier (to match fire rate)
float ANIM_SPEED_MULTIPLIER = 5.0f; 

// zapper animation speed
float ZAPPER_ANIM_SPEED = 0.0f;

// max bullets
int MAX_BULLETS = 50;  // numbr of bullets


void physics()
{
    g.tex.yc[0] += 0.01;
    g.tex.yc[1] += 0.01;

    // ship movement (with rotation)
    float rad = g.shipAngle * M_PI / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);
    float moveSpeed = g.ShipSpeed;

    float dx = 0.0f, dy = 0.0f;

    if (g.keys[XK_w]) { 

        dx += cosA * moveSpeed;
        dy += sinA * moveSpeed; 
    }

    if (g.keys[XK_a]) {
        
        dx -= sinA * moveSpeed;
        dy += cosA * moveSpeed; 
    }
    if (g.keys[XK_d]) {
        
        dx += sinA * moveSpeed;
        dy -= cosA * moveSpeed; 
    }

    g.shipx += dx;
    g.shipy += dy;

    if (g.shipx < 0) 
        g.shipx = 0;

    if (g.shipx > g.xres) 
        g.shipx = g.xres;

    if (g.shipy < 0) 
        g.shipy = 0;

    if (g.shipy > g.yres) 
        g.shipy = g.yres;

    // Aim at mouse
    float mx = g.mousex - g.shipx;
    float my = g.mousey - g.shipy;

    g.shipAngle = atan2(my, mx) * 180.0f / M_PI;

    // for weapon animation
    g.weaponTimer += FRAME_TIME;

    int maxFrames = (g.currentWeapon == 0) ? 7 :
                    (g.currentWeapon == 1) ? 16 :
                    (g.currentWeapon == 2) ? 12 : 14;

    // bullets fired
    if (g.spacePressed && g.currentWeapon != 3) {
        if (g.weaponTimer > FIRE_COOLDOWN) {  // global fire rate
            g.weaponTimer = 0.0f;

            for (int i = 0; i < MAX_BULLETS; i++) {

                if (!g.bullets[i].active) {

                    g.bullets[i].active = 1;
                    g.bullets[i].type = g.currentWeapon;
                    g.bullets[i].x = g.shipx;
                    g.bullets[i].y = g.shipy;
                    g.bullets[i].vel = 10.0f;
                    g.bullets[i].frame = 0;
                    g.bullets[i].frameTimer = 0.0f; // reset timer
                    g.bullets[i].angle = g.shipAngle;

                    float radb = g.shipAngle * M_PI / 180.0f;

                    g.bullets[i].xVel = cos(radb) * g.bullets[i].vel;
                    g.bullets[i].yVel = sin(radb) * g.bullets[i].vel;
                    break;
                }
            }

            // move weapon animation (relative to fire rate)
            g.weaponFrame += ANIM_SPEED_MULTIPLIER;
            if (g.weaponFrame >= maxFrames)
                g.weaponFrame = 1;
        }
    } else if (g.currentWeapon != 3) {

        g.weaponFrame = 0;
    }

    // zapper on
    if (g.currentWeapon == 3) {

        if (g.spacePressed) {

            bool zapActive = false;

            for (int i = 0; i < MAX_BULLETS; i++) {

                if (g.bullets[i].active && g.bullets[i].type == 3) {
                    
                    zapActive = true;
                    break;
                }
            }
            if (!zapActive) {

                for (int i = 0; i < MAX_BULLETS; i++) {

                    if (!g.bullets[i].active) {

                        g.bullets[i].active = 1;
                        g.bullets[i].type = 3;
                        g.bullets[i].x = g.shipx;
                        g.bullets[i].y = g.shipy;
                        g.bullets[i].angle = g.shipAngle;
                        g.bullets[i].frame = 0;
                        g.bullets[i].frameTimer = 0.0f;
                        g.bullets[i].xVel = 0.0f;
                        g.bullets[i].yVel = 0.0f;
                        break;
                    }
                }
            }

            // zapper animation
            g.weaponFrame++;

            if (g.weaponFrame >= maxFrames) 
                g.weaponFrame = 1;

        } else {

            // turn off zapper
            for (int i = 0; i < MAX_BULLETS; i++) {

                if (g.bullets[i].type == 3)
                    g.bullets[i].active = 0;
            }
            g.weaponFrame = 0;
        }
    }

    // update bullets and animate frames seperately
    for (int i = 0; i < MAX_BULLETS; i++) {

        if (!g.bullets[i].active)
            continue;

        g.bullets[i].x += g.bullets[i].xVel;
        g.bullets[i].y += g.bullets[i].yVel;

        // deactivate if off screen
        if (g.bullets[i].type != 3 &&
            (g.bullets[i].x < 0 || 
             g.bullets[i].x > g.xres ||
             g.bullets[i].y < 0 || 
             g.bullets[i].y > g.yres)) {

            g.bullets[i].active = 0;
        }

        g.bullets[i].frameTimer += FRAME_TIME;

        float bulletAnimInterval;
        switch (g.bullets[i].type) {

            case 0: bulletAnimInterval = 0.1f; // cannon
                    break;  
            case 1: bulletAnimInterval = 0.08f; // rocket
                     break; 
            case 2: bulletAnimInterval = 0.05f; // spaceGun
                     break; 
            case 3: bulletAnimInterval = 0.07f; // zapper
                    break; 
            default: bulletAnimInterval = 0.1f; 
                    break;
        }

        if (g.bullets[i].frameTimer > bulletAnimInterval) {

            g.bullets[i].frameTimer = 0.0f;
            g.bullets[i].frame++;

            int bulletMaxFrames = (g.bullets[i].type == 0) ? 4 :
                                  (g.bullets[i].type == 1) ? 3 :
                                  (g.bullets[i].type == 2) ? 10 :
                                  8;  // zapper

            if (g.bullets[i].frame >= bulletMaxFrames) 
                g.bullets[i].frame = 0;
        }
    }

    //title_physics(g.title);
}



void renderBullets()
{
    glEnable(GL_ALPHA_TEST);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < 30; i++) {

        if (!g.bullets[i].active) continue;

        GLuint tex;
        float frameWidth;

        switch (g.bullets[i].type) {

            case 0: tex = g.tex.bulletCannonTex;
                    frameWidth = 1.0f/4.0f;
                     break;
            case 1: tex = g.tex.bulletRocketTex;
                     frameWidth = 1.0f/3.0f;
                      break;
            case 2: tex = g.tex.bulletSpaceGunTex; 
                    frameWidth = 1.0f/10.0f;
                     break;
            case 3: tex = g.tex.bulletZapperTex; 
                    frameWidth = 1.0f/8.0f; 
                    break;
            default: tex = g.tex.bulletCannonTex; frameWidth = 1.0f/4.0f; 
                    break;
        }

        float tx0 = frameWidth * g.bullets[i].frame;
        float tx1 = tx0 + frameWidth;

        float x = g.bullets[i].x;
        float y = g.bullets[i].y;

        float w = 20;
        float h = 20;

        glBindTexture(GL_TEXTURE_2D, tex);

        // rockets rotated
        if (g.bullets[i].type == 1) {

            glPushMatrix();
            glTranslatef(x, y, 0.0f);
            glRotatef(g.bullets[i].angle - 90.0f, 0,0,1);

            float spacing = 10.0f;

            for (int j = 0; j < 4; j++) {

                float offset = (j - 1.5f) * spacing;

                glBegin(GL_QUADS);
                    glTexCoord2f(tx0,1); glVertex2f(offset-w, -h-15);
                    glTexCoord2f(tx0,0); glVertex2f(offset-w,  h);
                    glTexCoord2f(tx1,0); glVertex2f(offset+w,  h);
                    glTexCoord2f(tx1,1); glVertex2f(offset+w, -h-15);
                glEnd();
            }

            glPopMatrix();
        }

        // zapper rotation
        else if (g.bullets[i].type == 3) {

            glPushMatrix();
            glTranslatef(g.shipx, g.shipy, 0.0f);
            glRotatef(g.shipAngle - 90.0f, 0,0,1);

            float beamLength = g.yres;
            float beamWidth  = 40.0f;

            glBegin(GL_QUADS);
                glTexCoord2f(tx0,1); glVertex2f(-beamWidth/2, 0);
                glTexCoord2f(tx0,0); glVertex2f(-beamWidth/2, beamLength);
                glTexCoord2f(tx1,0); glVertex2f( beamWidth/2, beamLength);
                glTexCoord2f(tx1,1); glVertex2f( beamWidth/2, 0);
            glEnd();

            glPopMatrix();
        }

        // normal bullets
        else {
            glPushMatrix();
            glTranslatef(x, y, 0.0f);
            glRotatef(g.bullets[i].angle - 90.0f, 0,0,1);

            glBegin(GL_QUADS);
                glTexCoord2f(tx0,1); glVertex2f(-w, -h);
                glTexCoord2f(tx0,0); glVertex2f(-w,  h);
                glTexCoord2f(tx1,0); glVertex2f( w,  h);
                glTexCoord2f(tx1,1); glVertex2f( w, -h);
            glEnd();

            glPopMatrix();
        }
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}


void renderShip()
{
    float w = 60;
    float h = 60;

    float x = g.shipx;
    float y = g.shipy;

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(g.shipAngle - 90.0f, 0.0f, 0.0f, 1.0f);

    // ship
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex2f(-w/2, -h/2);
        glTexCoord2f(0,0); glVertex2f(-w/2,  h/2);
        glTexCoord2f(1,0); glVertex2f( w/2,  h/2);
        glTexCoord2f(1,1); glVertex2f( w/2, -h/2);
    glEnd();

    // weapon (rotates with ship)
    GLuint tex;

    if (g.currentWeapon == 0)
        tex = g.tex.cannonTex;
    else if (g.currentWeapon == 1) 
        tex = g.tex.rocketsTex;
    else if (g.currentWeapon == 2) 
        tex = g.tex.spaceGunTex;
    else tex = g.tex.zapperTex;

    float frameWidth =
        (g.currentWeapon == 0) ? 1.0f/7.0f :
        (g.currentWeapon == 1) ? 1.0f/16.0f :
        (g.currentWeapon == 2) ? 1.0f/12.0f :
                                1.0f/14.0f;

    float tx0 = frameWidth * g.weaponFrame;
    float tx1 = tx0 + frameWidth;

    float ww = 40;
    float hh = 40;

    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUADS);
        glTexCoord2f(tx0, 1); glVertex2f(-ww/2, -hh/2);
        glTexCoord2f(tx0, 0); glVertex2f(-ww/2,  hh/2);
        glTexCoord2f(tx1, 0); glVertex2f( ww/2,  hh/2);
        glTexCoord2f(tx1, 1); glVertex2f( ww/2, -hh/2);
    glEnd();

    glDisable(GL_ALPHA_TEST);
    glPopMatrix();
}

void render()
{
        
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
        glBegin(GL_QUADS);
                glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0,      0);
                glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0,      g.yres);
                glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
                glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
        glEnd();

        title_render(g.title);

        renderShip();
        renderBullets();

       // g.currentWeapon = 1;
       // if (g.spacePressed) 
        

       // title_render(g.title);

    Rect r;
	r.bot = g.yres - 20;
	r.left = 10;
	r.center = 0;
    ggprint16(&r, 10, 0x00ffffff, "S - switch weapon"); // white text
    r.bot -= 20;
    ggprint16(&r, 10, 0x00ffffff, "Spacebar - shoot");
    r.bot -= 20;
    ggprint16(&r, 10, 0x00ffffff, "W + Mouse Pointer - movement");
}
