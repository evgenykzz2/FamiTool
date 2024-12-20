#pragma once
#include <stdint.h>
#include <QString>

enum EPalette
{
    Palette_2C02 = 0,
    Palette_2C03,
    Palette_2C04,
    Palette_fceux
};

struct Palette
{
    QString name;
    uint8_t color[16];
};

const uint32_t* GetPalette(EPalette);

uint32_t ColorAvg(uint32_t a, uint32_t b);
