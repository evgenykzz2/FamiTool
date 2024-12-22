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

void BuildBlock(const Block &block, const uint32_t* palette, const std::vector<int>& palette_yuv,
                const uint8_t* palette_set, const QImage &tileset_image,
                QImage &dest_tile, std::vector<uint8_t> &dest_index)
{
    uint32_t color[4];
    int color_yuv[16];

    for (int i = 0; i < 4; ++i)
    {
        int index =palette_set[block.palette * 4 + i];
        color[i] = palette[index] | 0xFF000000;
        color_yuv[i*4+0] = palette_yuv[index*4+0];
        color_yuv[i*4+1] = palette_yuv[index*4+1];
        color_yuv[i*4+2] = palette_yuv[index*4+2];
    }

    dest_tile = QImage(16, 16, QImage::Format_ARGB32);
    dest_tile.fill(0xFF000000);
    dest_index.resize(16*16, 0);
    for (int y = 0; y < 16; ++y)
    {
        for (int x = 0; x < 16; ++x)
        {
            uint32_t tileset_pixel = tileset_image.pixel(block.tile_x+x, block.tile_y+y);
            int r = (int)((tileset_pixel >> 0 ) & 0xFF);
            int g = (int)((tileset_pixel >> 8 ) & 0xFF);
            int b = (int)((tileset_pixel >>16 ) & 0xFF);
            int cy =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
            int cu = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
            int cv = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;

            int best_diff = std::numeric_limits<int>::max();
            int best_c = 0;
            for (int c = 0; c < 4; ++c)
            {
                int dr = cy - color_yuv[c*4+0];
                int dg = cu - color_yuv[c*4+1];
                int db = cv - color_yuv[c*4+2];
                int diff = dr*dr + dg*dg + db*db;
                if (diff < best_diff)
                {
                    best_diff = diff;
                    best_c = c;
                }
            }
            dest_index[y*16+x] = best_c;
            dest_tile.setPixel(x, y, color[best_c]);
        }
    }

}
