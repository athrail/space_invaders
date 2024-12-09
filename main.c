#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "main.h"
#include "player.h"
#include "enemy.h"

#define ARR_LEN(a) (sizeof(a)/sizeof(a[0]))
#define DEBUG(format, ...) printf("%s:%d:%s() " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define SHOOT_DELAY 0.7
#define BULLET_COUNT 10
#define SCORE_PER_KILL 100
#define STARTING_LIVES 3

#define BARRIER_COUNT 4
#define BARRIER_WIDTH 70.0
#define BARRIER_HEIGHT 50.0

#define SPRITE_WIDTH 8
#define SPRITE_HEIGHT 8

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
  Music music;
  Sound over;
  Sound shoot_sound;
  Sound hit_sound;
} App_t;

int frames_counter = 0;

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
  reset_player(&app->player);
  reset_enemies(app->enemies);
  reset_bullets(app);

  // SeekMusicStream(app->music, 0.0);
  PlayMusicStream(app->music);
  // SeekMusicStream(app->over, 0.0);

  sprintf(app->status_text, "Score   %08zu       Lives   %02d", app->score, app->lives);
  frames_counter = 0;
}

void init_app(App_t *app) {
  app->font = LoadFontEx("./PressStart2P.ttf", 20, NULL, 0);
  if (!IsFontValid(app->font)) {
    printf("ERROR: Couldn't load font!\n");
  }

  app->sprites = LoadTexture("sprites.png");
  if (!IsTextureValid(app->sprites)) {
    printf("Couldn't load sprites.\n");
    exit(1);
  }
  init_player(&app->sprites, &app->player);
  reset_player(&app->player);

  app->enemies = RL_CALLOC(ENEMY_COUNT, sizeof(Enemy_t));
  if (app->enemies == NULL) {
    printf("Out of RAM?\n");
    exit(1);
  }
  reset_enemies(app->enemies);

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

  app->music = LoadMusicStream("music.mp3");
  if (!IsMusicValid(app->music)) {
    printf("Couldn't load music.\n");
    exit(1);
  }

  app->over = LoadSound("over.mp3");
  if (!IsSoundValid(app->over)) {
    printf("Couldn't load over.\n");
    exit(1);
  }

  app->shoot_sound = LoadSound("laser.ogg");
  if (!IsSoundValid(app->shoot_sound)) {
    printf("ERROR: Cannot load shoot sound\n");
    exit(1);
  }

  app->hit_sound = LoadSound("impact.ogg");
  if (!IsSoundValid(app->hit_sound)) {
    printf("ERROR: Cannot load hit sound\n");
    exit(1);
  }
}

void restart(App_t *app, bool full) {
  if (full) {
    reset_context(app);
    return;
  }

  if (app->lives == 0) {
    StopMusicStream(app->music);
    PlaySound(app->over);
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

void handle_input(App_t *app) {
  player_handle_input(&app->player);

  if (IsKeyDown(KEY_SPACE)) {
    if (app->player.shooting_timer <= 0) {
      app->player.shooting_timer = SHOOT_DELAY;
      // todo: consider bullet width when centering
      Vector2 loc = (Vector2){app->player.rect.x + (app->player.rect.width - 5) / 2, app->player.rect.y - 10};
      spawn_bullet(app, loc, -BULLET_VELOCITY);
      PlaySound(app->shoot_sound);
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
          PlaySound(app->hit_sound);
          break;
        }
      }
    } else {
      if (CheckCollisionRecs(bullet->rect, app->player.rect)) {
        app->lives -= 1;
        restart(app, false);
        bullet->active = false;
        PlaySound(app->hit_sound);
        continue;
      }
    }
  }
}

void update(App_t *app) {
  float delta = GetFrameTime();

  if (app->player.shooting_timer) {
    app->player.shooting_timer -= delta;
  }

  handle_input(app);
  UpdateMusicStream(app->music);

  if (app->game_over) return;

  sprintf(app->status_text, "Score %08zu           Lives %02d", app->score, app->lives);

  move_player(&app->player, delta);
  move_bullets(app, delta);
  move_enemies(app->enemies, frames_counter, &app->moving_line, &app->shift_direction);
  Vector2 loc = enemy_shoot(app->enemies, frames_counter);
  if (loc.x != 0.0 && loc.y != 0.0) {
    spawn_bullet(app, loc, BULLET_VELOCITY);
    PlaySound(app->shoot_sound);
  }

  check_collisions(app);
}

void render(App_t *app) {
  BeginDrawing();

  ClearBackground(BLACK);

  if (app->game_over) {
    Vector2 text_size = MeasureTextEx(app->font, app->status_text, 16, 1);
    Vector2 pos = {
      (WINDOW_WIDTH - text_size.x) / 2.0,
      (WINDOW_HEIGHT - text_size.y) / 2.0,
    };
    DrawTextEx(app->font, app->status_text, pos, 16, 1, WHITE);
    goto end;
  }

  DrawTextEx(app->font, app->status_text, (Vector2){10, 10}, 16, 1, WHITE);

  render_player(&app->player);

  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    Barrier_t *barrier = &app->barriers[i];
    if (barrier->level < Destroyed) {
      DrawRectangleRec(barrier->rect, GREEN);
    }
  }

  render_enemies(&app->sprites, app->enemies);

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
  InitAudioDevice();

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
