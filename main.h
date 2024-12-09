#ifndef MAIN_H_
#define MAIN_H_

#include <raylib.h>
#include <stddef.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define WINDOW_PADDING 32

#define BULLET_VELOCITY 800

typedef struct {
  Texture2D *sprite;
  Rectangle *frames;
  size_t frame_count;
  size_t current;
} Animated_Sprite_t;

typedef struct {
  Rectangle rect;
  int velocity;
  bool active;
} Bullet_t;

typedef enum {
  New,
  Small_Damage,
  Hard_Damage,
  Destroyed
} Barrier_Level_t;

typedef struct {
  Rectangle rect;
  Barrier_Level_t level;
} Barrier_t;

#endif // MAIN_H_
