#pragma once
#include <stdint.h>

void APU_Init();
uint8_t APU_pop_sample();
void APU_write(uint16_t addr, uint8_t val);
void APU_cpu_tick(int cycles);