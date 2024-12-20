#include "Sprite.h"

QImage ChrRender(const void* chr, int tile_width, int tile_height, int tile_zoom, const uint32_t *color)
{
    QImage image(tile_width*8, tile_height*8, QImage::Format_ARGB32);
    for (int ty = 0; ty < tile_height; ++ty)
    {
        for (int tx = 0; tx < tile_width; ++tx)
        {
            const uint8_t* tile_ptr = (const uint8_t*)chr + (tx+ty*tile_width)*16;
            for (int y = 0; y < 8; ++y)
            {
                uint8_t* image_ptr = image.scanLine((ty*8+y)*1) + (tx*8)*4*1;
                for (int x = 0; x < 8; ++x)
                {
                    uint8_t c = 0;
                    if (tile_ptr[y+0] & (0x80 >> x))
                        c |= 1;
                    if (tile_ptr[y+8] & (0x80 >> x))
                        c |= 2;
                    ((uint32_t*)image_ptr)[x] = color[c];
                }
            }
        }
    }
    return image.scaled(tile_width*8*tile_zoom, tile_height*8*tile_zoom);
}
