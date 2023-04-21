#pragma once
#include <stdint.h>

void Memory_Init();
uint8_t Memory_Read(uint16_t addr);
void Memory_Write(uint16_t addr, uint8_t val);

typedef uint8_t (*MemoryReadFunc)(uint16_t addr);
extern MemoryReadFunc s_nes_memory_read_func[8];

typedef void (*MemoryWriteFunc)(uint16_t addr, uint8_t val);
extern MemoryWriteFunc s_nes_memory_write_func[8];

extern uint8_t s_nes_ram[2048];
//extern uint8_t s_ciram[2048];
extern uint8_t s_nes_sram[8*1024];