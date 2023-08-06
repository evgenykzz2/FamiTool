#pragma once
#include <stdint.h>

enum EPalette
{
    Palette_2C02 = 0,
    Palette_2C03,
    Palette_2C04,
    Palette_fceux
};

const uint32_t* GetPalette(EPalette);


struct Palette
{
    uint8_t c[4];

    Palette();
};

uint32_t ColorAvg(uint32_t a, uint32_t b);

struct BlinkPalette
{
    uint32_t color[4*16];
    uint8_t enable[4*16];
    BlinkPalette();
};
