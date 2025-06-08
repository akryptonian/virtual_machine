#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; // 65_536 locations

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

unint16_t reg[R_COUNT];

