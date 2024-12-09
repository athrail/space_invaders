#ifndef ENEMY_H_
#define ENEMY_H_

#include <raylib.h>

#include "main.h"

#define ENEMY_PER_LINE 10
#define ENEMY_LINES 5
#define ENEMY_COUNT ENEMY_PER_LINE * ENEMY_LINES
#define ENEMY_SPACING 20

typedef struct {
  Rectangle rect;
  bool    dead;
  Rectangle sprite_rect;
  Animated_Sprite_t animation;
} Enemy_t;

void reset_enemies(Enemy_t *enemies);
void init_enemies(Texture *sprite, Enemy_t *player);
void move_enemies(Enemy_t *enemies, int frames_counter, unsigned int *moving_line, int *shift_direction);
Vector2 enemy_shoot(Enemy_t *enemies, int frames_counter);
void render_enemies(Texture *sprite, Enemy_t *enemies);

#endif // ENEMY_H_
