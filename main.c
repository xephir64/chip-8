#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

int main(int argc, char **const argv) {
  cpu_t *cpu = NULL;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s rom_file_path\n", argv[0]);
    return 1;
  }

  cpu = malloc(sizeof(cpu_t));
  reset(cpu);

  if (load_rom(cpu, argv[1]) == -1) {
    fprintf(stderr, "Cannot load ROM\n");
    free(cpu);
    return 1;
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Quit();

  free(cpu);
  return 0;
}