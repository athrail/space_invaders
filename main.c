#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>

#define ARR_LEN(a) (sizeof(a)/sizeof(a[0]))
#define DEBUG(format, ...) printf("%s:%d:%s() " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define WINDOW_PADDING 32

#define BULLET_COUNT 10
#define SHOOT_DELAY 0.7
#define BULLET_VELOCITY 800
#define PLAYER_VELOCITY 300
#define SCORE_PER_KILL 100
#define MOVE_FREQ 15
#define SHOOT_FREQ 60
#define STARTING_LIVES 3

#define ENEMY_PER_LINE 10
#define ENEMY_LINES 5
#define ENEMY_COUNT ENEMY_PER_LINE * ENEMY_LINES
#define ENEMY_SPACING 2
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
  bool game_over;
} App_t;

int frames_counter = 0;

void reset_player(App_t *app) {
  app->player.shooting_timer = 0;

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

void reset_barriers(App_t *app) {
  memset(app->barriers, 0, BARRIER_COUNT * sizeof(Barrier_t));

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

void reset_enemies(App_t *app) {
  memset(app->enemies, 0, ENEMY_COUNT * sizeof(Enemy_t));

  for(size_t i = 0; i < ENEMY_COUNT; i++) {
    app->enemies[i].rect = (Rectangle){
      WINDOW_PADDING + (i % ENEMY_PER_LINE) + (i % ENEMY_PER_LINE) * SPRITE_WIDTH,
      80 + (i / ENEMY_PER_LINE) * 20 + (i / ENEMY_PER_LINE) * SPRITE_HEIGHT,
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

void reset_bullets(App_t *app) {
  memset(app->bullets, 0, BULLET_COUNT * sizeof(Bullet_t));
}

void reset_context(App_t *app) {
  app->shift_direction = 1;
  app->moving_line = 0;
  app->lives = STARTING_LIVES;
  app->score = 0;
  app->game_over = false;
  app->bullet_id = 0;

  reset_barriers(app);
  reset_player(app);
  reset_enemies(app);
  reset_bullets(app);

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);
  frames_counter = 0;
}

void init_app(App_t *app) {
  app->font = LoadFontEx("./arcade.ttf", 20, NULL, 0);
  if (!IsFontValid(app->font)) {
    printf("ERROR: Couldn't load font!\n");
  }

  reset_player(app);

  app->enemies = RL_CALLOC(ENEMY_COUNT, sizeof(Enemy_t));
  if (app->enemies == NULL) {
    printf("Out of RAM?\n");
    exit(1);
  }
  reset_enemies(app);

  app->barriers = RL_CALLOC(BARRIER_COUNT, sizeof(Barrier_t));
  if (app->barriers == NULL) {
    printf("Couldn't allocate barriers data\n");
    exit(1);
  }
  reset_barriers(app);

  app->bullets = RL_CALLOC(BULLET_COUNT, sizeof(Bullet_t));
  if (app->bullets == NULL) {
    printf("Couldn't allocate memory for bullets...\n");
    exit(1);
  }
  reset_bullets(app);

  app->sprites = LoadTexture("sprite.png");
  if (!IsTextureValid(app->sprites)) {
    printf("Couldn't load sprites.\n");
    exit(1);
  }
}

void restart(App_t *app, bool full) {
  if (full) {
    reset_context(app);
    return;
  }

  if (app->lives == 0) {
    app->game_over = true;
    memset(app->status_text, 0, sizeof(app->status_text));
    sprintf(app->status_text, "Game Over!\nYou scored: %08zu\nPress R to restart game...", app->score);
    return;
  }
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

  if (app->game_over && IsKeyDown(KEY_R)) {
    restart(app, true);
  }
}

void move_bullets(App_t *app, float delta) {
  Bullet_t *bullet = NULL;

  for (size_t i = 0; i < BULLET_COUNT; i++) {
    bullet = &app->bullets[i];
    if (!bullet->active) continue;

    bullet->rect.y += (bullet->velocity * delta);
    if ((bullet->rect.y > WINDOW_HEIGHT) || (bullet->rect.y < 0)) {
      bullet->active = false;
    }
  }
}

void move_enemies(App_t *app) {
  Enemy_t *enemy = NULL;

  if (frames_counter % MOVE_FREQ == 0) {
    for (size_t enemy_index = 0; enemy_index < ENEMY_PER_LINE; enemy_index++) {
      enemy = &app->enemies[app->moving_line * ENEMY_PER_LINE + enemy_index];
      enemy->rect.x += (enemy->rect.width / 2.0) * app->shift_direction;
    }

    app->moving_line = (app->moving_line + 1) % ENEMY_LINES;

    if (app->moving_line == 0) {
      if ((app->enemies[ENEMY_PER_LINE-1].rect.x + app->enemies[ENEMY_PER_LINE-1].rect.width >= WINDOW_WIDTH - WINDOW_PADDING) ||
          (app->enemies[0].rect.x <= WINDOW_PADDING)) {
        app->shift_direction *= -1;
      }
    }
  }
}

void check_collisions(App_t *app) {
  Bullet_t *bullet = NULL;
  Barrier_t *barrier = NULL;
  Enemy_t *enemy = NULL;

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
      if (CheckCollisionRecs(bullet->rect, app->player.rect)) {
        app->lives -= 1;
        restart(app, false);
        bullet->active = false;
        continue;
      }
    }
  }
}

void enemy_shoot(App_t *app) {
  Enemy_t *enemy = NULL;

  if (frames_counter % SHOOT_FREQ == 0) {
    // todo: potentially infinite loop if all enemies are dead?
    do {
      enemy = &app->enemies[rand() % ENEMY_COUNT];
      if (enemy->dead) continue;

      Vector2 loc = (Vector2){enemy->rect.x + (enemy->rect.width - 5) / 2, enemy->rect.y + enemy->rect.height + 10};
      spawn_bullet(app, loc, BULLET_VELOCITY / 2.0);
      break;
    } while(true);
  }
}

void update(App_t *app) {
  float delta = GetFrameTime();

  if (app->player.shooting_timer) {
    app->player.shooting_timer -= delta;
  }

  handle_input(app, delta);

  if (app->game_over) return;

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);

  move_bullets(app, delta);
  move_enemies(app);
  check_collisions(app);
  enemy_shoot(app);

}

void render(App_t *app) {
  BeginDrawing();

  ClearBackground(BLACK);

  if (app->game_over) {
    char text[255] = {0};

    Vector2 text_size = MeasureTextEx(app->font, app->status_text, 32, 12);
    Vector2 pos = {
      (WINDOW_WIDTH - text_size.x) / 2.0,
      (WINDOW_HEIGHT - text_size.y) / 2.0,
    };
    DrawTextEx(app->font, app->status_text, pos, 32, 12, WHITE);
    goto end;
  }

  DrawTextEx(app->font, app->status_text, (Vector2){10, 10}, 30, 1, WHITE);

  Rectangle dest = {app->player.rect.x, app->player.rect.y, SPRITE_WIDTH, SPRITE_HEIGHT};
  DrawTexturePro(app->sprites, app->player.sprite_rect, dest, (Vector2){0.0, 0.0}, 0.0, WHITE);

  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    Barrier_t *barrier = &app->barriers[i];
    if (barrier->level < Destroyed) {
      DrawRectangleRec(barrier->rect, GREEN);
    }
  }

  for (size_t i = 0; i < ENEMY_COUNT; i++) {
    Enemy_t *enemy = &app->enemies[i];
    if (!enemy->dead) {
      Rectangle dest = {enemy->rect.x, enemy->rect.y, SPRITE_WIDTH, SPRITE_HEIGHT};
      DrawTexturePro(app->sprites, enemy->sprite_rect, dest, (Vector2){0.0, 0.0}, 0.0, WHITE);
    }
  }

  for (size_t i = 0; i < BULLET_COUNT; i++) {
    Bullet_t *bullet = &app->bullets[i];
    if (bullet->active) {
      DrawRectangleRec(bullet->rect, WHITE);
    }
  }
end:
  EndDrawing();
}

int main(void) {
  App_t app = {0};

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space invaders");
  SetTargetFPS(60);

  init_app(&app);
  reset_context(&app);

  while (!WindowShouldClose())
  {
    frames_counter++;

    update(&app);
    render(&app);

    frames_counter %= 255;
  }

  CloseWindow();

  return 0;
}
