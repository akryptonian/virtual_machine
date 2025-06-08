#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#define MEMORY_MAX (1 << 16)

uint16_t memory[MEMORY_MAX]; // 65_536 locations
uint16_t reg[R_COUNT];

// Registers

enum
{
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC, // Program counter
  R_COND,
  R_COUNT
};

// Condition flags

enum
{
  FL_POS = 1 << 0, // P
  FL_ZRO = 1 << 1, // Z
  FL_NEG = 1 << 2, // N
};

// Opcodes

enum
{
  OP_BR = 0, // branch
  OP_ADD, // add
  OP_LD, // load
  OP_ST, // store
  OP_JSR, // jump register
  OP_AND, // bitwise and
  OP_LDR, // load register
  OP_STR, // store register
  OP_RTI, // unused
  OP_NOT, // bitwise not
  OP_LDI, // load indirect
  OP_STI, // store indirect
  OP_JMP, // jump
  OP_RES, // reserved (unused)
  OP_LEA, // load effective address
  OP_TRAP // execute trap
}

int main(int argc, const char* argv[])
{

  // Load arguments

  if (argc < 2)
  {
    // Show usage string
    printf("lc3 [image-file1] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; ++j)
  {
    if (!read_image(argv[j]))
    {
      printf("failed to load image: %s\n", argv[j]);
    }
  }

  // Setup

  // Since exactly one condition flag should be set at any given time, set the Z flag

  reg[R_COND] = FL_ZRO;

  // Set the PC to starting position
  // 0x3000 is the default

  enum { PC_START = 0x3000};
  reg[R_PC] = PC_START;

  int running = 1;
  while(running)
  {
    // Fetch
    uint16_t instr = mem_read(reg[R_PC]++);
    uint16_t op = instr >> 12;

    switch(op)
    {
      case OP_ADD:
      break;
      
      case OP_AND:
      break;

      case OP_NOT:
      break;

      case OP_BR:
      break;

      case OP_JMP:
      break;

      case OP_JSR:
      break;

      case OP_LD:
      break;

      case OP_LDI:
      break;

      case OP_LDR:
      break;

      case OP_LEA:
      break;
    
      case OP_ST:
      break;

      case OP_STI:
      break;

      case OP_TRAP:
      break;

      case OP_RES:
      case OP_RTI:
      default:
        // Bad Opcode
        break;
    }
  } 
  // Shutdown
}
