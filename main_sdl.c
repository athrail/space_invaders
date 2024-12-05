#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

// macros
#define ARR_LEN(a) (sizeof(a)/sizeof(a[0]))
#define DEBUG(format, ...) printf("%s:%d:%s() " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);

#define FRAME_SLEEP 16
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define RENDER_SCALE 2.0
#define BULLET_COUNT 10
#define SHOOT_DELAY 300

#define ENEMY_PER_LINE 10
#define ENEMY_COUNT ENEMY_PER_LINE * 5
#define BARRIER_COUNT 4
#define BARRIER_WIDTH 70
#define BARRIER_HEIGHT 50

#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 8
#define PLAYER_SPRITE_X 1
#define PLAYER_SPRITE_Y 49
#define ENEMY_SPRITE_X 1
#define ENEMY_SPRITE_Y 1

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
  bool          running;
  SDL_Renderer  *renderer;
  SDL_Window    *window;
  Enemy_t       *enemies;
  SDL_Texture   *interim;
  size_t        enemy_count;
  int           moving_row;
  Player_t      player;
  int           player_lives;
  Barrier_t     *barriers;
  TTF_Font      *sans;
  unsigned int  score;
  SDL_Texture   *score_texture;
  Bullet_t      *bullets;
  unsigned int  bullet_id;
  SDL_Texture   *sprites;
} App_t;

void init_sdl(App_t *app) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() < 0) {
    printf("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
    exit(1);
  }

  if (IMG_Init(IMG_INIT_PNG) < 0) {
    printf("Couldn't initialize SDL_image: %s\n", SDL_GetError());
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

  // SDL_RenderSetIntegerScale(app->renderer, true);
  // SDL_RenderSetScale(app->renderer, RENDER_SCALE, RENDER_SCALE);

  app->sans = TTF_OpenFont("arcade.ttf", 24);
  if (app->sans == NULL) {
    printf("Couldn't load arcade.ttf font: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Surface *surface = IMG_Load("sprite.png");
  if (surface == NULL) {
    printf("Couldn't load sprite.png : %s\n", SDL_GetError());
    exit(1);
  }
  app->sprites = SDL_CreateTextureFromSurface(app->renderer, surface);
  if (app->sprites == NULL) {
    printf("Couldn't create texture from sprite surface : %s\n", SDL_GetError());
    exit(1);
  }
  SDL_FreeSurface(surface);
}

void update_score(App_t *app, int change) {
  app->score += change;
  char *buf = malloc(sizeof(char) * 255);
  memset(buf, 0, sizeof(char) * 255);
  sprintf(buf, "Score   %08d                  Lives   %02d", app->score, app->player_lives);
  SDL_Surface *score_surface = TTF_RenderText_Solid(app->sans, buf, (SDL_Color){255, 255, 255, 255});
  app->score_texture = SDL_CreateTextureFromSurface(app->renderer, score_surface);
  SDL_FreeSurface(score_surface);
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
        app->running = false;
        break;

      case SDL_KEYDOWN:
        switch(event.key.keysym.sym) {
          case SDLK_LEFT:
            app->player.rect.x = app->player.rect.x > 0 ? app->player.rect.x - 10 : 0;
            break;
          case SDLK_RIGHT:
            app->player.rect.x = app->player.rect.x < WINDOW_WIDTH - SPRITE_WIDTH ? app->player.rect.x + 10 : WINDOW_WIDTH - SPRITE_WIDTH;
            break;
          case SDLK_SPACE:
            if (app->player.shooting_timer <= 0) {
              app->player.shooting_timer = SHOOT_DELAY;
              SDL_Point loc = {app->player.rect.x + (app->player.rect.w - 5) / 2, app->player.rect.y - 10};
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

  if (app->player.shooting_timer) {
    app->player.shooting_timer -= FRAME_SLEEP;
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
  SDL_SetTextureColorMod(app->sprites, 255, 255, 255);
  for (size_t i = 0; i < app->enemy_count; i++) {
    if (!app->enemies[i].dead) {
      SDL_Rect src = {ENEMY_SPRITE_X, ENEMY_SPRITE_Y, SPRITE_WIDTH, SPRITE_HEIGHT};
      SDL_RenderCopy(app->renderer, app->sprites, &src, &app->enemies[i].rect);
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
  SDL_SetTextureColorMod(app->sprites, 255, 0, 0);
  SDL_Rect src = {PLAYER_SPRITE_X, PLAYER_SPRITE_Y, SPRITE_WIDTH, SPRITE_HEIGHT};
  SDL_RenderCopy(app->renderer, app->sprites, &src, &app->player.rect);

  SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
  for (size_t i = 0; i < BULLET_COUNT; i++) {
    if (app->bullets[i].active) {
      SDL_RenderFillRect(app->renderer, &app->bullets[i].rect);
    }
  }

  SDL_Rect score_rect = {10, 10, 0, 0};
  SDL_QueryTexture(app->score_texture, NULL, NULL, &score_rect.w, &score_rect.h);
  SDL_RenderCopy(app->renderer, app->score_texture, NULL, &score_rect);
}

void init_player(App_t *app) {
  app->player.rect = (SDL_Rect){
    (WINDOW_WIDTH - SPRITE_WIDTH) / 2,
    WINDOW_HEIGHT - 10 - SPRITE_HEIGHT,
    SPRITE_WIDTH,
    SPRITE_HEIGHT
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
      400,
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
  int spacing = (WINDOW_WIDTH - 100 - (ENEMY_PER_LINE * SPRITE_WIDTH)) / (ENEMY_PER_LINE + 1);

  app->enemy_count = count;
  DEBUG("set app->enemy_count to %zu", app->enemy_count);
  for(size_t i = 0; i < count; i++) {
    app->enemies[i].rect = (SDL_Rect){
      ((i % ENEMY_PER_LINE) + 1) * spacing + (i % ENEMY_PER_LINE) * SPRITE_WIDTH,
      20 + ((i / ENEMY_PER_LINE) + 1) * spacing + (i / ENEMY_PER_LINE) * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
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

  app->running = true;
}

void cleanup(App_t *app) {
  free(app->renderer);
  free(app->window);
  free(app->enemies);
  free(app->barriers);
  free(app->interim);
  free(app->sans);
  free(app->score_texture);
  free(app->bullets);
  free(app->sprites);
}

int main() {
  App_t app = {0};
  // atexit(SDL_Quit);

  init_sdl(&app);
  init_app(&app);

  while(app.running) {
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);

    update(&app);
    render(&app);

    SDL_RenderPresent(app.renderer);
    SDL_Delay(FRAME_SLEEP);
  }

  cleanup(&app);
  IMG_Quit();

  return 0;
}
