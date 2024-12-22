#include "Palette.h"
#include <memory.h>

#define RGB(r,g,b) ( 0xFF000000 | (r << 16) | (g << 8) | (b))

static const uint32_t NesPalette_2C02[64] =
{
   RGB( 84,  84,  84), RGB(  0,  30, 116), RGB(  8,  16, 144), RGB( 48,   0, 136), RGB( 68,   0, 100), RGB( 92,   0,  48), RGB( 84,   4,   0), RGB( 60,  24,   0), RGB( 32,  42,   0), RGB(  8,  58,   0),  RGB(  0,  64,   0),  RGB(  0,  60,   0),  RGB(  0,  50,  60),  RGB(  0,   0,   0), RGB(  0,   0,   0), RGB(  0,   0,   0),
   RGB(152, 150, 152), RGB(  8,  76, 196), RGB( 48,  50, 236), RGB( 92,  30, 228), RGB(136,  20, 176), RGB(160,  20, 100), RGB(152,  34,  32), RGB(120,  60,   0), RGB( 84,  90,   0), RGB( 40, 114,   0),  RGB(  8, 124,   0),  RGB(  0, 118,  40),  RGB(  0, 102, 120),  RGB(  0,   0,   0), RGB(  0,   0,   0), RGB(  0,   0,   0),
   RGB(236, 238, 236), RGB( 76, 154, 236), RGB(120, 124, 236), RGB(176,  98, 236), RGB(228,  84, 236), RGB(236,  88, 180), RGB(236, 106, 100), RGB(212, 136,  32), RGB(160, 170,   0), RGB(116, 196,   0),  RGB( 76, 208,  32),  RGB( 56, 204, 108),  RGB( 56, 180, 204),  RGB( 60,  60,  60), RGB(  0,   0,   0), RGB(  0,   0,   0),
   RGB(236, 238, 236), RGB(168, 204, 236), RGB(188, 188, 236), RGB(212, 178, 236), RGB(236, 174, 236), RGB(236, 174, 212), RGB(236, 180, 176), RGB(228, 196, 144), RGB(204, 210, 120), RGB(180, 222, 120),  RGB(168, 226, 144),  RGB(152, 226, 180),  RGB(160, 214, 228),  RGB(160, 162, 160), RGB(  0,   0,   0), RGB(  0,   0,   0)
};

static const uint32_t NesPalette_2C03[64] =
{
    RGB(109,109,109),RGB(  0, 36,146),RGB(  0,  0,219),RGB(109, 73,219),RGB(146,  0,109),RGB(182,  0,109),RGB(182, 36,  0),RGB(146, 73,  0),RGB(109, 73,  0),RGB( 36, 73,  0),RGB(  0,109, 36),RGB(  0,146,  0),RGB(  0, 73, 73),RGB(  0,  0,  0),RGB(  0,  0,  0),RGB(  0,  0,  0),
    RGB(182,182,182),RGB(  0,109,219),RGB(  0, 73,255),RGB(146,  0,255),RGB(182,  0,255),RGB(255,  0,146),RGB(255,  0,  0),RGB(219,109,  0),RGB(146,109,  0),RGB( 36,146,  0),RGB(  0,146,  0),RGB(  0,182,109),RGB(  0,146,146),RGB(  0,  0,  0),RGB(  0,  0,  0),RGB(  0,  0,  0),
    RGB(255,255,255),RGB(109,182,255),RGB(146,146,255),RGB(219,109,255),RGB(255,  0,255),RGB(255,109,255),RGB(255,146,  0),RGB(255,182,  0),RGB(219,219,  0),RGB(109,219,  0),RGB(  0,255,  0),RGB( 73,255,219),RGB(  0,255,255),RGB(  0,  0,  0),RGB(  0,  0,  0),RGB(  0,  0,  0),
    RGB(255,255,255),RGB(182,219,255),RGB(219,182,255),RGB(255,182,255),RGB(255,146,255),RGB(255,182,182),RGB(255,219,146),RGB(255,255, 73),RGB(255,255,109),RGB(182,255, 73),RGB(146,255,109),RGB( 73,255,219),RGB(146,219,255),RGB(  0,  0,  0),RGB(  0,  0,  0),RGB(  0,  0,  0)
};

static const uint32_t NesPalette_2C04[64] =
{
    RGB(109,109,109),RGB(  0, 36,146),RGB(  0,  0,219),RGB(109, 73,219),RGB(146,  0,109),RGB(182,  0,109),RGB(182, 36,  0),RGB(146, 73,  0),RGB(109, 73,  0),RGB( 36, 73,  0),RGB(  0,109, 36),RGB(  0,146,  0),RGB(  0, 73, 73),RGB( 36, 36, 36),RGB(  0,  0,109),RGB(  0, 73,  0),
    RGB(182,182,182),RGB(  0,109,219),RGB(  0, 73,255),RGB(146,  0,255),RGB(182,  0,255),RGB(255,  0,146),RGB(255,  0,  0),RGB(219,109,  0),RGB(146,109,  0),RGB( 36,146,  0),RGB(  0,146,  0),RGB(  0,182,109),RGB(  0,146,146),RGB( 73, 73, 73),RGB( 73,  0,  0),RGB(109, 36,  0),
    RGB(255,255,255),RGB(109,182,255),RGB(146,146,255),RGB(219,109,255),RGB(255,  0,255),RGB(255,109,255),RGB(255,146,  0),RGB(255,182,  0),RGB(219,219,  0),RGB(109,219,  0),RGB(  0,255,  0),RGB( 73,255,219),RGB(  0,255,255),RGB(146,146,146),RGB(  0,  0,  0),RGB(  0,  0,  0),
    RGB(255,255,255),RGB(182,219,255),RGB(219,182,255),RGB(255,182,255),RGB(255,146,255),RGB(255,182,182),RGB(255,219,146),RGB(255,255,  0),RGB(255,255,109),RGB(182,255, 73),RGB(146,255,109),RGB( 73,255,219),RGB(146,219,255),RGB(219,219,219),RGB(219,182,109),RGB(255,219,  0)
};


static const uint32_t NesPalette_fceux[64] =
{
    RGB( 0x1D<<2, 0x1D<<2, 0x1D<<2 ), /* Value 0 */
    RGB( 0x09<<2, 0x06<<2, 0x23<<2 ), /* Value 1 */
    RGB( 0x00<<2, 0x00<<2, 0x2A<<2 ), /* Value 2 */
    RGB( 0x11<<2, 0x00<<2, 0x27<<2 ), /* Value 3 */
    RGB( 0x23<<2, 0x00<<2, 0x1D<<2 ), /* Value 4 */
    RGB( 0x2A<<2, 0x00<<2, 0x04<<2 ), /* Value 5 */
    RGB( 0x29<<2, 0x00<<2, 0x00<<2 ), /* Value 6 */
    RGB( 0x1F<<2, 0x02<<2, 0x00<<2 ), /* Value 7 */
    RGB( 0x10<<2, 0x0B<<2, 0x00<<2 ), /* Value 8 */
    RGB( 0x00<<2, 0x11<<2, 0x00<<2 ), /* Value 9 */
    RGB( 0x00<<2, 0x14<<2, 0x00<<2 ), /* Value 10 */
    RGB( 0x00<<2, 0x0F<<2, 0x05<<2 ), /* Value 11 */
    RGB( 0x06<<2, 0x0F<<2, 0x17<<2 ), /* Value 12 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 13 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 14 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 15 */
    RGB( 0x2F<<2, 0x2F<<2, 0x2F<<2 ), /* Value 16 */
    RGB( 0x00<<2, 0x1C<<2, 0x3B<<2 ), /* Value 17 */
    RGB( 0x08<<2, 0x0E<<2, 0x3B<<2 ), /* Value 18 */
    RGB( 0x20<<2, 0x00<<2, 0x3C<<2 ), /* Value 19 */
    RGB( 0x2F<<2, 0x00<<2, 0x2F<<2 ), /* Value 20 */
    RGB( 0x39<<2, 0x00<<2, 0x16<<2 ), /* Value 21 */
    RGB( 0x36<<2, 0x0A<<2, 0x00<<2 ), /* Value 22 */
    RGB( 0x32<<2, 0x13<<2, 0x03<<2 ), /* Value 23 */
    RGB( 0x22<<2, 0x1C<<2, 0x00<<2 ), /* Value 24 */
    RGB( 0x00<<2, 0x25<<2, 0x00<<2 ), /* Value 25 */
    RGB( 0x00<<2, 0x2A<<2, 0x00<<2 ), /* Value 26 */
    RGB( 0x00<<2, 0x24<<2, 0x0E<<2 ), /* Value 27 */
    RGB( 0x00<<2, 0x20<<2, 0x22<<2 ), /* Value 28 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 29 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 30 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 31 */
    RGB( 0x3F<<2, 0x3F<<2, 0x3F<<2 ), /* Value 32 */
    RGB( 0x0F<<2, 0x2F<<2, 0x3F<<2 ), /* Value 33 */
    RGB( 0x17<<2, 0x25<<2, 0x3F<<2 ), /* Value 34 */
    RGB( 0x33<<2, 0x22<<2, 0x3F<<2 ), /* Value 35 */
    RGB( 0x3D<<2, 0x1E<<2, 0x3F<<2 ), /* Value 36 */
    RGB( 0x3F<<2, 0x1D<<2, 0x2D<<2 ), /* Value 37 */
    RGB( 0x3F<<2, 0x1D<<2, 0x18<<2 ), /* Value 38 */
    RGB( 0x3F<<2, 0x26<<2, 0x0E<<2 ), /* Value 39 */
    RGB( 0x3C<<2, 0x2F<<2, 0x0F<<2 ), /* Value 40 */
    RGB( 0x20<<2, 0x34<<2, 0x04<<2 ), /* Value 41 */
    RGB( 0x13<<2, 0x37<<2, 0x12<<2 ), /* Value 42 */
    RGB( 0x16<<2, 0x3E<<2, 0x26<<2 ), /* Value 43 */
    RGB( 0x00<<2, 0x3A<<2, 0x36<<2 ), /* Value 44 */
    RGB( 0x1E<<2, 0x1E<<2, 0x1E<<2 ), /* Value 45 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 46 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 47 */
    RGB( 0x3F<<2, 0x3F<<2, 0x3F<<2 ), /* Value 48 */
    RGB( 0x2A<<2, 0x39<<2, 0x3F<<2 ), /* Value 49 */
    RGB( 0x31<<2, 0x35<<2, 0x3F<<2 ), /* Value 50 */
    RGB( 0x35<<2, 0x32<<2, 0x3F<<2 ), /* Value 51 */
    RGB( 0x3F<<2, 0x31<<2, 0x3F<<2 ), /* Value 52 */
    RGB( 0x3F<<2, 0x31<<2, 0x36<<2 ), /* Value 53 */
    RGB( 0x3F<<2, 0x2F<<2, 0x2C<<2 ), /* Value 54 */
    RGB( 0x3F<<2, 0x36<<2, 0x2A<<2 ), /* Value 55 */
    RGB( 0x3F<<2, 0x39<<2, 0x28<<2 ), /* Value 56 */
    RGB( 0x38<<2, 0x3F<<2, 0x28<<2 ), /* Value 57 */
    RGB( 0x2A<<2, 0x3C<<2, 0x2F<<2 ), /* Value 58 */
    RGB( 0x2C<<2, 0x3F<<2, 0x33<<2 ), /* Value 59 */
    RGB( 0x27<<2, 0x3F<<2, 0x3C<<2 ), /* Value 60 */
    RGB( 0x31<<2, 0x31<<2, 0x31<<2 ), /* Value 61 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 ), /* Value 62 */
    RGB( 0x00<<2, 0x00<<2, 0x00<<2 )  /* Value 63 */
};


const uint32_t* GetPalette(EPalette palette)
{
    switch (palette)
    {
    case Palette_2C02:
        return NesPalette_2C02;
    case Palette_2C03:
        return NesPalette_2C03;
    case Palette_2C04:
        return NesPalette_2C04;
    case Palette_fceux:
        return NesPalette_fceux;
    default:
        return NesPalette_2C02;
    }
}

std::vector<int> GetPaletteYuv(EPalette palette)
{
    const uint32_t* pal = GetPalette(palette);
    std::vector<int> yuv(64*4, 0);
    for (int i = 0; i < 64; ++i)
    {
        int r = (int)((pal[i]      ) & 0xFF);
        int g = (int)((pal[i] >>  8) & 0xFF);
        int b = (int)((pal[i] >> 16) & 0xFF);

        yuv[i*4+0] =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
        yuv[i*4+1] = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
        yuv[i*4+2] = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;
    }
    return yuv;
}

uint32_t ColorAvg(uint32_t a, uint32_t b)
{
    uint8_t ra = (a >>  0) & 0xFF;
    uint8_t ga = (a >>  8) & 0xFF;
    uint8_t ba = (a >> 16) & 0xFF;

    uint8_t rb = (b >>  0) & 0xFF;
    uint8_t gb = (b >>  8) & 0xFF;
    uint8_t bb = (b >> 16) & 0xFF;

    return ((ra+rb)/2) | ((ga+gb)/2 << 8) | ((ba+bb)/2 << 16) | (0xFF000000);
}
