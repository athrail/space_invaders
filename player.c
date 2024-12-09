#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "player.h"

#define PLAYER_SPRITE_WIDTH 8
#define PLAYER_SPRITE_HEIGHT 8
#define ANIMATION_FRAMES_COUNT 3
#define PLAYER_VELOCITY 300

void init_player(Texture *sprite, Player_t *player) {
  memset(player, 0, sizeof(Player_t));

  player->animation.sprite = sprite;

  player->animation.frames = RL_CALLOC(ANIMATION_FRAMES_COUNT, sizeof(Rectangle));
  if (player->animation.frames == NULL) {
    printf("ERROR: cannot allocate frames for player\n");
    exit(1);
  }

  for(size_t i = 0; i < ANIMATION_FRAMES_COUNT; i++) {
    Rectangle *frame = &player->animation.frames[i];
    frame->x = PLAYER_SPRITE_WIDTH * i;
    frame->y = 0;
    frame->width = PLAYER_SPRITE_WIDTH;
    frame->height = PLAYER_SPRITE_HEIGHT;
  }
}

void move_player(Player_t *player, float delta) {
  player->rect.x += player->velocity * delta;
}

void player_handle_input(Player_t *player) {
  player->velocity = 0;

  if (IsKeyDown(KEY_LEFT)) {
    player->velocity = -PLAYER_VELOCITY;
  }

  if (IsKeyDown(KEY_RIGHT)) {
    player->velocity = PLAYER_VELOCITY;
  }

  if (player->velocity == 0.0) {
    player->animation.current = 0;
  } else if (player->velocity < 0) {
    player->animation.current = 1;
  } else if (player->velocity > 0) {
    player->animation.current = 2;
  }
}

void render_player(Player_t *player) {
  Animated_Sprite_t *anim = &player->animation;
  Rectangle dest = {player->rect.x, player->rect.y, 32, 32};
  DrawTexturePro(*anim->sprite, anim->frames[anim->current], dest, (Vector2){0.0, 0.0}, 0.0, WHITE);
}

void reset_player(Player_t *player) {
  player->shooting_timer = 0;

  player->rect = (Rectangle){
    (WINDOW_WIDTH - PLAYER_SPRITE_WIDTH) / 2.0,
    WINDOW_HEIGHT - 32 - PLAYER_SPRITE_HEIGHT,
    32, 32
  };

  player->animation.current = 0;
}
