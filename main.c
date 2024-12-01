#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_image.h>

#define ARR_LEN(a) (sizeof(a)/sizeof(a[0]))
#define DEBUG(format, ...) printf("%s:%d:%s() " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define ENEMY_WIDTH 40
#define ENEMY_HEIGHT 20
#define ENEMY_SPACING 20
#define ENEMY_PER_LINE 12

#define BARRIER_COUNT 3
#define BARRIER_WIDTH 100
#define BARRIER_HEIGHT 40

typedef struct {
  SDL_Rect rect;
} Enemy_t;

typedef struct {
  SDL_Rect rect;
} Player_t;

typedef struct {
  SDL_Rect rect;
} Barrier_t;

typedef struct {
  SDL_Renderer *renderer;
  SDL_Window   *window;
  Enemy_t      *enemies;
  size_t       enemy_count;
  Player_t     *player;
  Barrier_t    *barriers;
} App_t;

void init_sdl(App_t *app) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
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
}

void handle_input(void) {
  SDL_Event event = {0};

  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
      exit(0);
      break;
    }
  }
}

void render(App_t *app) {
  // Enemies rendering
  SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
  for (size_t i = 0; i < app->enemy_count; i++) {
    SDL_RenderFillRect(app->renderer, &app->enemies[i].rect);
  }

  // Barrier rendering
  SDL_SetRenderDrawColor(app->renderer, 0, 255, 0, 255);
  for (size_t i = 0; i < BARRIER_COUNT; i++) {
    SDL_RenderFillRect(app->renderer, &app->barriers[i].rect);
  }

  // Player rendering
}

void init_player(App_t *app) {

}

void init_barriers(App_t *app) {
  app->barriers = calloc(BARRIER_COUNT, sizeof(Barrier_t));
  if (app->barriers == NULL) {
    printf("Couldn't allocate barriers data: %s\n", SDL_GetError());
    exit(1);
  }

  for(size_t i = 0; i < BARRIER_COUNT; i++) {
    app->barriers[i].rect = (SDL_Rect){
      100 + (i * 250),
      475,
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
  app->enemy_count = count;
  DEBUG("set app->enemy_count to %zu", app->enemy_count);
  for(size_t i = 0; i < count; i++) {
    app->enemies[i].rect = (SDL_Rect){
      ((i % ENEMY_PER_LINE) + 1) * ENEMY_SPACING + (i % ENEMY_PER_LINE) * ENEMY_WIDTH,
      ((i / ENEMY_PER_LINE) + 1) * ENEMY_SPACING + (i / ENEMY_PER_LINE) * ENEMY_HEIGHT,
      ENEMY_WIDTH,
      ENEMY_HEIGHT
    };
  }
}

int main() {
  App_t app = {0};

  init_sdl(&app);
  init_enemies(&app, ENEMY_PER_LINE * 5);
  init_barriers(&app);
  init_player(&app);

  while(1) {
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);

    handle_input();
    render(&app);

    SDL_RenderPresent(app.renderer);
    SDL_Delay(16);
  }

  return 0;
}
