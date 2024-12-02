#include <SDL/SDL.h>
#include <SDL/SDL_error.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

#define SCALE 8

void draw_screen(SDL_Surface *screen, cpu_t *cpu) {
  int x, y;
  SDL_Rect pixel;
  uint32 color;

  pixel.w = SCALE;
  pixel.h = SCALE;

  for (y = 0; y < SCREEN_HEIGHT; y++) {
    for (x = 0; x < SCREEN_WIDTH; x++) {
      uint8 pixel_value = cpu->video[x + (y * SCREEN_WIDTH)];
      pixel.x = x * SCALE;
      pixel.y = y * SCALE;

      color = pixel_value ? SDL_MapRGB(screen->format, 255, 255, 255)
                          : SDL_MapRGB(screen->format, 0, 0, 0);

      SDL_FillRect(screen, &pixel, color);
    }
  }

  SDL_Flip(screen);
}

int main(int argc, char **const argv) {
  SDL_Event event;
  SDL_Surface *screen = NULL;
  cpu_t *cpu = NULL;
  int running = 0;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s rom_file_path\n", argv[0]);
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) == -1) {
    fprintf(stderr, "Cannot init SDL: %s", SDL_GetError());
    return 1;
  }

  screen = SDL_SetVideoMode(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, 32,
                            SDL_SWSURFACE);
  if (!screen) {
    fprintf(stderr, "Cannot set video mode: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  cpu = malloc(sizeof(cpu_t));
  if (!cpu) {
    fprintf(stderr, "Cannot allocate memory for the Chip-8 CPU");
    return 1;
  }

  reset(cpu);

  if (load_rom(cpu, argv[1]) == -1) {
    fprintf(stderr, "Cannot load ROM\n");
    free(cpu);
    return 1;
  }

  running = 1;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
      }
    }

    tick(cpu);
    tick_timers(cpu);
    draw_screen(screen, cpu);

    SDL_Delay(16);
  }

  SDL_Quit();

  free(cpu);
  return 0;
}