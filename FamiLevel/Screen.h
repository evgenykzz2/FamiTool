#pragma once
#include <stdint.h>
#include <vector>

struct Screen
{
    int palette_id;
    int chr0_id;
    int chr1_id;

    uint8_t block[256];
    uint8_t flags;
};
