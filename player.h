#ifndef PLAYER_H_
#define PLAYER_H_

#include <raylib.h>
#include <stddef.h>

#include "main.h"

typedef struct {
  Rectangle rect;
  float shooting_timer;
  float velocity;
  Animated_Sprite_t animation;
} Player_t;

void move_player(Player_t *player, float delta);
void init_player(Texture *sprite, Player_t *player);
void reset_player(Player_t *player);
void render_player(Player_t *player);
void player_handle_input(Player_t *player);

#endif // PLAYER_H_
