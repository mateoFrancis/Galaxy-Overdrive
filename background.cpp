

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
    float vel;
    int active;
    int type; // 
    int frame;
    float frameTimer;
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

        int keys[256]; 

        Bullet bullets[30];

        Global() {
                xres=640, yres=480;
                mousex = xres/2;
                mousey = yres/2;
                spacePressed = 0;
                currentWeapon = 2; // 0=cannon, 1=rockets, 2=spaceGun, 3=zapper

                weaponFrame = 0;
                weaponTimer = 0.0f;

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
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    // space press/release
    if (e->type == KeyPress)   g.keys[key] = 1;
    if (e->type == KeyRelease) g.keys[key] = 0;

    if (e->type == KeyPress) {
        switch (key) {
            case XK_Escape:
                return 1; // exit 

            case XK_space:
                g.spacePressed = 1;
                break;

            case XK_s: // switch weapons
                g.currentWeapon++;
                if (g.currentWeapon > 3) 
                    g.currentWeapon = 0;
                g.weaponFrame = 0; 
                g.weaponTimer = 0.0f;
                break;

        
        }
    }

    if (e->type == KeyRelease) {
        switch (key) {
            case XK_space:
                g.spacePressed = 0;
                break;
        }
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
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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


void physics() {

    g.tex.yc[0] += 0.01;
    g.tex.yc[1] += 0.01;

    // weapon animation
    if (g.spacePressed) {

        g.weaponTimer += 0.2f;

        int maxFrames;
        switch (g.currentWeapon) {
            case 0: maxFrames = 7;  break;
            case 1: maxFrames = 16; break;
            case 2: maxFrames = 12; break;
            case 3: maxFrames = 14; break;
            default: maxFrames = 7; break;
        }

        if (g.weaponTimer >= 1.0f) {

            g.weaponTimer = 0.0f;
            g.weaponFrame++;
            if (g.weaponFrame >= maxFrames) g.weaponFrame = 1;
        }
    } else {

        g.weaponFrame = 0;
    }

    // fire bullets
    static float fireCooldown = 0.0f;
    float fireRate;
    switch (g.currentWeapon) {

        case 0: fireRate = 0.3f; break;
        case 1: fireRate = 0.5f; break;
        case 2: fireRate = 0.1f; break;
        case 3: fireRate = 0.2f; break;
        default: fireRate = 0.3f; break;
    }

    if (g.spacePressed) {
        fireCooldown += 0.016f; 
        if (fireCooldown >= fireRate) {

            for (int i = 0; i < 30; i++) {

                if (!g.bullets[i].active) {

                    g.bullets[i].active = 1;
                    g.bullets[i].x = g.mousex;
                    g.bullets[i].y = g.mousey + 20;
                    g.bullets[i].type = g.currentWeapon;
                    g.bullets[i].frame = 0;
                    g.bullets[i].frameTimer = 0.0f;

                    switch (g.currentWeapon) {

                        case 0: g.bullets[i].vel = 12.0f; break;
                        case 1: g.bullets[i].vel = 8.0f;  break;
                        case 2: g.bullets[i].vel = 15.0f; break;
                        case 3: g.bullets[i].vel = 18.0f; break;
                        default: g.bullets[i].vel = 10.0f; break;
                    }
                    break;
                }
            }
            fireCooldown = 0.0f;
        }
    } else {

        fireCooldown = fireRate;
    }

    // update bullets
    for (int i = 0; i < 30; i++) {

        if (g.bullets[i].active) {

            g.bullets[i].y += g.bullets[i].vel;

            if (g.bullets[i].y > g.yres) {

                g.bullets[i].active = 0;
                continue;
            }

            g.bullets[i].frameTimer += 0.2f;

            int maxFrames;
            switch (g.bullets[i].type) {

                case 0: maxFrames = 4; break;
                case 1: maxFrames = 3; break;
                case 2: maxFrames = 10; break;
                case 3: maxFrames = 8; break;
                default: maxFrames = 4; break;
            }

            if (g.bullets[i].frameTimer >= 1.0f) {

                g.bullets[i].frameTimer = 0.0f;
                g.bullets[i].frame++;
                if (g.bullets[i].frame >= maxFrames) g.bullets[i].frame = 0;
            }
        }
    }

   // title_physics(g.title);
}

void renderBullets() {

    glEnable(GL_ALPHA_TEST);
   // glAlphaFunc(GL_GREATER, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < 30; i++) {

        if (!g.bullets[i].active) continue;

        GLuint tex;
        float frameWidth;

        // choose texture based on bullet type
        switch (g.bullets[i].type) {

            case 0:
                tex = g.tex.bulletCannonTex;
                frameWidth = 1.0f / 4.0f;
                break;
            case 1:
                tex = g.tex.bulletRocketTex;
                frameWidth = 1.0f / 3.0f;
                break;
            case 2:
                tex = g.tex.bulletSpaceGunTex;
                frameWidth = 1.0f / 10.0f;
                break;
            case 3:
                tex = g.tex.bulletZapperTex;
                frameWidth = 1.0f / 8.0f;
                break;
            default:
                tex = g.tex.bulletCannonTex;
                frameWidth = 1.0f / 4.0f;
                break;
        }

        float tx0 = frameWidth * g.bullets[i].frame;
        float tx1 = tx0 + frameWidth;

        float x = g.bullets[i].x;
        float y = g.bullets[i].y;

        float w = 20;
        float h = 20;

        int rocketSplit = 60/4;
        float zapperLen = 100;

        glBindTexture(GL_TEXTURE_2D, tex);

        if (g.bullets[i].type == 1) {

            
          //  for (int x = 0; x < 4; x ++) {

                glBegin(GL_QUADS);
                    glTexCoord2f(tx0, 1); glVertex2f(x-w, y-h);
                    glTexCoord2f(tx0, 0); glVertex2f(x-w, y+h);
                    glTexCoord2f(tx1, 0); glVertex2f(x+w, y+h);
                    glTexCoord2f(tx1, 1); glVertex2f(x+w, y-h);
                glEnd();
           // }
        }
        else if(g.bullets[i].type == 3) {

                glBegin(GL_QUADS);
                    glTexCoord2f(tx0, 1); glVertex2f(x-w, y-h);
                    glTexCoord2f(tx0, 0); glVertex2f(x-w, y+h);
                    glTexCoord2f(tx1, 0); glVertex2f(x+w, y+h);
                    glTexCoord2f(tx1, 1); glVertex2f(x+w, y-h);
                glEnd();
        }
        else{ 

        glBegin(GL_QUADS);
            glTexCoord2f(tx0, 1); glVertex2f(x-w, y-h);
            glTexCoord2f(tx0, 0); glVertex2f(x-w, y+h);
            glTexCoord2f(tx1, 0); glVertex2f(x+w, y+h);
            glTexCoord2f(tx1, 1); glVertex2f(x+w, y-h);
        glEnd();
        }
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}

void renderWeapon(int type)
{
    //float w = 40;
    //float h = 40;

    float x = g.mousex;
    float y = g.mousey + 40; 

   if (g.spacePressed || g.weaponFrame == 0) {

        GLuint tex;

        if (g.currentWeapon == 0) tex = g.tex.cannonTex;
        else if (g.currentWeapon == 1) tex = g.tex.rocketsTex;
        else if (g.currentWeapon == 2) tex = g.tex.spaceGunTex;
        else tex = g.tex.zapperTex;

        float ww = 60;
        float hh = 60;

        // sprite sheet math
        float frameWidth;
        switch (g.currentWeapon) {

            case 0: frameWidth = 1.0f / 7.0f; break;
            case 1: frameWidth = 1.0f / 16.0f; break;
            case 2: frameWidth = 1.0f / 12.0f; break;
            case 3: frameWidth = 1.0f / 14.0f; break;
            default: frameWidth = 1.0f / 7.0f; break;
        }

        float tx0 = frameWidth * g.weaponFrame;
        float tx1 = tx0 + frameWidth;

        glBindTexture(GL_TEXTURE_2D, tex);

        glBegin(GL_QUADS);
            glTexCoord2f(tx0, 1); glVertex2f(x - ww/2, y - hh/2);
            glTexCoord2f(tx0, 0); glVertex2f(x - ww/2, y + hh/2);
            glTexCoord2f(tx1, 0); glVertex2f(x + ww/2, y + hh/2);
            glTexCoord2f(tx1, 1); glVertex2f(x + ww/2, y - hh/2);
        glEnd();
    }
}

void renderShip()
{
    float w = 60;
    float h = 60;

    float x = g.mousex;
    float y = g.mousey;

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

    glBindTexture(GL_TEXTURE_2D, g.tex.ship01Tex);

    glBegin(GL_QUADS);
        glTexCoord2f(0,1); glVertex2f(x - w/2, y - h/2);
        glTexCoord2f(0,0); glVertex2f(x - w/2, y + h/2);
        glTexCoord2f(1,0); glVertex2f(x + w/2, y + h/2);
        glTexCoord2f(1,1); glVertex2f(x + w/2, y - h/2);
    glEnd();

    if (g.spacePressed || g.weaponFrame == 0) {

        GLuint tex;

        if (g.currentWeapon == 0) tex = g.tex.cannonTex;
        else if (g.currentWeapon == 1) tex = g.tex.rocketsTex;
        else if (g.currentWeapon == 2) tex = g.tex.spaceGunTex;
        else tex = g.tex.zapperTex;

        float ww = 40;
        float hh = 40;

        float frameWidth;

        switch (g.currentWeapon) {

            case 0: frameWidth = 1.0f / 7.0f; break;
            case 1: frameWidth = 1.0f / 16.0f; break;
            case 2: frameWidth = 1.0f / 12.0f; break;
            case 3: frameWidth = 1.0f / 14.0f; break;
            default: frameWidth = 1.0f / 7.0f; break;
        }

        float tx0 = frameWidth * g.weaponFrame;
        float tx1 = tx0 + frameWidth;

        glBindTexture(GL_TEXTURE_2D, tex);

        glBegin(GL_QUADS);
            glTexCoord2f(tx0, 1); glVertex2f(x - ww/2, y - hh/2);
            glTexCoord2f(tx0, 0); glVertex2f(x - ww/2, y + hh/2);
            glTexCoord2f(tx1, 0); glVertex2f(x + ww/2, y + hh/2);
            glTexCoord2f(tx1, 1); glVertex2f(x + ww/2, y - hh/2);
        glEnd();
    }

    glDisable(GL_ALPHA_TEST);
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
}
