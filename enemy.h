#ifndef ENEMY_H
#define ENEMY_H

#include <GL/gl.h>

class Enemy {
public:
    float x, y;
    float velx, vely;
    float speed;
    int width, height;
    int hp;
    int type;       // 0 = enemy.png, 1 = enemy2.png
    bool active;

    bool  exploding;
    int   explodeFrame;
    float explodeTimer;

    Enemy() :
        x(0.0f), y(0.0f),
        velx(0.0f), vely(0.0f),
        speed(2.0f),
        width(64), height(64),
        hp(1), type(0),
        active(false),
        exploding(false),
        explodeFrame(0),
        explodeTimer(0.0f) {}
};

void init_enemies();
void spawn_enemy(float playerX, float playerY, int screenW, int screenH);
void enemies_physics(float playerX, float playerY, int screenW, int screenH, float dt);
void render_enemies();

int  enemy_check_bullet_hit(float bx, float by, float br);
bool enemy_check_player_collision(float px, float py, float pr);

// shared explosion sprite for reuse by obstacles
GLuint get_explosion_texture();
int    get_explosion_frames();
float  get_explosion_frame_time();

// shared loader that preserves png alpha channel
void load_png_with_alpha(const char *fname, int *outW, int *outH, unsigned char **outData);

#endif
