#pragma once
#include <stdint.h>
#include "Registers.h"

extern int32_t s_nes_cycles;
extern int32_t s_nes_frame_cycles;

void CPU_6502_Init();
void CPU_6502_Step();
void CPU_6502_skip(int cycles);
void CPU_6502_Execute(int run_cycles);
void CPU_6502_RequestNmi();
void CPU_6502_RequestIrq();
