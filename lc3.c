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

uint16_t sign_extend(uint16_t x, int bit_count)
{
  if ((x >> (bit_count - 1)) & 1)
  {
    x |= (0xFFFF << bit_count);
  }
  return x;
}

void update_flags(uint16_t r)
{
  if (reg[r] == 0)
  {
    reg[R_COND] = FL_ZRO;
  }
  else if (reg[r] >> 15) // A 1 in the left-most bit indicates negative
  {
    reg[R_COND] = FL_NEG;
  }
  else
  {
    reg[R_COND] = FL_POS;
  }
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
      {
        // Destination register (DR)
        uint16_t r0 = (instr >> 9) & 0x7;
        // First operand (SR1)
        uint16_t r1 = (instr >> 6) & 0x7;
        // Whether we are in immediate mode
        uint16_t imm_flag = (instr >> 5) & 0x1;

        if (imm_flag)
        {
          uint16_t imm5 = sign_extend(instr & 0x1F, 5);
          reg[r0] = reg[r1] + imm5;
        }
        else
        {
          uint16_t r2 = instr & 0x7;
          reg[r0] = reg[r1] + reg[r2];
        }
        
        update_flags(r0);
      }
      break;
      
      case OP_AND:
      {
        uint16_t r0 (instr >> 9) & 0x7;
        uint16_t r1 = (instr >> 9) & 0x7;
        uint16_t imm_flag = (instr >> 5) & 0x1;
        
        if(imm_flag)
        {
          uint16_t r2 = sign_extend(instr & 0x1F, 5);
          reg[r0] = reg[r1] & imm5;
        }
        else
        {
          uint16_t r2 = instr & 0x7;
          reg[r0] = reg[r1] & reg[r2];
        }

        update_flags(r0);
      }
      break;

      case OP_NOT:
      {
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t r1 = (instr >> 6) & 0x7;

        reg[r0] = ~(reg[r1]);
        update_flags(r0);
      }
      break;

      case OP_BR:
      {
        uint16_t n = (instr >> 9) & 0x4;
        uint16_t z = (instr >> 9) & 0x2;
        uint16_t p = (instr >> 9) & 0x1;
        uint16_t pc_offset = instr & 0x1FF;

        if ((n && FL_NEG) || (z && FL_ZRO) || (p && FL_POS))
        {
          reg[R_PC] += sign_extend(pc_offset, 9);  
        }
        //update_flags(PC); // Why we don't need this?
      }
      break;

      case OP_JMP:
      {
        // Also handles RET
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r1];
      }
      break;

      case OP_JSR:
      {
        uint16_t flag = (instr >> 11) & 0x1;
        reg[R_R7] = reg[R_PC];
        if (!flag) // JSRR
        {
          uint16_t r1 = (instr >> 6) & 0x7;
          reg[R_PC] = reg[r1];
        }
        else // JSR
        {
          uint16_t pc_offset = instr & 0x7FF;
          reg[PC] += sign_extend(pc_offset, 11);
        }
      }
      break;

      case OP_LD:
      {
        uint16_t r0 = (instr >> 9) & 0x7; // Destination
        uint16_t pc_offset = (instr) & 0x1FF;
        
        reg[r0] = mem_read(reg[R_PC] + sign_extend(pc_offset, 9));
        update_flags(r0);
      }
      break;

      case OP_LDI:
      {
        // Destination register (DR)
        uint16_t r0 = (instr >> 9) & 0x7;
        // PCoffset 9
        uint16_t pc_offset = sign_extend(instr & 0X1FF, 9);
        // Add pc_offset to the current PC, look at that memory location to get the final address
        reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
        update_flags(r0);
      }
      break;

      case OP_LDR:
      {
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t r1 = (instr >> 6) & 0x7; //BaseR
        uint16_t offset = sign_extend((instr & 0x3F, 6);
        
        reg[r0] = mem_read(reg[r1] + offset);
        update_flags(r0);
      }
      break;

      case OP_LEA:
      {
        uint16_t r0 = (instr >> 9) & 0x7;
        uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

        reg[r0] = reg[R_PC] + pc_offset;
        update_flags(r0);
      }
      break;
    
      case OP_ST:
      {
        uint16_t r1 = (instr >> 9) & 0x7;
        uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

        reg[r1] = mem_read(reg[R_PC] + pc_offset); // Is it compulsory to write mem_write ??
      }
      break;

      case OP_STI:
      {
        uint16_t r1 = (instr >> 9) & 0x7;
        uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

        reg[r1] = mem_read(mem_read(reg[R_PC] + pc_offset));
      }
      break;

      case OP_STR:
      {
        uint16_t r0 = (instr >> 6) & 0x7;
        uint16_t r1 = (instr >> 9) & 0x7;
        uint16_t offset = sign_extend(instr & 0x3F, 6);

        reg[r1] = mem_read(r0 + offset);
      }
      break;

      case OP_TRAP:
      {
        uint16_t trapvect = ;
        
        reg[R_R7] = reg[R_PC];
        reg[R_PC] = mem_read();
      }
      break;

      case OP_RES:
      case OP_RTI:
      default:
        abort();// Bad Opcode
        break;
    }
  } 
  // Shutdown
}
