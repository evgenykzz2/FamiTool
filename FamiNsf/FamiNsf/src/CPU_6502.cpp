#include "CPU_6502.h"
#include "Memory.h"
#include "Registers.h"
#include "APU.h"

#ifndef WIN32
#include <Arduino.h>
#else
#include <Windows.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#endif

#define BIT_C 0x01
#define BIT_Z 0x02
#define BIT_I 0x04
#define BIT_D 0x08
#define BIT_B 0x10
#define BIT_U 0x20
#define BIT_V 0x40
#define BIT_N 0x80

int32_t s_nes_cycles = 0;
int32_t s_nes_frame_cycles = 0;

static uint8_t s_iqr_mask = 0;

static const uint8_t IQR_NMI = 0x01;
static const uint8_t IQR = 0x02;

#define SET_FLAGS(FLAGS)        \
{                               \
  s_nes_regs.C = FLAGS & BIT_C;     \
  s_nes_regs.Z = ((FLAGS & BIT_Z)==0);     \
  s_nes_regs.I = FLAGS & BIT_I;     \
  s_nes_regs.D = FLAGS & BIT_D;     \
  s_nes_regs.B = FLAGS & BIT_B;     \
  s_nes_regs.U = 1;                 \
  s_nes_regs.V = FLAGS & BIT_V;     \
  s_nes_regs.N = FLAGS & BIT_N;     \
}

#define GET_FLAGS  \
  (        s_nes_regs.C |                \
          (s_nes_regs.Z ? 0 : BIT_Z) |   \
          (s_nes_regs.I ? BIT_I : 0) |   \
          (s_nes_regs.D ? BIT_D : 0) |   \
          (s_nes_regs.B ? BIT_B : 0) |   \
          (BIT_U)                |   \
          (s_nes_regs.V ? BIT_V : 0) |   \
          (s_nes_regs.N & BIT_N) )       \

#define FLAG_NP_SET(VAL)            \
{                                   \
  s_nes_regs.Z = VAL;                   \
  s_nes_regs.N = VAL;                   \
}

#define ADD_CYCLES(CYCLES)	s_nes_cycles += CYCLES

#define PAGE_CHECK(ADDR, REG)   \
if (REG>(uint8_t)(ADDR))        \
  ADD_CYCLES(1);

static uint8_t s_tmp_value = 0;
static uint16_t s_tmp_addr = 0;
static uint8_t s_tmp_8= 0;
static uint16_t s_tmp_16 = 0;

#define MEMORY_READ(ADDR) s_nes_memory_read_func[ADDR >> 13](ADDR)
#define MEMORY_READZERO_PAGE(ADDR) s_nes_ram[ADDR]

#define MEMORY_WRITE(ADDR, VAL) s_nes_memory_write_func[ADDR >> 13](ADDR, VAL)
#define MEMORY_WRITE_ZP(ADDR, VAL) s_nes_ram[ADDR]=VAL

#define IMMEDIATE_VALUE 					\
{											\
   s_tmp_value = MEMORY_READ(s_nes_regs.PC);	\
   s_nes_regs.PC ++;							\
}

#define ABSOLUTE_ADDR 	    				\
s_tmp_addr = MEMORY_READ(s_nes_regs.PC);	    \
s_nes_regs.PC ++;							    \
s_tmp_addr |= (uint16_t)(MEMORY_READ(s_nes_regs.PC))<<8;    \
s_nes_regs.PC ++;

#define ABSOLUTE_VALUE 	    				\
ABSOLUTE_ADDR;							    \
s_tmp_value = MEMORY_READ(s_tmp_addr);

#define ZERO_PAGE_ADDR 	    				\
s_tmp_addr = MEMORY_READ(s_nes_regs.PC);	    \
s_nes_regs.PC ++;

#define ZERO_PAGE_VALUE     				\
{											\
   ZERO_PAGE_ADDR;							\
   s_tmp_value = MEMORY_READZERO_PAGE(s_tmp_addr);	\
}

#define IND_X_ADDR 	    			    	\
{											\
   s_tmp_8 = MEMORY_READ(s_nes_regs.PC) + s_nes_regs.X;\
   s_nes_regs.PC ++;                            \
   s_tmp_addr = MEMORY_READZERO_PAGE(s_tmp_8);       \
   s_tmp_8++;                               \
   s_tmp_addr |= MEMORY_READZERO_PAGE(s_tmp_8) << 8; \
}

#define IND_X_VALUE                         \
{                                           \
   IND_X_ADDR;                              \
   s_tmp_value = MEMORY_READ(s_tmp_addr);   \
}

#define IND_Y_ADDR 	    			    	\
{											\
   s_tmp_8 = MEMORY_READ(s_nes_regs.PC);        \
   s_nes_regs.PC ++;                            \
   s_tmp_addr = MEMORY_READZERO_PAGE(s_tmp_8);       \
   s_tmp_8++;                               \
   s_tmp_addr |= MEMORY_READZERO_PAGE(s_tmp_8) << 8; \
   s_tmp_addr += s_nes_regs.Y;                  \
}

#define IND_Y_VALUE                         \
{                                           \
   IND_Y_ADDR;                              \
   s_tmp_value = MEMORY_READ(s_tmp_addr);   \
}

#define ZP_X_ADDR_NEW 	    			    	\
{											\
   s_tmp_8 = MEMORY_READ(s_nes_regs.PC) + s_nes_regs.X;\
   s_nes_regs.PC ++;                            \
   s_tmp_addr = s_tmp_8;                    \
}

#define ZP_X_VALUE                         \
{                                           \
   ZP_X_ADDR_NEW;                              \
   s_tmp_value = MEMORY_READZERO_PAGE(s_tmp_addr);   \
}

#define ABS_X_ADDR 	    			    	\
{											\
   ABSOLUTE_ADDR; 	    				    \
   s_tmp_addr += s_nes_regs.X;                  \
}

#define ABS_X_VALUE                         \
{                                           \
  ABS_X_ADDR;                               \
  PAGE_CHECK(s_tmp_addr,s_nes_regs.X)           \
  s_tmp_value = MEMORY_READ(s_tmp_addr);    \
}

#define ZP_Y_ADDR_NEW    			    	\
{											\
   s_tmp_8 = MEMORY_READ(s_nes_regs.PC) + s_nes_regs.Y;\
   s_nes_regs.PC ++;                            \
   s_tmp_addr = s_tmp_8;                    \
}

#define ZP_Y_VALUE                         \
{                                           \
   ZP_Y_ADDR_NEW;                              \
   s_tmp_value = MEMORY_READZERO_PAGE(s_tmp_addr);   \
}

#define ABS_Y_ADDR 	    			    	\
{											\
   ABSOLUTE_ADDR; 	    				    \
   s_tmp_addr += s_nes_regs.Y;                  \
}

#define ABS_Y_VALUE                         \
ABS_Y_ADDR;                                 \
PAGE_CHECK(s_tmp_addr,s_nes_regs.Y)             \
s_tmp_value = MEMORY_READ(s_tmp_addr);

#define PUSH8(VAL)                  \
s_nes_ram[0x0100 | s_nes_regs.S] = VAL;     \
s_nes_regs.S --;

#define PUSH16(VAL)                     \
s_nes_ram[0x0100 | s_nes_regs.S] = VAL >> 8;    \
s_nes_regs.S --;                            \
s_nes_ram[0x0100 | s_nes_regs.S] = VAL & 0xFF;  \
s_nes_regs.S --;

#define POP_8 s_nes_ram[0x0100 | (++s_nes_regs.S)]

#define REL_BRANCH_NEW(condition)                 \
{                                                 \
  if (condition)                                  \
  {                                               \
    IMMEDIATE_VALUE 					          \
    if (((int8_t)s_tmp_value + (s_nes_regs.PC & 0x00FF))&0x100) \
      ADD_CYCLES(1);                              \
    ADD_CYCLES(3);                                \
    s_nes_regs.PC += (int8_t)s_tmp_value;             \
  } else                                          \
  {                                               \
    s_nes_regs.PC++;                                  \
    ADD_CYCLES(2);                                \
  }                                               \
}

#define BRK_NEW         \
s_nes_regs.PC++;            \
PUSH16(s_nes_regs.PC)       \
s_nes_regs.B = 1;           \
PUSH8(GET_FLAGS);       \
s_nes_regs.I = 1;           \
ABSOLUTE_ADDR;          \
s_nes_regs.PC = s_tmp_addr; \
ADD_CYCLES(7);

#define JSR_NEW     \
ABSOLUTE_ADDR       \
s_nes_regs.PC--;        \
PUSH16(s_nes_regs.PC)   \
s_nes_regs.PC = s_tmp_addr; \
ADD_CYCLES(6);

#define NOT_IMPLEMENTED     \
s_nes_cycles = 0;

#define ASL_NEW(CYCLES, READ_FUNC, WRITE_FUNC)      \
READ_FUNC;                              \
s_nes_regs.C = s_tmp_value >> 7;            \
s_tmp_value <<= 1;                      \
FLAG_NP_SET(s_tmp_value);               \
WRITE_FUNC(s_tmp_addr, s_tmp_value);  \
ADD_CYCLES(CYCLES);

#define ASL_A                       \
s_nes_regs.C = s_nes_regs.A >> 7;           \
s_nes_regs.A <<= 1;                     \
FLAG_NP_SET(s_nes_regs.A);              \
ADD_CYCLES(2);

#define LDA_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;						\
s_nes_regs.A = s_tmp_value;			\
FLAG_NP_SET(s_nes_regs.A);			\
ADD_CYCLES(CYCLES);

#define LDX_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;						\
s_nes_regs.X = s_tmp_value;			\
FLAG_NP_SET(s_nes_regs.X);			\
ADD_CYCLES(CYCLES);

#define LDY_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;						\
s_nes_regs.Y = s_tmp_value;			\
FLAG_NP_SET(s_nes_regs.Y);			\
ADD_CYCLES(CYCLES);

#define ORA_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;                          \
s_nes_regs.A |= s_tmp_value;            \
FLAG_NP_SET(s_nes_regs.A);              \
ADD_CYCLES(CYCLES);

#define AND_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;                          \
s_nes_regs.A &= s_tmp_value;            \
FLAG_NP_SET(s_nes_regs.A);              \
ADD_CYCLES(CYCLES);

#define EOR_NEW(CYCLES, READ_FUNC)	\
READ_FUNC;                          \
s_nes_regs.A ^= s_tmp_value;            \
FLAG_NP_SET(s_nes_regs.A);              \
ADD_CYCLES(CYCLES);

#define BIT_NEW(CYCLES, READ_FUNC)  \
READ_FUNC;                          \
s_nes_regs.N = s_tmp_value;             \
s_nes_regs.V = s_tmp_value & BIT_V;     \
s_nes_regs.Z = s_tmp_value & s_nes_regs.A;  \
ADD_CYCLES(CYCLES);

#define ROL_A                                   \
s_tmp_8 = s_nes_regs.C;                             \
s_nes_regs.C = s_nes_regs.A >> 7;                       \
s_nes_regs.A = (s_nes_regs.A << 1) | s_tmp_8;           \
FLAG_NP_SET(s_nes_regs.A);                          \
ADD_CYCLES(2);

#define ROL_NEW(CYCLES, READ_FUNC, WRITE_FUNC)  \
READ_FUNC;                                      \
s_tmp_8 = s_nes_regs.C;                             \
s_nes_regs.C = s_tmp_value >> 7;                    \
s_tmp_value = (s_tmp_value << 1) | s_tmp_8;     \
FLAG_NP_SET(s_tmp_value);                       \
WRITE_FUNC(s_tmp_addr, s_tmp_value);            \
ADD_CYCLES(CYCLES);

#define ROR_A                                   \
s_tmp_8 = s_nes_regs.C << 7;                        \
s_nes_regs.C = s_nes_regs.A & 1;                        \
s_nes_regs.A = (s_nes_regs.A >> 1) | s_tmp_8;           \
FLAG_NP_SET(s_nes_regs.A);                          \
ADD_CYCLES(2);

#define ROR_NEW(CYCLES, READ_FUNC, WRITE_FUNC)  \
READ_FUNC;                                      \
s_tmp_8 = s_nes_regs.C << 7;                        \
s_nes_regs.C = s_tmp_value & 1;                     \
s_tmp_value = (s_tmp_value >> 1) | s_tmp_8;     \
FLAG_NP_SET(s_tmp_value);                       \
WRITE_FUNC(s_tmp_addr, s_tmp_value);            \
ADD_CYCLES(CYCLES);

#define LSR_A                   \
s_nes_regs.C = (s_nes_regs.A & 1);      \
s_nes_regs.A >>= 1;                 \
FLAG_NP_SET(s_nes_regs.A);          \
ADD_CYCLES(2);

#define LSR_NEW(CYCLES, READ_FUNC, WRITE_FUNC)  \
READ_FUNC;                                      \
s_nes_regs.C = (s_tmp_value & 1);                   \
s_tmp_value >>= 1;                              \
FLAG_NP_SET(s_tmp_value);                       \
WRITE_FUNC(s_tmp_addr, s_tmp_value);            \
ADD_CYCLES(CYCLES);

#define ADC_NEW(CYCLES, READ_FUNC)                                          \
READ_FUNC;                                                                  \
s_tmp_16 = (uint16_t)s_tmp_value + (uint16_t)s_nes_regs.A + s_nes_regs.C;           \
s_nes_regs.C = (s_tmp_16 >> 8) & 1;                                             \
s_nes_regs.V = ((~(s_nes_regs.A ^ s_tmp_value)) & (s_nes_regs.A ^ s_tmp_16) & 0x80);    \
s_nes_regs.A = (uint8_t)s_tmp_16;                                               \
FLAG_NP_SET(s_nes_regs.A);                                                      \
ADD_CYCLES(CYCLES);

#define SBC_NEW(CYCLES, READ_FUNC)                                          \
READ_FUNC;                                                                  \
s_tmp_16 = (uint16_t)s_nes_regs.A - (uint16_t)s_tmp_value - (s_nes_regs.C ^ 1);     \
s_nes_regs.C = ((s_tmp_16 >> 8) & 1) ^ 1;                                       \
s_nes_regs.V = ((s_nes_regs.A ^ s_tmp_value) & (s_nes_regs.A ^ s_tmp_16) & 0x80);       \
s_nes_regs.A = (uint8_t)s_tmp_16;                                               \
FLAG_NP_SET(s_nes_regs.A);                                                      \
ADD_CYCLES(CYCLES);

#define DEC_NEW(CYCLES, READ_FUNC, WRITE_FUNC)  \
READ_FUNC;                                      \
s_tmp_value --;                                 \
FLAG_NP_SET(s_tmp_value);                       \
WRITE_FUNC(s_tmp_addr, s_tmp_value);            \
ADD_CYCLES(CYCLES);

#define INC_NEW(CYCLES, READ_FUNC, WRITE_FUNC)  \
READ_FUNC;                                      \
s_tmp_value ++;                                 \
FLAG_NP_SET(s_tmp_value);                       \
WRITE_FUNC(s_tmp_addr, s_tmp_value);            \
ADD_CYCLES(CYCLES);

#define SEC_NEW     \
s_nes_regs.C = 1;       \
ADD_CYCLES(2);

#define CLC_NEW     \
s_nes_regs.C = 0;       \
ADD_CYCLES(2);

#define CLI_NEW     \
s_nes_regs.I = 0;       \
ADD_CYCLES(2);

#define SEI_NEW     \
s_nes_regs.I = 1;       \
ADD_CYCLES(2);

#define CLV_NEW     \
s_nes_regs.V = 0;       \
ADD_CYCLES(2);

#define CLD_NEW     \
s_nes_regs.D = 0;       \
ADD_CYCLES(2);

#define SED_NEW     \
s_nes_regs.D = 1;       \
ADD_CYCLES(2);

#define RTI_NEW         \
s_tmp_8 = POP_8;        \
SET_FLAGS(s_tmp_8);     \
s_nes_regs.PC = POP_8;      \
s_nes_regs.PC |= POP_8 << 8;\
ADD_CYCLES(6);

#define RTS_NEW         \
s_nes_regs.PC = POP_8;      \
s_nes_regs.PC |= POP_8 << 8;\
s_nes_regs.PC ++;           \
ADD_CYCLES(6);

#define PHA_NEW     \
PUSH8(s_nes_regs.A);    \
ADD_CYCLES(3);

#define PHP_NEW     \
PUSH8(GET_FLAGS);   \
ADD_CYCLES(3);

#define PLA_NEW         \
s_nes_regs.A = POP_8;       \
FLAG_NP_SET(s_nes_regs.A);  \
ADD_CYCLES(4);

#define PLP_NEW      \
s_tmp_8 = POP_8;    \
SET_FLAGS(s_tmp_8); \
ADD_CYCLES(4);

#define JMP_ABS_NEW     \
ABSOLUTE_ADDR;          \
s_nes_regs.PC = s_tmp_addr; \
ADD_CYCLES(3);

#define JMP_IND_NEW     \
ABSOLUTE_ADDR;          \
s_nes_regs.PC = MEMORY_READ(s_tmp_addr);    \
s_tmp_addr = (s_tmp_addr & 0xFF00) | ( ((s_tmp_addr & 0xFF) + 1) & 0xFF );  \
s_nes_regs.PC |= ( MEMORY_READ(s_tmp_addr) << 8);   \
ADD_CYCLES(5);

#define STA_NEW(CYCLES, READ_ADDR_FUNC, WRITE_FUNC) \
READ_ADDR_FUNC;                                     \
WRITE_FUNC(s_tmp_addr, s_nes_regs.A);                   \
ADD_CYCLES(CYCLES);

#define STX_NEW(CYCLES, READ_ADDR_FUNC, WRITE_FUNC) \
READ_ADDR_FUNC;                                     \
WRITE_FUNC(s_tmp_addr, s_nes_regs.X);                   \
ADD_CYCLES(CYCLES);

#define STY_NEW(CYCLES, READ_ADDR_FUNC, WRITE_FUNC) \
READ_ADDR_FUNC;                                     \
WRITE_FUNC(s_tmp_addr, s_nes_regs.Y);                   \
ADD_CYCLES(CYCLES);

#define CPY_NEW(CYCLES, READ_ADDR_FUNC)             \
READ_ADDR_FUNC;                                     \
s_tmp_16 = (uint16_t)s_nes_regs.Y - (uint16_t)s_tmp_value;  \
FLAG_NP_SET((uint8_t)s_tmp_16);                      \
s_nes_regs.C = ((s_tmp_16 & 0x100) >> 8) ^ 1;           \
ADD_CYCLES(CYCLES);

#define CPX_NEW(CYCLES, READ_ADDR_FUNC)             \
READ_ADDR_FUNC;                                     \
s_tmp_16 = (uint16_t)s_nes_regs.X - (uint16_t)s_tmp_value;  \
FLAG_NP_SET((uint8_t)s_tmp_16);                      \
s_nes_regs.C = ((s_tmp_16 & 0x100) >> 8) ^ 1;           \
ADD_CYCLES(CYCLES);

#define CMP_NEW(CYCLES, READ_ADDR_FUNC)             \
READ_ADDR_FUNC;                                     \
s_tmp_16 = (uint16_t)s_nes_regs.A - (uint16_t)s_tmp_value;  \
FLAG_NP_SET((uint8_t)s_tmp_16);                     \
s_nes_regs.C = ((s_tmp_16 & 0x100) >> 8) ^ 1;           \
ADD_CYCLES(CYCLES);

#define DEX_NEW         \
s_nes_regs.X -= 1;          \
FLAG_NP_SET(s_nes_regs.X);  \
ADD_CYCLES(2);

#define DEY_NEW         \
s_nes_regs.Y -= 1;          \
FLAG_NP_SET(s_nes_regs.Y);  \
ADD_CYCLES(2);

#define TXA_NEW         \
s_nes_regs.A = s_nes_regs.X;    \
FLAG_NP_SET(s_nes_regs.X);  \
ADD_CYCLES(2);

#define TXS_NEW         \
s_nes_regs.S = s_nes_regs.X;    \
ADD_CYCLES(2);

#define TSX_NEW         \
s_nes_regs.X = s_nes_regs.S;    \
FLAG_NP_SET(s_nes_regs.X);  \
ADD_CYCLES(2);

#define TYA_NEW         \
s_nes_regs.A = s_nes_regs.Y;    \
FLAG_NP_SET(s_nes_regs.Y);  \
ADD_CYCLES(2);

#define TAY_NEW         \
s_nes_regs.Y = s_nes_regs.A;    \
FLAG_NP_SET(s_nes_regs.Y);  \
ADD_CYCLES(2);

#define TAX_NEW         \
s_nes_regs.X = s_nes_regs.A;    \
FLAG_NP_SET(s_nes_regs.X);  \
ADD_CYCLES(2);

#define INX_NEW           \
s_nes_regs.X ++;              \
FLAG_NP_SET(s_nes_regs.X);    \
ADD_CYCLES(2);

#define INY_NEW           \
s_nes_regs.Y ++;              \
FLAG_NP_SET(s_nes_regs.Y);    \
ADD_CYCLES(2);

#define NOP_EMU           \
ADD_CYCLES(2);

typedef void (*Command_func)();

struct Command
{
  const char*     mnemonic;
  uint8_t         size;
  uint8_t         cycle;
};
  
static const Command Cmd_List[256] =
{
    /*00*/ { "BRK",             1,  7 },
    /*01*/ { "ORA IND X",       2,  6 },
    /*02*/ { "NON 02",          1,  1 },
    /*03*/ { "NON 03",          1,  1 },
    /*04*/ { "NON 04",          1,  1 },
    /*05*/ { "ORA ZP",          2,  3 },
    /*06*/ { "ASL ZP",          2,  5 },
    /*07*/ { "NON 07",          1,  1 },
    /*08*/ { "PHP",             1,  3 },
    /*09*/ { "ORA IMM",         2,  2 },
    /*0A*/ { "ASL",             1,  2 },
    /*0B*/ { "NON 0B",          1,  1 },
    /*0C*/ { "NON 0C",          1,  1 },
    /*0D*/ { "ORA ABS",         3,  4 },
    /*0E*/ { "ASL ABS",         3,  6 },
    /*0F*/ { "NON 0F",          1,  1 },
    /*10*/ { "BPL",             2,  2 },
    /*11*/ { "ORA IND Y",       2,  5 },
    /*12*/ { "NON 12",          1,  1 },
    /*13*/ { "NON 13",          1,  1 },
    /*14*/ { "NON 14",          1,  1 },
    /*15*/ { "ORA ZP X",        2,  4 },
    /*16*/ { "ASL ZP X",        2,  6 },
    /*17*/ { "NON 17",          1,  1 },
    /*18*/ { "CLC",             1,  2 },
    /*19*/ { "ORA ABS Y",       3,  4 },
    /*1A*/ { "NON 1A",          1,  1 },
    /*1B*/ { "NON 1B",          1,  1 },
    /*1C*/ { "NON 1C",          1,  1 },
    /*1D*/ { "ORA ABS X",       3,  4 },
    /*1E*/ { "ASL ABS X",       3,  7 },
    /*1F*/ { "NON 1F",          1,  1 },
    /*20*/ { "JSR",             3,  6 },
    /*21*/ { "AND IND X",       2,  6 },
    /*22*/ { "NON 22",          1,  1 },
    /*23*/ { "NON 23",          1,  1 },
    /*24*/ { "BIT ZP",          2,  3 },
    /*25*/ { "AND ZP",          2,  3 },
    /*26*/ { "ROL ZP",          2,  5 },
    /*27*/ { "NON 27",          1,  1 },
    /*28*/ { "PLP",             1,  4 },
    /*29*/ { "AND IMM",         2,  2 },
    /*2A*/ { "ROL",             1,  2 },
    /*2B*/ { "NON 2B",          1,  1 },
    /*2C*/ { "BIT ABS",         3,  4 },
    /*2D*/ { "AND ABS",         3,  4 },
    /*2E*/ { "ROL ABS",         3,  6 },
    /*2F*/ { "NON 2F",          1,  1 },
    /*30*/ { "BMI",             2,  2 },
    /*31*/ { "AND IND Y",       2,  5 },
    /*32*/ { "NON 32",          1,  1 },
    /*33*/ { "NON 33",          1,  1 },
    /*34*/ { "NON 34",          1,  1 },
    /*35*/ { "AND ZP X",        2,  4 },
    /*36*/ { "ROL ZP X",        2,  6 },
    /*37*/ { "NON 37",          1,  1 },
    /*38*/ { "SEC",             1,  2 },
    /*39*/ { "AND ABS Y",       3,  4 },
    /*3A*/ { "NON 3A",          1,  1 },
    /*3B*/ { "NON 3B",          1,  1 },
    /*3C*/ { "NON 3C",          1,  1 },
    /*3D*/ { "AND ABS X",       3,  4 },
    /*3E*/ { "ROL ABS X",       3,  7 },
    /*3F*/ { "NON 3F",          1,  1 },
    
    /*40*/ { "RTI",             1,  6 },
    /*41*/ { "EOR IND X",       2,  6 },
    /*42*/ { "NON 42",          1,  1 },
    /*43*/ { "NON 43",          1,  1 },
    /*44*/ { "NON 44",          1,  1 },
    /*45*/ { "EOR ZP",          2,  3 },
    /*46*/ { "LSR ZP",          2,  5 },
    /*47*/ { "NON 47",          1,  1 },
    /*48*/ { "PHA",             1,  3 },
    /*49*/ { "EOR IMM",         2,  2 },
    /*4A*/ { "LSR",             1,  2 },
    /*4B*/ { "NON 4B",          1,  1 },
    /*4C*/ { "JMP ABS",         3,  3 },
    /*4D*/ { "EOR ABS",         3,  4 },
    /*4E*/ { "LSR ABS",         3,  6 },
    /*4F*/ { "NON 4F",          1,  1 },
    /*50*/ { "BVC",             2,  2 },
    /*51*/ { "EOR IND Y",       2,  5 },
    /*52*/ { "NON 52",          1,  1 },
    /*53*/ { "NON 53",          1,  1 },
    /*54*/ { "NON 54",          1,  1 },
    /*55*/ { "EOR ZP X",        2,  4 },
    /*56*/ { "LSR ZP X",        2,  6 },
    /*57*/ { "NON 57",          1,  1 },
    /*58*/ { "CLI",             1,  2 },
    /*59*/ { "EOR ABS Y",       3,  4 },
    /*5A*/ { "NON 5A",          1,  1 },
    /*5B*/ { "NON 5B",          1,  1 },
    /*5C*/ { "NON 5C",          1,  1 },
    /*5D*/ { "EOR ABS X",       3,  4 },
    /*5E*/ { "LSR ABS X",       3,  7 },
    /*5F*/ { "NON 5F",          1,  1 },
    /*60*/ { "RTS",             1,  6 },
    /*61*/ { "ADC IND X",       2,  6 },
    /*62*/ { "NON 62",          1,  1 },
    /*63*/ { "NON 63",          1,  1 },
    /*64*/ { "NON 64",          1,  1 },
    /*65*/ { "ADC ZP",          2,  2 },
    /*66*/ { "ROR_ZP",          2,  5 },
    /*67*/ { "NON 67",          1,  1 },
    /*68*/ { "PLA",             1,  4 },
    /*69*/ { "ADC IMM",         2,  3 },
    /*6A*/ { "ROR",             1,  2 },
    /*6B*/ { "NON 6B",          1,  1 },
    /*6C*/ { "JMP IND",         3,  5 },
    /*6D*/ { "ADC ABS",         3,  4 },
    /*6E*/ { "ROR_ABS",         3,  6 },
    /*6F*/ { "NON 6F",          1,  1 },
    /*70*/ { "BVS",             2,  2 },
    /*71*/ { "ADC IND Y",       2,  5 },
    /*72*/ { "NON 72",          1,  1 },
    /*73*/ { "NON 73",          1,  1 },
    /*74*/ { "NON 74",          1,  1 },
    /*75*/ { "ADC ZP X",        2,  4 },
    /*76*/ { "ROR_ZP_X",        2,  6 },
    /*77*/ { "NON 77",          1,  1 },
    /*78*/ { "SEI",             1,  2 },
    /*79*/ { "ADC ABS Y",       3,  4 },
    /*7A*/ { "NON 7A",          1,  1 },
    /*7B*/ { "NON 7B",          1,  1 },
    /*7C*/ { "NON 7C",          1,  1 },
    /*7D*/ { "ADC ABS X",       3,  4 },
    /*7E*/ { "ROR_ABS_X",       3,  7 },
    /*7F*/ { "NON 7F",          1,  1 },

    /*80*/ { "NON 80",          1,  1 },
    /*81*/ { "STA IND X",       2,  6 },
    /*82*/ { "NON 82",          1,  1 },
    /*83*/ { "NON 83",          1,  1 },
    /*84*/ { "STY ZP",          2,  3 },
    /*85*/ { "STA ZP",          2,  3 },
    /*86*/ { "STX ZP",          2,  3 },
    /*87*/ { "NON 87",          1,  1 },
    /*88*/ { "DEY",             1,  2 },
    /*89*/ { "NON 89",          1,  1 },
    /*8A*/ { "TXA",             1,  2 },
    /*8B*/ { "NON 8B",          1,  1 },
    /*8C*/ { "STY ABS",         3,  4 },
    /*8D*/ { "STA ABS",         3,  4 },
    /*8E*/ { "STX ABS",         3,  4 },
    /*8F*/ { "NON 8F",          1,  1 },
    /*90*/ { "BCC",             2,  2 },
    /*91*/ { "STA IND Y",       2,  6 },
    /*92*/ { "NON 92",          1,  1 },
    /*93*/ { "NON 93",          1,  1 },
    /*94*/ { "STY ZP X",        2,  4 },
    /*95*/ { "STA ZP X",        2,  4 },
    /*96*/ { "STX ZP Y",        2,  4 },
    /*97*/ { "NON 97",          1,  1 },
    /*98*/ { "TYA",             1,  2 },
    /*99*/ { "STA ABS Y",       3,  5 },
    /*9A*/ { "TXS",             1,  2 },
    /*9B*/ { "NON 9B",          1,  1 },
    /*9C*/ { "NON 9C",          1,  1 },
    /*9D*/ { "STA ABS X",       3,  5 },
    /*9E*/ { "NON 9E",          1,  1 },
    /*9F*/ { "NON 9F",          1,  1 },
    /*A0*/ { "LDY IMM",         2,  2 },
    /*A1*/ { "LDA IND X",       2,  6 },
    /*A2*/ { "LDX IMM",         2,  2 },
    /*A3*/ { "NON A3",          1,  1 },
    /*A4*/ { "LDY ZP",          2,  3 },
    /*A5*/ { "LDA ZP",          2,  3 },
    /*A6*/ { "LDX ZP",          2,  3 },
    /*A7*/ { "NON A7",          1,  1 },
    /*A8*/ { "TAY",             1,  2 },
    /*A9*/ { "LDA IMM",         2,  2 },
    /*AA*/ { "TAX",             1,  2 },
    /*AB*/ { "NON AB",          1,  1 },
    /*AC*/ { "LDY ABS",         3,  4 },
    /*AD*/ { "LDA ABS",         3,  4 },
    /*AE*/ { "LDX ABS",         3,  4 },
    /*AF*/ { "NON AF",          1,  1 },
    /*B0*/ { "BCS",             2,  2 },
    /*B1*/ { "LDA IND Y",       2,  5 },
    /*B2*/ { "NON B2",          1,  1 },
    /*B2*/ { "NON B2",          1,  1 },
    /*B4*/ { "LDY ZP X",        2,  4 },
    /*B5*/ { "LDA ZP X",        2,  4 },
    /*B6*/ { "LDX ZP Y",        2,  4 },
    /*B7*/ { "NON B7",          1,  1 },
    /*B8*/ { "CLV",             1,  2 },
    /*B9*/ { "LDA ABS Y",       3,  4 },
    /*BA*/ { "TSX",             1,  2 },
    /*BB*/ { "NON BB",          1,  1 },
    /*BC*/ { "LDY ABS X",       3,  4 },
    /*BD*/ { "LDA ABS X",       3,  4 },
    /*BE*/ { "LDX ABS Y",       3,  4 },
    /*BF*/ { "NON BF",          1,  1 },

    /*C0*/ { "CPY IMM",         2,  2 },
    /*C1*/ { "CMP IND X",       2,  6 },
    /*C2*/ { "NON C2",          1,  1 },
    /*C3*/ { "NON C3",          1,  1 },
    /*C4*/ { "CPY ZP",          2,  3 },
    /*C5*/ { "CMP ZP",          2,  3 },
    /*C6*/ { "DEC ZP",          2,  5 },
    /*C7*/ { "NON C7",          1,  1 },
    /*C8*/ { "INY",             1,  2 },
    /*C9*/ { "CMP IMM",         2,  2 },
    /*CA*/ { "DEX",             1,  2 },
    /*CB*/ { "NON CB",          1,  1 },
    /*CC*/ { "CPY ABS",         3,  4 },
    /*CD*/ { "CMP ABS",         3,  4 },
    /*CE*/ { "DEC ABS",         3,  6 },
    /*CF*/ { "NON CF",          1,  1 },
    /*D0*/ { "BNE",             2,  2 },
    /*D1*/ { "CMP IND Y",       2,  5 },
    /*D2*/ { "NON D2",          1,  1 },
    /*D3*/ { "NON D3",          1,  1 },
    /*D4*/ { "NON D4",          1,  1 },
    /*D5*/ { "CMP ZP X",        2,  4 },
    /*D6*/ { "DEC ZP X",        2,  6 },
    /*D7*/ { "NON D7",          1,  1 },
    /*D8*/ { "CLD",             1,  2 },
    /*D9*/ { "CMP ABS Y",       3,  4 },
    /*DA*/ { "NON DA",          1,  1 },
    /*DB*/ { "NON DB",          1,  1 },
    /*DC*/ { "NON DC",          1,  1 },
    /*DD*/ { "CMP ABS X",       3,  4 },
    /*DE*/ { "DEC ABS X",       3,  7 },
    /*DF*/ { "NON DF",          1,  1 },
    /*E0*/ { "CPX IMM",         2,  2 },
    /*E1*/ { "SBC IND X",       2,  6 },
    /*E2*/ { "NON E2",          1,  1 },
    /*E3*/ { "NON E3",          1,  1 },
    /*E4*/ { "CPX ZP",          2,  3 },
    /*E5*/ { "SBC ZP",          2,  3 },
    /*E6*/ { "INC ZP",          2,  5 },
    /*E7*/ { "NON E7",          1,  1 },
    /*E8*/ { "INX",             1,  2 },
    /*E9*/ { "SBC IMM",         2,  2 },
    /*EA*/ { "NOP",             1,  2 },
    /*EB*/ { "NON EB",          1,  1 },
    /*EC*/ { "CPX ABS",         3,  4 },
    /*ED*/ { "SBC ABS",         3,  4 },
    /*EE*/ { "INC ABS",         3,  6 },
    /*EF*/ { "NON EF",          1,  1 },
    /*F0*/ { "BEQ",             2,  2 },
    /*F1*/ { "SBC IND Y",       2,  5 },
    /*F2*/ { "NON F2",          1,  1 },
    /*F3*/ { "NON F3",          1,  1 },
    /*F4*/ { "NON F4",          1,  1 },
    /*F5*/ { "SBC ZP X",        2,  4 },
    /*F6*/ { "INC ZP X",        2,  6 },
    /*F7*/ { "NON F7",          1,  1 },
    /*F8*/ { "SED",             1,  2 },
    /*F9*/ { "SBC ABS Y",       3,  4 },
    /*FA*/ { "NON FA",          1,  1 },
    /*FB*/ { "NON FB",          1,  1 },
    /*FC*/ { "NON FC",          1,  1 },
    /*FD*/ { "SBC ABS X",       3,  4 },
    /*FE*/ { "INC ABS X",       3,  7 },
    /*FF*/ { "NON FF",          1,  1 }
};

void CPU_6502_Init()
{
  s_nes_regs.S = 0xFF;
  s_nes_regs.Z = 1;
  s_nes_regs.I = 1;
  s_nes_regs.U = 1;

  s_nes_cycles = 0;
  s_nes_regs.PC = Memory_Read(0xFFFC) | ( Memory_Read(0xFFFD) << 8);
#ifndef WIN32
  Serial.print("CPU_6502_Init PC=0x");
  Serial.println((int)s_nes_regs.PC, HEX);
#endif
}

void CPU_6502_Step()
{
    uint8_t cmd = Memory_Read(s_nes_regs.PC);
    /*if (s_nes_regs.PC == 0x8EED)
    {
      std::cerr << "0x8EED at " << ppu_scanline << std::endl;
    }*/
#ifdef WIN32
#if 0
    {
        //std::stringstream stream;
        std::cout << "PC: 0x" << std::hex << std::setw(4) << std::setfill('0') << (int)s_nes_regs.PC << " cmd: " << Cmd_List[cmd].mnemonic;
        if (Cmd_List[cmd].size == 2)
        {
            std::cout << " 0x" << std::hex << std::setw(4) << std::setfill('0') << (int)Memory_Read(s_nes_regs.PC + 1) << std::endl;
        }
        else if (Cmd_List[cmd].size == 3)
        {
            uint8_t v0 = Memory_Read(s_nes_regs.PC + 1);
            uint8_t v1 = Memory_Read(s_nes_regs.PC + 2);
            uint16_t val = v0 | (v1 << 8);
            std::cout << " 0x" << std::hex << std::setw(4) << std::setfill('0') << (int)val << std::endl;;
        }
        else
            std::cout << std::endl;
        //OutputDebugStringA(stream.str().c_str());
    }
#endif
#endif

  /*if (  s_nes_regs.PC != 0x8018
   && s_nes_regs.PC != 0x801A
   && s_nes_regs.PC != 0x8015
   
   && s_nes_regs.PC != 0x8021
   && s_nes_regs.PC != 0x8024
   && s_nes_regs.PC != 0x8026

   && s_nes_regs.PC != 0x802D
   && s_nes_regs.PC != 0x8030
   && s_nes_regs.PC != 0x8032

   && s_nes_regs.PC != 0x8088
     && s_nes_regs.PC != 0x8089
     && s_nes_regs.PC != 0x808A
     && s_nes_regs.PC != 0x808B
    
   )
  {
  Serial.print("PC: 0x");
  Serial.print((int)s_nes_regs.PC, HEX);
  Serial.print(" cmd: ");
  Serial.print(Cmd_List[cmd].mnemonic);
  if ( Cmd_List[cmd].size == 2 )
  {
    Serial.print(" 0x");
    Serial.println((int)Memory_Read(s_nes_regs.PC+1), HEX);
  } else if ( Cmd_List[cmd].size == 3 )
  {
    Serial.print(" 0x");
    uint8_t v0 = Memory_Read(s_nes_regs.PC+1);
    uint8_t v1 = Memory_Read(s_nes_regs.PC+2);
    uint16_t val = v0 | (v1 << 8);
    Serial.println((int)val, HEX);
  } else
    Serial.println("");
  }*/
  
  /*if (s_nes_regs.PC == 0xB361)
  {
    s_ram[0x59a] = 0x04;  //HP cheat

    uint16_t graphics_addr = s_ram[0x3A] | (s_ram[0x3B] << 8);
    uint16_t tiles = s_ram[0x3C];
    std::cerr << "Graphics at " << std::hex << (int)graphics_addr << " cnt=" << std::dec << (int)tiles << std::endl;
  }*/

  s_nes_regs.PC++;
  switch(cmd)
  {
  case 0x00:
      BRK_NEW
    break;
  case 0x01:
      ORA_NEW(6, IND_X_VALUE)
    break;
  case 0x02:
  case 0x03:
  case 0x04:
      NOT_IMPLEMENTED
      break;
  case 0x05:
      ORA_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0x06:
      ASL_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x07:
      NOT_IMPLEMENTED
    break;
  case 0x08:
      PHP_NEW
    break;
  case 0x09:
      ORA_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0x0A:
      ASL_A
    break;
  case 0x0B:
  case 0x0C:
      NOT_IMPLEMENTED
    break;
  case 0x0D:
      ORA_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0x0E:
      ASL_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE)
    break;
  case 0x0F:
      NOT_IMPLEMENTED
    break;
  case 0x10:    //BPL
      REL_BRANCH_NEW((s_nes_regs.N & BIT_N) == 0)
    break;
  case 0x11:
      ORA_NEW(5, IND_Y_VALUE)
    break;
  case 0x12:
  case 0x13:
  case 0x14:
      NOT_IMPLEMENTED
    break;
  case 0x15:
      ORA_NEW(4, ZP_X_VALUE)
    break;
  case 0x16:
      ASL_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x17:
      NOT_IMPLEMENTED
    break;
  case 0x18:
      CLC_NEW
    break;
  case 0x19:
      ORA_NEW(4, ABS_Y_VALUE)
    break;
  case 0x1A:
  case 0x1B:
  case 0x1C:
      NOT_IMPLEMENTED
    break;
  case 0x1D:
      ORA_NEW(4, ABS_X_VALUE)
    break;
  case 0x1E:
      ASL_NEW(7, ABS_X_VALUE, MEMORY_WRITE)
    break;
  case 0x1F:
      NOT_IMPLEMENTED
    break;
  case 0x20:
      JSR_NEW
    break;
  case 0x21:
      AND_NEW(6, IND_X_VALUE)
    break;
  case 0x22:
  case 0x23:
      NOT_IMPLEMENTED
      break;
  case 0x24:
      BIT_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0x25:
      AND_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0x26:
      ROL_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x27:
      NOT_IMPLEMENTED
    break;
  case 0x28:
      PLP_NEW
    break;
  case 0x29:
      AND_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0x2A:
      ROL_A
    break;
  case 0x2B:
      NOT_IMPLEMENTED
    break;
  case 0x2C:
      BIT_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0x2D:
      AND_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0x2E:
      ROL_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE)
    break;
  case 0x2F:
      NOT_IMPLEMENTED
    break;
  case 0x30:
      REL_BRANCH_NEW(s_nes_regs.N & BIT_N)
    break;
  case 0x31:
      AND_NEW(5, IND_Y_VALUE)
    break;
  case 0x32:
  case 0x33:
  case 0x34:
      NOT_IMPLEMENTED
    break;
  case 0x35:
      AND_NEW(4, ZP_X_VALUE)
    break;
  case 0x36:
      ROL_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x37:
      NOT_IMPLEMENTED
    break;
  case 0x38:
      SEC_NEW
    break;
  case 0x39:
      AND_NEW(4, ABS_Y_VALUE)
    break;
  case 0x3A:
  case 0x3B:
  case 0x3C:
      NOT_IMPLEMENTED
    break;
  case 0x3D:
      AND_NEW(4, ABS_X_VALUE)
    break;
  case 0x3E:
      ROL_NEW(7, ABS_X_VALUE, MEMORY_WRITE)
    break;
  case 0x3F:
      NOT_IMPLEMENTED
    break;
  case 0x40:
      RTI_NEW
    break;
  case 0x41:
      EOR_NEW(6, IND_X_VALUE)
    break;
  case 0x42:
  case 0x43:
  case 0x44:
      NOT_IMPLEMENTED
    break;
  case 0x45:
      EOR_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0x46:
      LSR_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x47:
      NOT_IMPLEMENTED
    break;
  case 0x48:
      PHA_NEW
    break;
  case 0x49:
      EOR_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0x4A:
      LSR_A
    break;
  case 0x4B:
      NOT_IMPLEMENTED
    break;
  case 0x4C:
      JMP_ABS_NEW
      /*ABSOLUTE_ADDR;
      if (s_nes_regs.PC == s_tmp_addr + 3)
      {
        s_cycles += ((-s_cycles + 2) / 3) * 3;  //SMB optimization
      } else
      {
        ADD_CYCLES(3);
      }
      s_nes_regs.PC = s_tmp_addr;*/
    break;
  case 0x4D:
      EOR_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0x4E:
      LSR_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE)
    break;
  case 0x4F:
      NOT_IMPLEMENTED
    break;
  case 0x50:
      REL_BRANCH_NEW(s_nes_regs.V == 0)
    break;
  case 0x51:
      EOR_NEW(5, IND_Y_VALUE)
    break;
  case 0x52:
  case 0x53:
  case 0x54:
      NOT_IMPLEMENTED
    break;
  case 0x55:
      EOR_NEW(4, IND_X_VALUE)
    break;
  case 0x56:
      LSR_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x57:
      NOT_IMPLEMENTED
    break;
  case 0x58:
      CLI_NEW
    break;
  case 0x59:
      EOR_NEW(4, ABS_Y_VALUE)
    break;
  case 0x5A:
  case 0x5B:
  case 0x5C:
      NOT_IMPLEMENTED
    break;
  case 0x5D:
      EOR_NEW(4, ABS_X_VALUE)
    break;
  case 0x5E:
      LSR_NEW(7, ABS_X_VALUE, MEMORY_WRITE)
    break;
  case 0x5F:
      NOT_IMPLEMENTED
    break;
  case 0x60:
  {
      if (s_nes_regs.S == 0xFF)
      {
          s_nes_execution_finished = 1;
          return;
      }
      RTS_NEW
  }
    break;
  case 0x61:
      ADC_NEW(6, IND_X_VALUE)
    break;
  case 0x62:
  case 0x63:
  case 0x64:
      NOT_IMPLEMENTED
    break;
  case 0x65:
      ADC_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0x66:
      ROR_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x67:
      NOT_IMPLEMENTED
    break;
  case 0x68:
      PLA_NEW
    break;      
  case 0x69:
      ADC_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0x6A:
      ROR_A
    break;
  case 0x6B:
      NOT_IMPLEMENTED
    break;
  case 0x6C:
      JMP_IND_NEW;
    break;
  case 0x6D:
      ADC_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0x6E:
      ROR_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE)
    break;
  case 0x6F:
      NOT_IMPLEMENTED
    break;
  case 0x70:
      REL_BRANCH_NEW(s_nes_regs.V != 0)
    break;
  case 0x71:
      ADC_NEW(5, IND_Y_VALUE)
    break;
  case 0x72:
  case 0x73:
  case 0x74:
      NOT_IMPLEMENTED
    break;
  case 0x75:
      ADC_NEW(4, ZP_X_VALUE)
    break;
  case 0x76:
      ROR_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0x77:
      NOT_IMPLEMENTED
    break;
  case 0x78:
      SEI_NEW
    break;
  case 0x79:
      ADC_NEW(4, ABS_Y_VALUE)
    break;
  case 0x7A:
  case 0x7B:
  case 0x7C:
      NOT_IMPLEMENTED
    break;
  case 0x7D:
      ADC_NEW(4, ABS_X_VALUE)
    break;
  case 0x7E:
      ROR_NEW(7, ABS_X_VALUE, MEMORY_WRITE)
    break;
  case 0x7F:
      NOT_IMPLEMENTED
    break;

  case 0x80:
      NOT_IMPLEMENTED
    break;
  case 0x81:
      STA_NEW(6, IND_X_ADDR, MEMORY_WRITE)
    break;
  case 0x82:
  case 0x83:
      NOT_IMPLEMENTED
    break;
  case 0x84:
      STY_NEW(3, ZERO_PAGE_ADDR, MEMORY_WRITE_ZP)
    break;
  case 0x85:
      STA_NEW(3, ZERO_PAGE_ADDR, MEMORY_WRITE_ZP)
    break;
  case 0x86:
      STX_NEW(3, ZERO_PAGE_ADDR, MEMORY_WRITE_ZP)
    break;
  case 0x87:
      NOT_IMPLEMENTED
    break;
  case 0x88:
      DEY_NEW
    break;  
  case 0x89:
      NOT_IMPLEMENTED
    break;
  case 0x8A:
      TXA_NEW
    break;
  case 0x8B:
      NOT_IMPLEMENTED
    break;
  case 0x8C:
      STY_NEW(4, ABSOLUTE_ADDR, MEMORY_WRITE)
    break;
  case 0x8D:
      STA_NEW(4, ABSOLUTE_ADDR, MEMORY_WRITE)
      break;
  case 0x8E:
      STX_NEW(4, ABSOLUTE_ADDR, MEMORY_WRITE)
    break;
  case 0x8F:
      NOT_IMPLEMENTED
    break;
  case 0x90:
      REL_BRANCH_NEW(s_nes_regs.C == 0)
      break;
  case 0x91:
      STA_NEW(6, IND_Y_ADDR, MEMORY_WRITE)
    break;
  case 0x92:
  case 0x93:
      NOT_IMPLEMENTED
    break;
  case 0x94:
      STY_NEW(4, ZP_X_ADDR_NEW, MEMORY_WRITE_ZP)
    break;
  case 0x95:
      STA_NEW(4, ZP_X_ADDR_NEW, MEMORY_WRITE_ZP)
    break;
  case 0x96:
      STY_NEW(4, ZP_Y_ADDR_NEW, MEMORY_WRITE_ZP)
    break;
  case 0x97:
      NOT_IMPLEMENTED
    break;
  case 0x98:
      TYA_NEW
    break;
  case 0x99:
      STA_NEW(5, ABS_Y_ADDR, MEMORY_WRITE)
    break;
  case 0x9A:
      TXS_NEW
    break;
  case 0x9B:
  case 0x9C:
      NOT_IMPLEMENTED
    break;
  case 0x9D:
      STA_NEW(5, ABS_X_ADDR, MEMORY_WRITE)
    break;
  case 0x9E:
  case 0x9F:
      NOT_IMPLEMENTED
    break;
  case 0xA0:
	  LDY_NEW(2, IMMEDIATE_VALUE)
	break;
  case 0xA1:
	  LDA_NEW(6, IND_X_VALUE)
	break;
  case 0xA2:
	  LDX_NEW(2, IMMEDIATE_VALUE)
	break;
  case 0xA3:
      NOT_IMPLEMENTED
    break;
  case 0xA4:
	  LDY_NEW(3, ZERO_PAGE_VALUE)
	break;
  case 0xA5:
	  LDA_NEW(3, ZERO_PAGE_VALUE)
	break;
  case 0xA6:
	  LDX_NEW(3, ZERO_PAGE_VALUE)
	break;
  case 0xA7:
      NOT_IMPLEMENTED
    break;
  case 0xA8:
      TAY_NEW
    break;
  case 0xA9:
	  LDA_NEW(2, IMMEDIATE_VALUE)
	break;
  case 0xAA:
      TAX_NEW
    break;
  case 0xAB:
      NOT_IMPLEMENTED
    break;
  case 0xAC:
	  LDY_NEW(4, ABSOLUTE_VALUE)
	break;
  case 0xAD:
	  LDA_NEW(4, ABSOLUTE_VALUE)
	break;
  case 0xAE:
	  LDX_NEW(4, ABSOLUTE_VALUE)
	break;
  case 0xAF:
      NOT_IMPLEMENTED
    break;
  case 0xB0:
      REL_BRANCH_NEW(s_nes_regs.C != 0)
    break;
  case 0xB1:
	  LDA_NEW(5, IND_Y_VALUE)
	break;
  case 0xB2:
  case 0xB3:
      NOT_IMPLEMENTED
    break;
  case 0xB4:
	  LDY_NEW(4, ZP_X_VALUE)
	break;
  case 0xB5:
	  LDA_NEW(4, ZP_X_VALUE)
	break;
  case 0xB6:
	  LDX_NEW(4, ZP_Y_VALUE)
	break;
  case 0xB7:
      NOT_IMPLEMENTED
    break;
  case 0xB8:
      CLV_NEW
    break;
  case 0xB9:
	  LDA_NEW(4, ABS_Y_VALUE)
	break;
  case 0xBA:
	  TSX_NEW
	break;
  case 0xBB:
      NOT_IMPLEMENTED
    break;
  case 0xBC:
	  LDY_NEW(4, ABS_X_VALUE)
	break;
  case 0xBD:
	  LDA_NEW(4, ABS_X_VALUE)
	break;
  case 0xBE:
	  LDX_NEW(4, ABS_Y_VALUE)
	break;
  case 0xBF:
      NOT_IMPLEMENTED
    break;

  case 0xC0:
      CPY_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0xC1:
      CMP_NEW(6, IND_X_VALUE)
    break;
  case 0xC2:
  case 0xC3:
     NOT_IMPLEMENTED
    break;
  case 0xC4:
      CPY_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0xC5:
      CMP_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0xC6:
      DEC_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP);
    break;
  case 0xC7:
     NOT_IMPLEMENTED
    break;
  case 0xC8:
    INY_NEW
    break;
  case 0xC9:
    CMP_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0xCA:
    DEX_NEW
    break;
  case 0xCB:
     NOT_IMPLEMENTED
    break;
  case 0xCC:
      CPY_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0xCD:
      CMP_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0xCE:
      DEC_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE);
    break;
  case 0xCF:
     NOT_IMPLEMENTED
    break;

  case 0xD0:
    REL_BRANCH_NEW(s_nes_regs.Z != 0)
    break;
  case 0xD1:
    CMP_NEW(5, IND_Y_VALUE)
    break;
  case 0xD2:
  case 0xD3:
  case 0xD4:
     NOT_IMPLEMENTED
    break;
  case 0xD5:
    CMP_NEW(4, ZP_X_VALUE)
    break;
  case 0xD6:
      DEC_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP);
    break;
  case 0xD7:
     NOT_IMPLEMENTED
    break;
  case 0xD8:
    CLD_NEW
    break;
  case 0xD9:
    CMP_NEW(4, ABS_Y_VALUE)
    break;
  case 0xDA:
  case 0xDB:
  case 0xDC:
     NOT_IMPLEMENTED
    break;
  case 0xDD:
    CMP_NEW(4, ABS_X_VALUE)
    break;
  case 0xDE:
    DEC_NEW(7, ABS_X_VALUE, MEMORY_WRITE);
    break;
  case 0xDF:
    NOT_IMPLEMENTED
    break;

  case 0xE0:
    CPX_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0xE1:
    SBC_NEW(6, IND_X_VALUE)
    break;
  case 0xE2:
  case 0xE3:
    NOT_IMPLEMENTED
    break;
  case 0xE4:
    CPX_NEW(3, ZERO_PAGE_VALUE)
      break;
  case 0xE5:
    SBC_NEW(3, ZERO_PAGE_VALUE)
    break;
  case 0xE6:
    INC_NEW(5, ZERO_PAGE_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0xE7:
    NOT_IMPLEMENTED
    break;
  case 0xE8:
    INX_NEW
    break;
  case 0xE9:
    SBC_NEW(2, IMMEDIATE_VALUE)
    break;
  case 0xEA:
    NOP_EMU
    break;
  case 0xEB:
    NOT_IMPLEMENTED
    break;
  case 0xEC:
    CPX_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0xED:
    SBC_NEW(4, ABSOLUTE_VALUE)
    break;
  case 0xEE:
    INC_NEW(6, ABSOLUTE_VALUE, MEMORY_WRITE)
    break;
  case 0xEF:
    NOT_IMPLEMENTED
    break;
  
  case 0xF0:
    REL_BRANCH_NEW(s_nes_regs.Z == 0)
    break;
  case 0xF1:
    SBC_NEW(5, IND_Y_VALUE)
    break;
  case 0xF2:
  case 0xF3:
  case 0xF4:
    NOT_IMPLEMENTED
    break;
  case 0xF5:
    SBC_NEW(4, ZP_X_VALUE)
    break;
  case 0xF6:
    INC_NEW(6, ZP_X_VALUE, MEMORY_WRITE_ZP)
    break;
  case 0xF7:
    NOT_IMPLEMENTED
    break;
  case 0xF8:
    SED_NEW
    break;
  case 0xF9:
    SBC_NEW(4, ABS_Y_VALUE)
    break;
  case 0xFA:
  case 0xFB:
  case 0xFC:
    NOT_IMPLEMENTED
    break;
  case 0xFD:
    SBC_NEW(4, ABS_X_VALUE)
    break;
  case 0xFE:
    INC_NEW(7, ABS_X_VALUE, MEMORY_WRITE)
    break;
  case 0xFF:
    NOT_IMPLEMENTED
    break;
  default:
	  break;
  }
}

void CPU_6502_skip(int cycles)
{
    s_nes_cycles += cycles;
}

void CPU_6502_Execute(int run_cycles, bool apu)
{
  s_nes_frame_cycles += run_cycles;
  s_nes_cycles -= run_cycles;

  while (s_nes_cycles < 0)
  {
    int start_cycle = s_nes_cycles;

    if ( (uint8_t)(s_iqr_mask & IQR_NMI) != 0 )
    {
      s_nes_cycles += 7;
      PUSH16(s_nes_regs.PC);
      PUSH8(GET_FLAGS);
      s_nes_regs.I = 1;
      s_nes_regs.PC = Memory_Read(0xFFFA) | ( Memory_Read(0xFFFB) << 8);
      s_iqr_mask &= ~IQR_NMI;
    }
    
    if ( (uint8_t)(s_iqr_mask & IQR) != 0 && s_nes_regs.I == 0)
    {
      s_nes_cycles += 7;
      PUSH16(s_nes_regs.PC);
      PUSH8(GET_FLAGS);
      s_nes_regs.I = 1;
      s_nes_regs.PC = Memory_Read(0xFFFE) | ( Memory_Read(0xFFFF) << 8);
      s_iqr_mask &= ~IQR;
    }

    CPU_6502_Step();
    int elapsed = s_nes_cycles - start_cycle;

    if (apu)
        APU_cpu_tick(elapsed);
    if (s_nes_execution_finished)
    {
        //s_nes_frame_cycles = 0;
        //s_nes_cycles = 0;
        if (apu)
        {
            while (s_nes_cycles < 0)
            {
                int ticks = s_nes_cycles < -1024 ? 1024 : -s_nes_cycles;
                APU_cpu_tick(ticks);
                s_nes_cycles += ticks;
            }
        } else
            s_nes_cycles = 0;
        return;
    }
  }
}

void CPU_6502_RequestNmi()
{
  s_iqr_mask |= IQR_NMI;
}

void CPU_6502_RequestIrq()
{
  s_iqr_mask |= IQR;
}
