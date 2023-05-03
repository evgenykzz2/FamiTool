#pragma once

#include <stdint.h>

struct Registers
{
    volatile uint8_t  A;
    volatile uint8_t  S;
    volatile uint16_t PC;
    volatile uint8_t  X;
    volatile uint8_t  Y;
    
    volatile uint8_t  C;
    volatile uint8_t  Z;
    volatile uint8_t  I;
    volatile uint8_t  D;
    volatile uint8_t  B;
    volatile uint8_t  U;
    volatile uint8_t  V;
    volatile uint8_t  N;
};

//extern const uint8_t s_nes_register_z[256]; 
extern Registers s_nes_regs;
extern uint8_t s_nes_execution_finished;