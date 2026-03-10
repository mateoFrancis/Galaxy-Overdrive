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
                sprintf(ppmname,"%s.ppm", name);
                char ts[200];
                sprintf(ts, "convert %s -background black -flatten %s", fname, ppmname);
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
                        int n = width * height * 3;
                        data = new unsigned char[n];
                        for (int i=0; i<n; i++)
                                data[i] = fgetc(fpi);
                        fclose(fpi);
                } else {
                        printf("ERROR opening image: %s\n", ppmname);
                        exit(0);
                }
                unlink(ppmname);
        }
};
Image img[2] = {"Starfield08.png", "galov.png"};

class Texture {
public:
        Image *backImage;
        GLuint backTexture;
        GLuint logoTexture;
        int logoW, logoH;
        float xc[2];
        float yc[2];
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
        Global() {
                xres=640, yres=480;
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
        g.tex.backImage = &img[0];
        glGenTextures(1, &g.tex.backTexture);
        int w = g.tex.backImage->width;
        int h = g.tex.backImage->height;
        glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
                                        GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
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
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, logo->width, logo->height,
                                        GL_RGB, GL_UNSIGNED_BYTE, logo->data);
}

void check_mouse(XEvent *e)
{
        static int savex = 0;
        static int savey = 0;
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
        if (e->type == KeyPress) {
                int key = XLookupKeysym(&e->xkey, 0);
                if (key == XK_Escape) {
                        return 1;
                }
        }
        return 0;
}

void title_physics(TitleAnim &t)
{
        if (t.timer < 1.0f)
                t.timer += 0.008f;
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
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
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

void physics()
{
        g.tex.yc[0] += 0.001;
        g.tex.yc[1] += 0.001;
        title_physics(g.title);
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
}
