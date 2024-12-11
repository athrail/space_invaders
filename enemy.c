#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "enemy.h"
#include "main.h"

#define ENEMY_SPRITE_X 24.0
#define ENEMY_SPRITE_Y 0.0
#define ENEMY_SPRITE_WIDTH 8
#define ENEMY_SPRITE_HEIGHT 8
#define MOVE_FREQ 15
#define SHOOT_FREQ 60
#define ANIMATION_FRAMES_COUNT 2

size_t animation_frame = 0;

void init_enemies(Texture *sprite, Enemy_t *enemies) {
  memset(enemies, 0, ENEMY_COUNT * sizeof(Enemy_t));

  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    Enemy_t *enemy = &enemies[i];
    enemy->animation.sprite = sprite;
    enemy->animation.current = 0;

    enemy->animation.frames = RL_CALLOC(ANIMATION_FRAMES_COUNT, sizeof(Rectangle));
    if (enemy->animation.frames == NULL) {
      printf("ERROR: cannot allocate frames for enemy\n");
      exit(1);
    }

    for(size_t f = 0; f < ANIMATION_FRAMES_COUNT; f++) {
      Rectangle *frame = &enemy->animation.frames[f];
      frame->x = 24;
      frame->y = ENEMY_SPRITE_HEIGHT * f;
      frame->width = ENEMY_SPRITE_WIDTH;
      frame->height = ENEMY_SPRITE_HEIGHT;
    }
  }
}

bool are_enemies_dead(Enemy_t *enemies) {
  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    if (!enemies[i].dead) return false;
  }
  return true;
}

void render_enemies(Texture *sprite, Enemy_t *enemies) {
  for (size_t i = 0; i < ENEMY_COUNT; i++) {
    Enemy_t *enemy = &enemies[i];
    if (!enemy->dead) {
      Rectangle dest = {enemy->rect.x, enemy->rect.y, 32, 32};
      DrawTexturePro(*sprite, enemy->animation.frames[animation_frame], dest, (Vector2){0.0, 0.0}, 0.0, WHITE);
    }
  }
}

void update_enemies(Enemy_t *enemies, int frame_counter) {
  if (frame_counter % 25 == 0) {
    animation_frame = (animation_frame + 1) % ANIMATION_FRAMES_COUNT;
  }
}

void move_enemies(Enemy_t *enemies, int frames_counter, unsigned int *moving_line, int *shift_direction) {
  Enemy_t *enemy = NULL;

  if (frames_counter % MOVE_FREQ == 0) {
    for (size_t enemy_index = 0; enemy_index < ENEMY_PER_LINE; enemy_index++) {
      enemy = &enemies[*moving_line * ENEMY_PER_LINE + enemy_index];
      enemy->rect.x += (enemy->rect.width / 2.0) * *shift_direction;
    }

    *moving_line = (*moving_line + 1) % ENEMY_LINES;

    if (*moving_line == 0) {
      if ((enemies[ENEMY_PER_LINE-1].rect.x + enemies[ENEMY_PER_LINE-1].rect.width >= WINDOW_WIDTH - WINDOW_PADDING) ||
          (enemies[0].rect.x <= WINDOW_PADDING)) {
        *shift_direction *= -1;
      }
    }
  }
}

Vector2 enemy_shoot(Enemy_t *enemies, int frames_counter) {
  Enemy_t *enemy = NULL;

  if (frames_counter % SHOOT_FREQ == 0) {
    // todo: potentially infinite loop if all enemies are dead?
    do {
      enemy = &enemies[GetRandomValue(0, ENEMY_COUNT - 1)];
      if (enemy->dead) continue;

      return (Vector2){enemy->rect.x + 14, enemy->rect.y + 42};
    } while(true);
  }

  return (Vector2){0.0, 0.0};
}

void reset_enemies(Enemy_t *enemies) {
  animation_frame = 0;
  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    enemies[i].rect = (Rectangle){
      WINDOW_PADDING + (i % ENEMY_PER_LINE) * ENEMY_SPACING + (i % ENEMY_PER_LINE) * 32,
      80 + (i / ENEMY_PER_LINE) * ENEMY_SPACING + (i / ENEMY_PER_LINE) * 32,
      32,
      32
    };
    enemies[i].dead = false;
  }
}
