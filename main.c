#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define BULLET_COUNT 10
#define SHOOT_DELAY 300

#define ENEMY_PER_LINE 10
#define ENEMY_COUNT ENEMY_PER_LINE * 5
#define BARRIER_COUNT 4
#define BARRIER_WIDTH 70
#define BARRIER_HEIGHT 50

#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 8

typedef struct {
  Rectangle rect;
  bool    dead;
} Enemy_t;

typedef struct {
  Rectangle rect;
  int shooting_timer;
} Player_t;

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

typedef struct {
  Color background_color;
  Font  font;
  size_t score;
  unsigned char lives;
  char  status_text[255];
  Enemy_t *enemies;
  size_t enemy_count;
  Player_t player;
  Barrier_t *barriers;
  Bullet_t *bullets;
  size_t bullet_id;
} App_t;

void init_player(App_t *app) {
  app->player.rect = (Rectangle){
    (WINDOW_WIDTH - SPRITE_WIDTH) / 2,
    WINDOW_HEIGHT - 10 - SPRITE_HEIGHT,
    SPRITE_WIDTH,
    SPRITE_HEIGHT
  };
}

void init_barriers(App_t *app) {
  app->barriers = RL_CALLOC(BARRIER_COUNT, sizeof(Barrier_t));
  if (app->barriers == NULL) {
    printf("Couldn't allocate barriers data\n");
    exit(1);
  }

  int spacing = (WINDOW_WIDTH - (BARRIER_COUNT * BARRIER_WIDTH)) / (BARRIER_COUNT + 1);
  for(size_t i = 0; i < BARRIER_COUNT; i++) {
    app->barriers[i].rect = (Rectangle){
      (i + 1) * spacing + i * BARRIER_WIDTH,
      600,
      BARRIER_WIDTH,
      BARRIER_HEIGHT
    };
  }
}

void init_enemies(App_t *app) {
  app->enemies = RL_CALLOC(ENEMY_COUNT, sizeof(Enemy_t));
  if (app->enemies == NULL) {
    printf("Out of RAM?\n");
    exit(1);
  }

  // Add maybe some static initial offset to center enemies on screen
  int spacing = (WINDOW_WIDTH - 100 - (ENEMY_PER_LINE * SPRITE_WIDTH)) / (ENEMY_PER_LINE + 1);

  app->enemy_count = ENEMY_COUNT;
  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    app->enemies[i].rect = (Rectangle){
      ((i % ENEMY_PER_LINE) + 1) * spacing + (i % ENEMY_PER_LINE) * SPRITE_WIDTH,
      20 + ((i / ENEMY_PER_LINE) + 1) * spacing + (i / ENEMY_PER_LINE) * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
    };
  }
}

void init_app(App_t *app) {
  app->background_color = (Color){0x18, 0x18, 0x18, 0xFF};
  app->font = LoadFontEx("./arcade.ttf", 20, NULL, 0);

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);

  init_player(app);
  init_enemies(app);
  init_barriers(app);
}

void update(App_t *app) {

}

void render(App_t *app) {
  BeginDrawing();

  ClearBackground(app->background_color);

  DrawTextEx(app->font, app->status_text, (Vector2){10, 10}, 30, 1, WHITE);

  DrawRectangleRec(app->player.rect, (Color){255, 0, 0, 255});

  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    DrawRectangleRec(app->barriers[i].rect, (Color){0, 255, 0, 255});
  }

  for (size_t i = 0; i < ENEMY_COUNT; i++) {
    DrawRectangleRec(app->enemies[i].rect, (Color){255, 255, 255, 255});
  }

  EndDrawing();
}

int main(void)
{
  App_t app = {0};

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space invaders");
  SetTargetFPS(60);

  init_app(&app);

  while (!WindowShouldClose())
  {
    update(&app);
    render(&app);
  }

  CloseWindow();

  return 0;
}
