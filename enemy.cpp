#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "enemy.h"

class EnemyImage {
public:
    int width, height;
    unsigned char *data;

    EnemyImage() : width(0), height(0), data(NULL) {}

    void load(const char *fname) {
        if (!fname || fname[0] == '\0')
            return;

        char name[128];
        snprintf(name, sizeof(name), "%s", fname);
        int slen = strlen(name);
        if (slen < 5) {
            printf("bad image filename: %s\n", fname);
            return;
        }

        name[slen - 4] = '\0';
        char ppmname[160];
        snprintf(ppmname, sizeof(ppmname), "%s.ppm", name);

        char ts[512];
        snprintf(ts, sizeof(ts), "convert %s %s", fname, ppmname);
        system(ts);

        FILE *fpi = fopen(ppmname, "rb");
        if (!fpi) {
            printf("ERROR opening image: %s\n", ppmname);
            return;
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
            unsigned char r = fgetc(fpi);
            unsigned char g = fgetc(fpi);
            unsigned char b = fgetc(fpi);
            data[i * 4 + 0] = r;
            data[i * 4 + 1] = g;
            data[i * 4 + 2] = b;

            if (r < 50 && g < 50 && b < 50)
                data[i * 4 + 3] = 0;
            else
                data[i * 4 + 3] = 255;
        }

        fclose(fpi);
        unlink(ppmname);
    }

    ~EnemyImage() {
        delete [] data;
    }
};

// load png keeping its real alpha channel
void load_png_with_alpha(const char *fname, int *outW, int *outH, unsigned char **outData)
{
    *outW = 0; *outH = 0; *outData = NULL;
    if (!fname || fname[0] == '\0') return;

    char name[128];
    snprintf(name, sizeof(name), "%s", fname);
    int slen = strlen(name);
    if (slen < 5) return;
    name[slen - 4] = '\0';

    char ppmname[160], pgmname[160];
    snprintf(ppmname, sizeof(ppmname), "%s.ppm", name);
    snprintf(pgmname, sizeof(pgmname), "%s_a.pgm", name);

    char ts[512];
    snprintf(ts, sizeof(ts), "convert %s %s", fname, ppmname);
    system(ts);
    snprintf(ts, sizeof(ts), "convert %s -alpha extract %s", fname, pgmname);
    system(ts);

    FILE *fpi = fopen(ppmname, "rb");
    if (!fpi) { printf("ERROR opening %s\n", ppmname); return; }

    char line[200];
    fgets(line, 200, fpi);
    fgets(line, 200, fpi);
    while (line[0] == '#') fgets(line, 200, fpi);
    int w, h;
    sscanf(line, "%i %i", &w, &h);
    fgets(line, 200, fpi);

    unsigned char *data = new unsigned char[w * h * 4];
    for (int i = 0; i < w * h; i++) {
        data[i*4+0] = fgetc(fpi);
        data[i*4+1] = fgetc(fpi);
        data[i*4+2] = fgetc(fpi);
        data[i*4+3] = 255;
    }
    fclose(fpi);

    FILE *fpa = fopen(pgmname, "rb");
    if (fpa) {
        fgets(line, 200, fpa);
        fgets(line, 200, fpa);
        while (line[0] == '#') fgets(line, 200, fpa);
        int aw, ah;
        sscanf(line, "%i %i", &aw, &ah);
        fgets(line, 200, fpa);
        if (aw == w && ah == h) {
            for (int i = 0; i < w * h; i++)
                data[i*4+3] = fgetc(fpa);
        }
        fclose(fpa);
        unlink(pgmname);
    }
    unlink(ppmname);

    *outW = w;
    *outH = h;
    *outData = data;
}

static EnemyImage enemyImg1;
static EnemyImage enemyImg2;
static GLuint     enemyTex1;
static GLuint     enemyTex2;
static GLuint     explosionTex;
static int        explosionW = 0, explosionH = 0;

const int MAX_ENEMIES = 20;
Enemy enemies[MAX_ENEMIES];

static const int   EXPLOSION_FRAMES      = 6;
static const float EXPLOSION_FRAME_TIME  = 0.08f;

GLuint get_explosion_texture()     { return explosionTex; }
int    get_explosion_frames()      { return EXPLOSION_FRAMES; }
float  get_explosion_frame_time()  { return EXPLOSION_FRAME_TIME; }

static void upload_enemy_tex(GLuint *tex, EnemyImage *img)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        img->width, img->height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
}

void init_enemies()
{
    enemyImg1.load("enemy.png");
    enemyImg2.load("enemy2.png");
    upload_enemy_tex(&enemyTex1, &enemyImg1);
    upload_enemy_tex(&enemyTex2, &enemyImg2);

    // load explosion with real alpha
    unsigned char *expData = NULL;
    load_png_with_alpha("explosion.png", &explosionW, &explosionH, &expData);
    if (expData) {
        glGenTextures(1, &explosionTex);
        glBindTexture(GL_TEXTURE_2D, explosionTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, explosionW, explosionH,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, expData);
        delete[] expData;
    }
}

static void start_explosion(Enemy &e)
{
    e.exploding     = true;
    e.explodeFrame  = 0;
    e.explodeTimer  = 0.0f;
    e.velx          = 0.0f;
    e.vely          = 0.0f;
}

void spawn_enemy(float playerX, float playerY, int screenW, int screenH)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].active = true;
            enemies[i].exploding = false;
            enemies[i].explodeFrame = 0;
            enemies[i].explodeTimer = 0.0f;

            enemies[i].type = rand() % 2;

            enemies[i].width = 64;
            enemies[i].height = 64;

            if (enemies[i].type == 1) {
                enemies[i].speed = 3.2f;
                enemies[i].hp = 1;
            } else {
                enemies[i].speed = 2.0f;
                enemies[i].hp = 2;
            }

            int side = rand() % 4;
            if (side == 0) {
                enemies[i].x = -enemies[i].width;
                enemies[i].y = rand() % screenH;
            } else if (side == 1) {
                enemies[i].x = screenW + enemies[i].width;
                enemies[i].y = rand() % screenH;
            } else if (side == 2) {
                enemies[i].x = rand() % screenW;
                enemies[i].y = screenH + enemies[i].height;
            } else {
                enemies[i].x = rand() % screenW;
                enemies[i].y = -enemies[i].height;
            }

            float dx = playerX - enemies[i].x;
            float dy = playerY - enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist > 0.0f) {
                enemies[i].velx = (dx / dist) * enemies[i].speed;
                enemies[i].vely = (dy / dist) * enemies[i].speed;
            } else {
                enemies[i].velx = 0.0f;
                enemies[i].vely = 0.0f;
            }
            return;
        }
    }
}

void enemies_physics(float playerX, float playerY, int screenW, int screenH, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active)
            continue;

        if (enemies[i].exploding) {
            enemies[i].explodeTimer += dt;
            if (enemies[i].explodeTimer >= EXPLOSION_FRAME_TIME) {
                enemies[i].explodeTimer = 0.0f;
                enemies[i].explodeFrame++;
                if (enemies[i].explodeFrame >= EXPLOSION_FRAMES) {
                    enemies[i].active = false;
                }
            }
            continue;
        }

        float dx = playerX - enemies[i].x;
        float dy = playerY - enemies[i].y;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist > 0.0f) {
            enemies[i].velx = (dx / dist) * enemies[i].speed;
            enemies[i].vely = (dy / dist) * enemies[i].speed;
        }

        enemies[i].x += enemies[i].velx;
        enemies[i].y += enemies[i].vely;

        if (enemies[i].x < -200 || enemies[i].x > screenW + 200 ||
            enemies[i].y < -200 || enemies[i].y > screenH + 200) {
            enemies[i].active = false;
        }
    }
}

int enemy_check_bullet_hit(float bx, float by, float br)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].exploding) continue;

        float er = enemies[i].width * 0.4f;
        float dx = bx - enemies[i].x;
        float dy = by - enemies[i].y;
        float rsum = er + br;

        if (dx*dx + dy*dy < rsum*rsum) {
            enemies[i].hp--;
            if (enemies[i].hp <= 0)
                start_explosion(enemies[i]);
            return i;
        }
    }
    return -1;
}

bool enemy_check_player_collision(float px, float py, float pr)
{
    bool hit = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].exploding) continue;

        float er = enemies[i].width * 0.4f;
        float dx = px - enemies[i].x;
        float dy = py - enemies[i].y;
        float rsum = er + pr;

        if (dx*dx + dy*dy < rsum*rsum) {
            start_explosion(enemies[i]);
            hit = true;
        }
    }
    return hit;
}

void render_enemies()
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active)
            continue;

        float x = enemies[i].x;
        float y = enemies[i].y;
        float w = (float)enemies[i].width / 2.0f;
        float h = (float)enemies[i].height / 2.0f;

        if (enemies[i].exploding) {
            float frameW = 1.0f / (float)EXPLOSION_FRAMES;
            float tx0 = frameW * (float)enemies[i].explodeFrame;
            float tx1 = tx0 + frameW;

            glBindTexture(GL_TEXTURE_2D, explosionTex);
            glPushMatrix();
            glTranslatef(x, y, 0.0f);
            glBegin(GL_QUADS);
                glTexCoord2f(tx0, 1.0f); glVertex2f(-w, -h);
                glTexCoord2f(tx0, 0.0f); glVertex2f(-w,  h);
                glTexCoord2f(tx1, 0.0f); glVertex2f( w,  h);
                glTexCoord2f(tx1, 1.0f); glVertex2f( w, -h);
            glEnd();
            glPopMatrix();
            continue;
        }

        float degrees = atan2(enemies[i].vely, enemies[i].velx) * (180.0f / M_PI);

        glBindTexture(GL_TEXTURE_2D,
            (enemies[i].type == 1) ? enemyTex2 : enemyTex1);

        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glRotatef(degrees - 90.0f, 0.0f, 0.0f, 1.0f);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -h);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-w,  h);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( w,  h);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -h);
        glEnd();

        glPopMatrix();
    }
}
