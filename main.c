#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define BULLET_COUNT 10
#define SHOOT_DELAY 1.5
#define BULLET_VELOCITY 200
#define PLAYER_VELOCITY 200
#define SCORE_PER_KILL 100

#define ENEMY_PER_LINE 10
#define ENEMY_LINES 5
#define ENEMY_COUNT ENEMY_PER_LINE * ENEMY_LINES
#define BARRIER_COUNT 4
#define BARRIER_WIDTH 70.0
#define BARRIER_HEIGHT 50.0

#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 8
#define PLAYER_SPRITE_X 1.0
#define PLAYER_SPRITE_Y 49.0
#define ENEMY_SPRITE_X 1.0
#define ENEMY_SPRITE_Y 1.0

typedef struct {
  Rectangle rect;
  bool    dead;
  Rectangle sprite_rect;
} Enemy_t;

typedef struct {
  Rectangle rect;
  float shooting_timer;
  Rectangle sprite_rect;
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
  Font  font;
  size_t score;
  unsigned char lives;
  char  status_text[255];
  Enemy_t *enemies;
  Player_t player;
  Barrier_t *barriers;
  Bullet_t *bullets;
  size_t bullet_id;
  Texture2D sprites;
  unsigned int moving_line;
  int shift_direction;
} App_t;

int frames_counter = 0;

void init_player(App_t *app) {
  app->player.rect = (Rectangle){
    (WINDOW_WIDTH - SPRITE_WIDTH) / 2,
    WINDOW_HEIGHT - 10 - SPRITE_HEIGHT,
    SPRITE_WIDTH,
    SPRITE_HEIGHT
  };

  app->player.sprite_rect = (Rectangle){
    PLAYER_SPRITE_X,
    PLAYER_SPRITE_Y,
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

  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    app->enemies[i].rect = (Rectangle){
      ((i % ENEMY_PER_LINE) + 1) * spacing + (i % ENEMY_PER_LINE) * SPRITE_WIDTH,
      20 + ((i / ENEMY_PER_LINE) + 1) * spacing + (i / ENEMY_PER_LINE) * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
    };

    app->enemies[i].sprite_rect = (Rectangle){
      ENEMY_SPRITE_X,
      ENEMY_SPRITE_Y,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
    };
  }
}


void init_app(App_t *app) {
  app->font = LoadFontEx("./arcade.ttf", 20, NULL, 0);
  if (!IsFontValid(app->font)) {
    printf("ERROR: Couldn't load font!\n");
  }

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);

  init_player(app);
  init_enemies(app);
  init_barriers(app);

  app->bullets = RL_CALLOC(BULLET_COUNT, sizeof(Bullet_t));
  if (app->bullets == NULL) {
    printf("Couldn't allocate memory for bullets...\n");
    exit(1);
  }

  app->sprites = LoadTexture("sprite.png");
  if (!IsTextureValid(app->sprites)) {
    printf("Couldn't load sprites.\n");
    exit(1);
  }

  app->shift_direction = 1;
}

void spawn_bullet(App_t *app, Vector2 loc, int velocity) {
  app->bullets[app->bullet_id].active = true;
  app->bullets[app->bullet_id].rect = (Rectangle){loc.x, loc.y, 5, 10};
  app->bullets[app->bullet_id].velocity = velocity;
  app->bullet_id = (app->bullet_id + 1) % BULLET_COUNT;
}

void handle_input(App_t *app, float delta) {
  if (IsKeyDown(KEY_LEFT)) {
    app->player.rect.x = app->player.rect.x > 0 ? app->player.rect.x - (PLAYER_VELOCITY * delta) : 0;
  }
  if (IsKeyDown(KEY_RIGHT)) {
    app->player.rect.x = app->player.rect.x < WINDOW_WIDTH - SPRITE_WIDTH ? app->player.rect.x + (PLAYER_VELOCITY * delta) : WINDOW_WIDTH - SPRITE_WIDTH;
  }
  if (IsKeyDown(KEY_SPACE)) {
    if (app->player.shooting_timer <= 0) {
      app->player.shooting_timer = SHOOT_DELAY;
      // todo: consider bullet width when centering
      Vector2 loc = (Vector2){app->player.rect.x + (app->player.rect.width - 5) / 2, app->player.rect.y - 10};
      spawn_bullet(app, loc, -BULLET_VELOCITY);
    }
  }
}

void update(App_t *app) {
  float delta = GetFrameTime();
  Bullet_t *bullet = NULL;
  Barrier_t *barrier = NULL;
  Enemy_t *enemy = NULL;

  handle_input(app, delta);

  if (app->player.shooting_timer) {
    app->player.shooting_timer -= delta;
  }

  for (size_t i = 0; i < BULLET_COUNT; i++) {
    bullet = &app->bullets[i];
    if (!bullet->active) continue;

    bullet->rect.y += (bullet->velocity * delta);
    if ((bullet->rect.y > WINDOW_HEIGHT) || (bullet->rect.y < 0)) {
      bullet->active = false;
    }
  }

  // Enemy movement logic
  if (frames_counter > 30) {
    frames_counter = 0;

    if (app->shift_direction > 0) {
      for (size_t i = ENEMY_PER_LINE - 1; i >= 0; i--) {
        enemy = &app->enemies[app->moving_line * ENEMY_PER_LINE + i];
        if (enemy->dead) continue;

        float destination = enemy->rect.x + SPRITE_WIDTH * app->shift_direction;
        if (destination > WINDOW_WIDTH) {
          app->shift_direction *= -1;
          break;
        }
      }
    } else {
      for (size_t i = 0; i < ENEMY_PER_LINE; i++) {
        enemy = &app->enemies[app->moving_line * ENEMY_PER_LINE + i];
        if (enemy->dead) continue;

        float destination = enemy->rect.x + SPRITE_WIDTH * app->shift_direction;
        if (destination < 0) {
          app->shift_direction *= -1;
          break;
        }
      }
    }

    for (size_t i = 0; i < ENEMY_PER_LINE; i++) {
      enemy = &app->enemies[app->moving_line * ENEMY_PER_LINE + i];
      if (enemy->dead) continue;
      enemy->rect.x += SPRITE_WIDTH * app->shift_direction;
    }

    app->moving_line = (app->moving_line + 1) % ENEMY_LINES;
  }

  // Check bullet collisions between player, enemies or barriers
  for (size_t i = 0; i < BULLET_COUNT; i++) {
    bullet = &app->bullets[i];
    if (!bullet->active) continue;

    for (size_t barrier_index = 0; barrier_index < BARRIER_COUNT; barrier_index++) {
      barrier = &app->barriers[barrier_index];
      if ((barrier->level < Destroyed) && CheckCollisionRecs(barrier->rect, bullet->rect)) {
        barrier->level += 1;
        barrier->rect.height -= (float)BARRIER_HEIGHT / (float)Destroyed;
        barrier->rect.y += (float)BARRIER_HEIGHT / (float)Destroyed;
        bullet->active = false;
        continue;
      }
    }

    if (bullet->velocity < 0) {
      for (size_t enemy_index = 0; enemy_index < ENEMY_COUNT; enemy_index++) {
        enemy = &app->enemies[enemy_index];
        if (enemy->dead) continue;

        if (CheckCollisionRecs(enemy->rect, bullet->rect)) {
          enemy->dead = true;
          bullet->active = false;
          app->score += SCORE_PER_KILL;
          break;
        }
      }
    } else {
      // Player collision detection
    }
  }

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);
}

void render(App_t *app) {
  BeginDrawing();

  ClearBackground(BLACK);

  DrawTextEx(app->font, app->status_text, (Vector2){10, 10}, 30, 1, WHITE);

  Vector2 pos = {app->player.rect.x, app->player.rect.y};
  DrawTextureRec(app->sprites, app->player.sprite_rect, pos, WHITE);

  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    Barrier_t *barrier = &app->barriers[i];
    if (barrier->level < Destroyed) {
      DrawRectangleRec(barrier->rect, GREEN);
    }
  }

  for (size_t i = 0; i < ENEMY_COUNT; i++) {
    Enemy_t *enemy = &app->enemies[i];
    if (!enemy->dead) {
      Vector2 pos = {enemy->rect.x, enemy->rect.y};
      DrawTextureRec(app->sprites, enemy->sprite_rect, pos, WHITE);
    }
  }

  for (size_t i = 0; i < BULLET_COUNT; i++) {
    Bullet_t *bullet = &app->bullets[i];
    if (bullet->active) {
      DrawRectangleRec(bullet->rect, WHITE);
    }
  }

  EndDrawing();
}

int main(void) {
  App_t app = {0};

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space invaders");
  SetTargetFPS(60);

  init_app(&app);

  while (!WindowShouldClose())
  {
    frames_counter++;

    update(&app);
    render(&app);
  }

  CloseWindow();

  return 0;
}
