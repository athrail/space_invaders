#include <SDL2/SDL_rect.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define ARR_LEN(a) (sizeof(a)/sizeof(a[0]))
#define DEBUG(format, ...) printf("%s:%d:%s() " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define FRAME_SLEEP 16
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define BULLET_COUNT 10

#define ENEMY_WIDTH 30
#define ENEMY_HEIGHT 20
#define ENEMY_PER_LINE 10
#define ENEMY_COUNT ENEMY_PER_LINE * 5

#define BARRIER_COUNT 4
#define BARRIER_WIDTH 70
#define BARRIER_HEIGHT 50

#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 30
#define SHOOT_DELAY 300

typedef struct {
  SDL_Rect rect;
  bool     dead;
} Enemy_t;

typedef struct {
  SDL_Rect rect;
  int shooting_timer;
} Player_t;

typedef struct {
  SDL_Rect rect;
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
  SDL_Rect rect;
  Barrier_Level_t level;
} Barrier_t;

typedef struct {
  SDL_Renderer *renderer;
  SDL_Window   *window;
  Enemy_t      *enemies;
  size_t       enemy_count;
  Player_t     *player;
  Barrier_t    *barriers;
  TTF_Font     *sans;
  unsigned int score;
  SDL_Texture  *score_texture;
  Bullet_t     *bullets;
  unsigned int bullet_id;
} App_t;

void init_sdl(App_t *app) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() < 0) {
    printf("Couldn't initialize SDL_TTF: %s\n", SDL_GetError());
    exit(1);
  }

  app->window = SDL_CreateWindow("Not space invaders", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (!app->window) {
    printf("Couldn't create window: %s\n", SDL_GetError());
    exit(1);
  }

  app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
  if (!app->renderer) {
    printf("Couldn't create renderer: %s\n", SDL_GetError());
    exit(1);
  }

  app->sans = TTF_OpenFont("arcade.ttf", 24);
  if (app->sans == NULL) {
    printf("Couldn't load arcade.ttf font: %s\n", SDL_GetError());
    exit(1);
  }
}

void update_score(App_t *app, int change) {
  app->score += change;
  char *buf = malloc(sizeof(char) * 255);
  memset(buf, 0, sizeof(char) * 255);
  sprintf(buf, "Score   %08d", app->score);
  SDL_Surface *score_surface = TTF_RenderText_Solid(app->sans, buf, (SDL_Color){255, 255, 255});
  app->score_texture = SDL_CreateTextureFromSurface(app->renderer, score_surface);
}

void spawn_bullet(App_t *app, SDL_Point loc, int velocity) {
  DEBUG("spawn_bullet with id: %d", app->bullet_id);
  app->bullets[app->bullet_id].active = true;
  app->bullets[app->bullet_id].rect = (SDL_Rect){loc.x, loc.y, 5, 10};
  app->bullets[app->bullet_id].velocity = velocity;
  app->bullet_id = (app->bullet_id + 1) % BULLET_COUNT;
}

void handle_input(App_t *app) {
  SDL_Event event = {0};

  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
        exit(0);
        break;

      case SDL_KEYDOWN:
        switch(event.key.keysym.sym) {
          case SDLK_LEFT:
            app->player->rect.x = app->player->rect.x > 0 ? app->player->rect.x - 10 : 0;
            break;
          case SDLK_RIGHT:
            app->player->rect.x = app->player->rect.x < WINDOW_WIDTH - PLAYER_WIDTH ? app->player->rect.x + 10 : WINDOW_WIDTH - PLAYER_WIDTH;
            break;
          case SDLK_SPACE:
            if (app->player->shooting_timer <= 0) {
              app->player->shooting_timer = SHOOT_DELAY;
              SDL_Point loc = {app->player->rect.x + (app->player->rect.w - 5) / 2, app->player->rect.y - 10};
              spawn_bullet(app, loc, -5);
            }
            update_score(app, 100);
          default:
            break;
        }
        break;
    }
  }
}

void update(App_t *app) {
  Bullet_t *bullet = NULL;
  Barrier_t *barrier = NULL;
  Enemy_t *enemy = NULL;

  handle_input(app);

  if (app->player->shooting_timer) {
    app->player->shooting_timer -= FRAME_SLEEP;
  }

  for (size_t i = 0; i < BULLET_COUNT; i++) {
    bullet = &app->bullets[i];
    if (bullet->active) {
      bullet->rect.y += bullet->velocity;
      if ((bullet->rect.y > WINDOW_HEIGHT) || (bullet->rect.y < 0)) {
        bullet->active = false;
      }
    }
  }

  // Check bullet collisions between player, enemies or barriers
  for (size_t i = 0; i < BULLET_COUNT; i++) {
    bullet = &app->bullets[i];
    if (!bullet->active) continue;

    for (size_t barrier_index = 0; barrier_index < BARRIER_COUNT; barrier_index++) {
      barrier = &app->barriers[barrier_index];
      if ((barrier->level < Destroyed) && SDL_HasIntersection(&barrier->rect, &bullet->rect)) {
        barrier->level += 1;
        barrier->rect.h -= BARRIER_HEIGHT / Destroyed;
        barrier->rect.y += BARRIER_HEIGHT / Destroyed;
        bullet->active = false;
        continue;
      }
    }

    if (bullet->velocity < 0) {
      for (size_t enemy_index = 0; enemy_index < ENEMY_COUNT; enemy_index++) {
        enemy = &app->enemies[enemy_index];
        if (enemy->dead) continue;

        if (SDL_HasIntersection(&enemy->rect, &bullet->rect)) {
          DEBUG("Enemy hit!")
          enemy->dead = true;
          bullet->active = false;
          break;
        }
      }
    } else {
      // Player collision detection
    }

  }
}

void render(App_t *app) {
  // Enemies rendering
  SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
  for (size_t i = 0; i < app->enemy_count; i++) {
    if (!app->enemies[i].dead) {
      SDL_RenderFillRect(app->renderer, &app->enemies[i].rect);
    }
  }

  // Barrier rendering
  SDL_SetRenderDrawColor(app->renderer, 0, 255, 0, 255);
  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    if (app->barriers[i].level < Destroyed) {
      SDL_RenderFillRect(app->renderer, &app->barriers[i].rect);
    }
  }

  // Player rendering
  SDL_SetRenderDrawColor(app->renderer, 255, 0, 0, 255);
  SDL_RenderFillRect(app->renderer, &app->player->rect);

  SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
  for (size_t i = 0; i < BULLET_COUNT; i++) {
    if (app->bullets[i].active) {
      SDL_RenderFillRect(app->renderer, &app->bullets[i].rect);
    }
  }

  SDL_Rect score_rect = {0};
  SDL_QueryTexture(app->score_texture, NULL, NULL, &score_rect.w, &score_rect.h);
  SDL_RenderCopy(app->renderer, app->score_texture, NULL, &score_rect);
}

void init_player(App_t *app) {
  app->player = malloc(sizeof(Player_t));

  app->player->rect = (SDL_Rect){
    (WINDOW_WIDTH - PLAYER_WIDTH) / 2,
    WINDOW_HEIGHT - 10 - PLAYER_HEIGHT,
    PLAYER_WIDTH,
    PLAYER_HEIGHT
  };

  update_score(app, 0);
}

void init_barriers(App_t *app) {
  app->barriers = calloc(BARRIER_COUNT, sizeof(Barrier_t));
  if (app->barriers == NULL) {
    printf("Couldn't allocate barriers data: %s\n", SDL_GetError());
    exit(1);
  }

  int spacing = (WINDOW_WIDTH - (BARRIER_COUNT * BARRIER_WIDTH)) / (BARRIER_COUNT + 1);
  for(size_t i = 0; i < BARRIER_COUNT; i++) {
    app->barriers[i].rect = (SDL_Rect){
      (i + 1) * spacing + i * BARRIER_WIDTH,
      500,
      BARRIER_WIDTH,
      BARRIER_HEIGHT
    };
  }
}

void init_enemies(App_t *app, unsigned int count) {
  DEBUG("start");
  app->enemies = calloc(count, sizeof(Enemy_t));
  if (app->enemies == NULL) {
    DEBUG("malloc returned null");
    printf("Out of RAM?\n");
    exit(1);
  }

  // Add maybe some static initial offset to center enemies on screen
  int spacing = (WINDOW_WIDTH - 100 - (ENEMY_PER_LINE * ENEMY_WIDTH)) / (ENEMY_PER_LINE + 1);

  app->enemy_count = count;
  DEBUG("set app->enemy_count to %zu", app->enemy_count);
  for(size_t i = 0; i < count; i++) {
    app->enemies[i].rect = (SDL_Rect){
      ((i % ENEMY_PER_LINE) + 1) * spacing + (i % ENEMY_PER_LINE) * ENEMY_WIDTH,
      20 + ((i / ENEMY_PER_LINE) + 1) * spacing + (i / ENEMY_PER_LINE) * ENEMY_HEIGHT,
      ENEMY_WIDTH,
      ENEMY_HEIGHT
    };
  }
}

void init_app(App_t *app) {
  init_enemies(app, ENEMY_COUNT);
  init_barriers(app);
  init_player(app);

  app->bullets = calloc(50, sizeof(Bullet_t));
  if (app->bullets == NULL) {
    printf("Couldn't allocate memory for bullets...\n");
    exit(1);
  }
}

int main() {
  App_t app = {0};

  init_sdl(&app);
  init_app(&app);

  while(1) {
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);

    update(&app);
    render(&app);

    SDL_RenderPresent(app.renderer);
    SDL_Delay(FRAME_SLEEP);
  }

  return 0;
}
