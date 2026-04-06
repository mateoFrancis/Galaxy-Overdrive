// fmateo.cpp
#include <iostream>

struct Bullet {
    float x, y;
    float vel;
    int type;
    int frame;
    float frameTimer;
    bool active;
};

struct WeaponState {
    int currentWeapon;
    float weaponTimer;
    int weaponFrame;
};

struct GameState {
    int mousex, mousey;
    int yres;
    bool spacePressed;
    Bullet bullets[30];
    WeaponState weapon;
};

extern void f_physics(GameState &g);
extern void updateBullets(GameState &g);
extern void fireBullets(GameState &g);
extern void updateWeapon(GameState &g);

// weapon animation
void updateWeapon(GameState &g) {
    if (g.spacePressed) {
        g.weapon.weaponTimer += 0.2f;

        int maxFrames;
        switch (g.weapon.currentWeapon) {
            case 0: maxFrames = 7;  break;
            case 1: maxFrames = 16; break;
            case 2: maxFrames = 12; break;
            case 3: maxFrames = 14; break;
            default: maxFrames = 7; break;
        }

        if (g.weapon.weaponTimer >= 1.0f) {
            g.weapon.weaponTimer = 0.0f;
            g.weapon.weaponFrame++;
            if (g.weapon.weaponFrame >= maxFrames) g.weapon.weaponFrame = 1;
        }
    } else {
        g.weapon.weaponFrame = 0;
    }
}

// fire bullets
void fireBullets(GameState &g) {

    static float fireCooldown = 0.0f;
    float fireRate;
    switch (g.weapon.currentWeapon) {
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

                    g.bullets[i].active = true;
                    g.bullets[i].x = g.mousex;
                    g.bullets[i].y = g.mousey + 20;
                    g.bullets[i].type = g.weapon.currentWeapon;
                    g.bullets[i].frame = 0;
                    g.bullets[i].frameTimer = 0.0f;

                    switch (g.weapon.currentWeapon) {

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
}

// update bullets
void updateBullets(GameState &g) {
    for (int i = 0; i < 30; i++) {
        if (g.bullets[i].active) {
            g.bullets[i].y += g.bullets[i].vel;

            if (g.bullets[i].y > g.yres) {
                g.bullets[i].active = false;
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
}

// main physics 
void f_physics(GameState &g) {

    updateWeapon(g);
    fireBullets(g);
    updateBullets(g);
}