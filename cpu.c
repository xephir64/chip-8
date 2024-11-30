#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu.h"

uint8 fonts[5 * 16] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80};

void reset(cpu_t *cpu) {
  cpu->i_register = 0;
  cpu->program_counter = START_ADDRESS;
  cpu->stack_pointer = 0;
  cpu->delay_timer = 0;
  cpu->sound_timer = 0;

  memset(cpu->registers, 0, sizeof(cpu->registers));
  memset(cpu->stack, 0, sizeof(cpu->stack));
  memset(cpu->memory, 0, sizeof(cpu->memory));
  memset(cpu->keypad, 0, sizeof(cpu->keypad));
  memcpy(cpu->memory, fonts, sizeof(fonts));
}

int load_rom(cpu_t *cpu, const char *path) {
  long rom_file_size = 0;
  unsigned long byte_read = 0;
  FILE *rom_file;

  rom_file = fopen(path, "rb");

  if (rom_file == NULL) {
    return -1;
  }

  fseek(rom_file, 0L, SEEK_END);
  rom_file_size = ftell(rom_file);
  rewind(rom_file);

  byte_read = fread(cpu->memory + START_ADDRESS, 1, rom_file_size, rom_file);

  if (byte_read == -1) {
    fclose(rom_file);
    return -1;
  }

  fclose(rom_file);
  return 0;
}

void push_stack(cpu_t *cpu, uint16 value) {
  cpu->stack[cpu->stack_pointer] = value;
  cpu->stack_pointer += 1;
}

uint16 pop_stack(cpu_t *cpu) {
  cpu->stack_pointer -= 1;
  return cpu->stack[cpu->stack_pointer];
}

nibble_t _decode(uint16 opcode) {
  nibble_t data;
  data.opcode = opcode;
  data.t = opcode >> 12;
  data.x = opcode >> 8 & 0xF;
  data.y = opcode >> 4 & 0xF;
  data.n = opcode & 0xF;
  data.nn = opcode & 0xFF;
  data.nnn = opcode & 0xFFF;
  return data;
}

nibble_t _fetch(cpu_t *cpu) {
  uint16 opcode = 0;
  uint16 high = cpu->memory[cpu->program_counter];
  uint16 low = cpu->memory[cpu->program_counter + 2];
  cpu->program_counter += 2;
  opcode = (high << 8) | low;
  return _decode(opcode);
}

void tick(cpu_t *cpu) {
  nibble_t op = _fetch(cpu);
  cpu->opcode = op;
  execute(cpu, op);
}

void tick_timers(cpu_t *cpu) {
  if (cpu->delay_timer > 0) {
    cpu->delay_timer -= 1;
  }
  if (cpu->sound_timer > 0) {
    if (cpu->sound_timer == 1) {
      /* Beep */
    }
    cpu->sound_timer -= 1;
  }
}

void execute(cpu_t *cpu, nibble_t op) {
  switch (op.t) {
  case 0x0:
    if (op.nn == 0xE0) {
      /* CLS */
      memset(cpu->video, 0, sizeof(cpu->video));
    } else if (op.nn == 0xEE) {
      /* RET */
      cpu->program_counter = pop_stack(cpu);
    }
    break;

  case 0x1:
    /* JP addr */
    cpu->program_counter = op.nnn;
    break;

  case 0x2:
    /* CALL addr*/
    push_stack(cpu, cpu->program_counter);
    cpu->program_counter = op.nnn;
    break;

  case 0x3:
    /* SE Vx, byte*/
    if (cpu->registers[op.x] == op.nn)
      cpu->program_counter += 2;
    break;

  case 0x4:
    /* SNE Vx, byte*/
    if (cpu->registers[op.x] != op.nn)
      cpu->program_counter += 2;
    break;

  case 0x5:
    /* SE Vx, Vy*/
    if (cpu->registers[op.x] == cpu->registers[op.y])
      cpu->program_counter += 2;
    break;

  case 0x6:
    /* LD Vx, byte*/
    cpu->registers[op.x] = op.nn;
    break;

  case 0x7:
    /* ADD Vx, byte */
    _add(cpu, op);
    break;

  case 0x8:
    switch (op.n) {
    case 0x1:
      /* OR Vx, Vy */
      cpu->registers[op.x] = cpu->registers[op.x] | cpu->registers[op.y];
      break;

    case 0x2:
      /* AND Vx, Vy */
      cpu->registers[op.x] = cpu->registers[op.x] & cpu->registers[op.y];
      break;

    case 0x3:
      /* OR Vx, Vy */
      cpu->registers[op.x] = cpu->registers[op.x] ^ cpu->registers[op.y];
      break;

    case 0x4:
      /* ADD Vx, Vy (Perform ADD with Carry)*/
      _add_with_carry(cpu, op);
      break;

    case 0x5:
      /* SUB Vx, Vy (Perform SUB with Carry)*/
      _sub_with_carry(cpu, op);
      break;

    case 0x6:
      /* SHR Vx {, Vy} */
      _shr(cpu, op);
      break;

    case 0x7:
      /* SUBN Vx, Vy (Perform SUB with Carry)*/
      _subn_with_carry(cpu, op);
      break;

    case 0xE:
      /* SHL Vx {, Vy} */
      _shl(cpu, op);
      break;
    }

  case 0x9:
    /* SNE Vx, Vy*/
    if (cpu->registers[op.x] != cpu->registers[op.y])
      cpu->program_counter += 2;
    break;

  case 0xA:
    /* LD I, addr */
    cpu->i_register = op.nnn;
    break;

  case 0xB:
    /* JP V0, addr */
    cpu->program_counter = cpu->registers[0] + op.nnn;
    break;
  case 0xC:
    /* RND Vx, byte */
    _rnd(cpu, op);
  default:
    printf("Unsupported opcode 0x%X\n", op.opcode);
  }
}

void _add(cpu_t *cpu, nibble_t op) {
  uint16 res = cpu->registers[op.x] + op.nn;
  if (res > 255)
    res = 0;

  cpu->registers[op.x] = res;
}

void _add_with_carry(cpu_t *cpu, nibble_t op) {
  uint16 res = cpu->registers[op.x] + cpu->registers[op.y];
  if (res > 255)
    cpu->registers[0xF] = 1;
  else
    cpu->registers[0xF] = 0;
  cpu->registers[op.x] = res & 0xFF;
}

void _sub_with_carry(cpu_t *cpu, nibble_t op) {
  uint16 res = cpu->registers[op.x] - cpu->registers[op.y];
  if (cpu->registers[op.x] > cpu->registers[op.y])
    cpu->registers[0xF] = 1;
  else
    cpu->registers[0xF] = 0;
  cpu->registers[op.x] = res;
}

void _shr(cpu_t *cpu, nibble_t op) {
  if (cpu->registers[op.x] & 1)
    cpu->registers[0xF] = 1;
  else
    cpu->registers[0xF] = 0;
  cpu->registers[op.x] /= 2;
}

void _subn_with_carry(cpu_t *cpu, nibble_t op) {
  uint16 res = cpu->registers[op.y] - cpu->registers[op.x];
  if (cpu->registers[op.y] > cpu->registers[op.x])
    cpu->registers[0xF] = 1;
  else
    cpu->registers[0xF] = 0;
  cpu->registers[op.x] = res;
}

void _shl(cpu_t *cpu, nibble_t op) {
  if ((cpu->registers[op.x] >> 7) & 1)
    cpu->registers[0xF] = 1;
  else
    cpu->registers[0xF] = 0;
  cpu->registers[op.x] *= 2;
}

void _rnd(cpu_t *cpu, nibble_t op) {
  uint8 rd_num = rand() % 256; /* Not the best way to obtain a random */
  cpu->registers[op.x] = rd_num & op.nn;
}