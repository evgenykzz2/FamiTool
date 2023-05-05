#include "Memory.h"
#include <string.h>
#include "CPU_6502.h"
#include "APU.h"

#ifndef WIN32
#include <Arduino.h>
#else
#include <iostream>
#endif

uint8_t s_nes_ram[2048];
//uint8_t s_ciram[2048];
uint8_t s_nes_sram[8*1024];
//uint8_t s_joy_key = 0;
//uint8_t s_joy_key_shift = 0;

MemoryReadFunc  s_nes_memory_read_func[8];
MemoryWriteFunc s_nes_memory_write_func[8];

static uint8_t Memory_ReadRam(uint16_t addr)
{
	return s_nes_ram[addr & 0x7FF];
}

static uint8_t Memory_ReadSram(uint16_t addr)
{
	return s_nes_sram[addr & 0x1FFF];
}

static uint8_t Memory_ReadPgm0(uint16_t addr)
{
    return 0;
	//return s_pgm_active_ptr[0][addr & 0x1FFF];
}

static uint8_t Memory_ReadPgm1(uint16_t addr)
{
    return 0;
	//return s_pgm_active_ptr[1][addr & 0x1FFF];
}

static uint8_t Memory_ReadPgm2(uint16_t addr)
{
    return 0;
	//return s_pgm_active_ptr[2][addr & 0x1FFF];
}

static uint8_t Memory_ReadPgm3(uint16_t addr)
{
    return 0;
	//return s_pgm_active_ptr[3][addr & 0x1FFF];
}

static void Memory_WriteRam(uint16_t addr, uint8_t val)
{
	s_nes_ram[addr & 0x7FF] = val;
}

static void Memory_WriteSram(uint16_t addr, uint8_t val)
{
	s_nes_sram[addr & 0x1FFF] = val;
}

static void Memory_WritePgm0(uint16_t addr, uint8_t val)
{
	//s_pgm_active_ptr[0][addr & 0x1FFF] = val;
}

static void Memory_WritePgm1(uint16_t addr, uint8_t val)
{
	//s_pgm_active_ptr[1][addr & 0x1FFF] = val;
}

static void Memory_WritePgm2(uint16_t addr, uint8_t val)
{
	//s_pgm_active_ptr[2][addr & 0x1FFF] = val;
}

static void Memory_WritePgm3(uint16_t addr, uint8_t val)
{
	//s_pgm_active_ptr[3][addr & 0x1FFF] = val;
}


static uint8_t Read_APU_Reg(uint16_t addr)
{
  //if( addr == 0x4016 )
  {
    //uint8_t val = (s_joy_key >> s_joy_key_shift) & 0x01;
    //s_joy_key_shift ++;
    //return val;
  } //else
    return 0;
}

static void Write_APU_Reg(uint16_t addr, uint8_t val)
{
  if ( addr == 0x4016 )
  {
    //s_joy_key_shift = 0;
  } else if ( addr == 0x4014 )
  {
    /*
    uint16_t base_addr = (uint16_t)val << 8;
    for ( uint16_t n = 0; n < 256; ++n )
    {
      ppu_mem.sprite_mem[ppu_mem.sprite_addr] = Memory_Read(base_addr + n);
      ppu_mem.sprite_addr ++;
      ppu_mem.sprite_addr &= 0xFF;
    }
    CPU_6502_skip(513);
    ppu_mem.sprite_update = 1;
    */
  } else if (addr == 0x4017)
  {
    static const int NES_CLOCK_DIVIDER = 12;
    static const int NES_MASTER_CLOCK = (236250000 / 11);
    static const int NES_FIQ_PERIOD = (NES_MASTER_CLOCK / NES_CLOCK_DIVIDER / 60);
    //ppu_mem.fiq_state = val;
    //ppu_mem.fiq_cycles = NES_FIQ_PERIOD;
  } else
    APU_write(&s_apu, addr, val);
}

static uint8_t Read_PPU_Reg(uint16_t addr)
{
    return 0;
}

static void Write_PPU_Reg(uint16_t addr, uint8_t val)
{
}

uint8_t Memory_Read(uint16_t addr)
{
	return s_nes_memory_read_func[addr >> 13](addr);
}

void Memory_Write(uint16_t addr, uint8_t val)
{
    s_nes_memory_write_func[addr >> 13](addr, val);
}

void Memory_Init()
{
	//0000-1FFF	internal 2K RAM
	s_nes_memory_read_func[0] = Memory_ReadRam;
    s_nes_memory_write_func[0] = Memory_WriteRam;

	//2000-3FFF	PPU registers
	s_nes_memory_read_func[1] = Read_PPU_Reg;
    s_nes_memory_write_func[1] = Write_PPU_Reg;

	//4000-5FFF	APU registers
	s_nes_memory_read_func[2] = Read_APU_Reg;
    s_nes_memory_write_func[2] = Write_APU_Reg;

	//6000-7FFF	External SRAM
	s_nes_memory_read_func[3] = Memory_ReadSram;
    s_nes_memory_write_func[3] = Memory_WriteSram;

	//8000-9FFF ROM memory
	s_nes_memory_read_func[4] = Memory_ReadPgm0;
    s_nes_memory_write_func[4] = Memory_WritePgm0;

	//A000-BFFF ROM memory
	s_nes_memory_read_func[5] = Memory_ReadPgm1;
    s_nes_memory_write_func[5] = Memory_WritePgm1;

	//C000-DFFF ROM memory
	s_nes_memory_read_func[6] = Memory_ReadPgm2;
    s_nes_memory_write_func[6] = Memory_WritePgm2;

	//E000-FFFF ROM memory
	s_nes_memory_read_func[7] = Memory_ReadPgm3;
    s_nes_memory_write_func[7] = Memory_WritePgm3;
}
