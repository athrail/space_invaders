#ifndef ENEMY_H_
#define ENEMY_H_

#include <raylib.h>
#include <stdbool.h>

#include "main.h"

#define ENEMY_PER_LINE 2
#define ENEMY_LINES 2
#define ENEMY_COUNT ENEMY_PER_LINE * ENEMY_LINES
#define ENEMY_SPACING 20

typedef struct {
  Rectangle rect;
  bool dead;
  Animated_Sprite_t animation;
} Enemy_t;

void reset_enemies(Enemy_t *enemies);
void init_enemies(Texture *sprite, Enemy_t *enemies);
void move_enemies(Enemy_t *enemies, int frames_counter, unsigned int *moving_line, int *shift_direction);
Vector2 enemy_shoot(Enemy_t *enemies, int frames_counter);
void render_enemies(Texture *sprite, Enemy_t *enemies);
void update_enemies(Enemy_t *enemies, int frame_counter);
bool are_enemies_dead(Enemy_t *enemies);

#endif // ENEMY_H_
