#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <set>
#include <QDebug>
#include <iomanip>
#include <QFile>
#include "NoiseTable.h"


static uint8_t s_dither_BayerMap[64] =
{
    0,  48, 12, 60, 3,  51, 15, 63,
    32, 16, 44, 28, 35, 19, 47, 31,
    8,  56, 4,  52, 11, 59, 7,  55,
    40, 24, 36, 20, 43, 27, 39, 23,
    2,  50, 14, 62, 1,  49, 13, 61,
    34, 18, 46, 30, 33, 17, 45, 29,
    10, 58, 6,  54, 9,  57, 5,  53,
    42, 26, 38, 22, 41, 25, 37, 21
};

#if 0
static void PrintBuffer_cpp(std::stringstream &stream, std::vector<uint8_t>& buffer)
{
    int pos = 0;
    stream << "{" << std::endl;
    for (size_t n = 0; n < buffer.size(); ++n)
    {
        if (pos >= 16)
        {
            stream << std::endl;
            pos = 0;
        }
        if (pos == 0)
            stream << "    ";
        stream << "0x" << std::hex << std::setw(2) << std::setfill('0') << ((int)(buffer[n] & 0xFF));
        if (n+1 != buffer.size())
            stream << ",";
        else
            stream << std::endl;
        pos ++;
    }
    stream << "};" << std::endl;
}
#endif


static int ColorDelta(int r1, int g1, int b1, int r2, int g2, int b2)
{
#if 1
    int y1 =  ((int)(0.299   *1024)*r1 + (int)(0.587   *1024)*g1 + (int)(0.144   *1024)*b1) >> 10;
    int y2 =  ((int)(0.299   *1024)*r2 + (int)(0.587   *1024)*g2 + (int)(0.144   *1024)*b2) >> 10;
    int dy = y1-y2;
    int dr = r1-r2;
    int dg = g1-g2;
    int db = b1-b2;

#define WH 0.75
    int diff = dy*dy + (dr*dr*(int(0.299*WH*1024)) >> 10) + (dg*dg*(int(0.587*WH*1024)) >> 10) + (db*db*(int(0.587*WH*1024)) >> 10);
    return diff;
#else
    int dr = r1-r2;
    int dg = g1-g2;
    int db = b1-b2;
    return dr*dr + dg*dg + db*db;
#endif
}

static QImage ImageToYuv(const QImage &image, int brightness, int saturation, EDietherMethod diether_method, int noise_power)
{
    int saturation_mult = 1024 + saturation;
    int noise_mult = 1024 * noise_power / 128;
    QImage yuv_image(image.width(), image.height(), QImage::Format_ARGB32);

    const uint8_t* noise_table = s_rnd_noise;
    if (diether_method == DietherMethod_Bayer)
        noise_table = s_bayer_noise;
    else if (diether_method == DietherMethod_BlueNoise)
        noise_table = s_blue_noise;

    for (int y = 0; y < image.height(); ++y)
    {
        const uint8_t* line_ptr = image.scanLine(y);
        uint8_t* yuv_ptr = yuv_image.scanLine(y);
        const uint8_t* dither_line = noise_table + y * 256;
        for (int x = 0; x < image.width(); ++x)
        {
            int r = line_ptr[x*4+0];
            int g = line_ptr[x*4+1];
            int b = line_ptr[x*4+2];

#if 1
            //saturation
            r = (((r-128) * saturation_mult) >> 10) + 128;
            g = (((g-128) * saturation_mult) >> 10) + 128;
            b = (((b-128) * saturation_mult) >> 10) + 128;
#endif

#if 1
            //brightness
            r += brightness;
            g += brightness;
            b += brightness;
#endif

#if 1
            if (diether_method != DietherMethod_None)
            {
                int d = (((int)dither_line[x] - 128) * noise_mult) >> 10;
                r += d;
                g += d;
                b += d;
            }
#endif
            r = r > 255 ? 255 : (r < 0 ? 0 : r);
            g = g > 255 ? 255 : (g < 0 ? 0 : g);
            b = b > 255 ? 255 : (b < 0 ? 0 : b);
#if 1
            int y =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
            int u = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
            int v = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;
            yuv_ptr[x*4+0] = y > 255 ? 255 : (y < 0 ? 0 : y);
            yuv_ptr[x*4+1] = u > 255 ? 255 : (u < 0 ? 0 : u);
            yuv_ptr[x*4+2] = v > 255 ? 255 : (v < 0 ? 0 : v);
#else
            yuv_ptr[x*4+0] = r;
            yuv_ptr[x*4+1] = g;
            yuv_ptr[x*4+2] = b;
#endif
        }
    }
    return yuv_image;
}


void MainWindow::GenerateNesPaletteSet_EvgenyKz(const QImage &image, std::vector<int> &gen_palette,
                                                int max_colors, int brightness, int saturation, EDietherMethod diether_method, int noise_power)
{
    //Image to yuv
    QImage yuv_image = ImageToYuv(image, brightness, saturation, diether_method, noise_power);
    yuv_image.save("yuv_image.png");

    //Palette to uyv
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    uint8_t uyv_palette[64*4];
    for (int i = 0; i < 64; ++i)
    {
        int r = (int)((palette[i]>> 0) & 0xFF);
        int g = (int)((palette[i]>> 8) & 0xFF);
        int b = (int)((palette[i]>>16) & 0xFF);
#if 1
        uyv_palette[i*4+0] =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
        uyv_palette[i*4+1] = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
        uyv_palette[i*4+2] = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;
#else
        uyv_palette[i*4+0] = r;
        uyv_palette[i*4+1] = g;
        uyv_palette[i*4+2] = b;
#endif
    }

    uint8_t s_palette_skip[64] =
    {
        0,0,0,0,  0,0,0,0,  0,0,0,0,  0,1,1,0,
        0,0,0,0,  0,0,0,0,  0,0,0,0,  0,1,1,1,
        0,0,0,0,  0,0,0,0,  0,0,0,0,  0,1,1,1,
        1,0,0,0,  0,0,0,0,  0,0,0,0,  0,1,1,1
    };

    std::vector<int> color_hit(64, 0);
    std::vector<int> color_summ_r(64, 0);
    std::vector<int> color_summ_g(64, 0);
    std::vector<int> color_summ_b(64, 0);

    for (int y = 0; y < image.height(); ++y)
    {
        uint8_t* line_ptr = yuv_image.scanLine(y);
        for (int x = 0; x < image.width(); ++x)
        {
            int r = line_ptr[x*4+0];
            int g = line_ptr[x*4+1];
            int b = line_ptr[x*4+2];

            int best_n = 0;
            int best_delta = INT32_MAX;
            for (int n = 0; n < 64; ++n)
            {
                if (s_palette_skip[n])
                    continue;

                int delta = ColorDelta(r, g, b, uyv_palette[n*4+0], uyv_palette[n*4+1], uyv_palette[n*4+2]);
                if (delta < best_delta)
                {
                    best_delta = delta;
                    best_n = n;
                }
            }
            color_hit[best_n] ++;
            color_summ_r[best_n] += r;
            color_summ_g[best_n] += g;
            color_summ_b[best_n] += b;
        }
    }

    int max_color = 0;
    int max_count = 0;
    int color_count = 0;
    std::set<int> color_set;
    for (int i = 0; i < 64; ++i)
    {
        if (color_hit[i] != 0)
            ++color_count;
        else
            continue;
        //qDebug() << i << color_hit[i];
        color_set.insert(i);
        if (color_hit[i] > max_count)
        {
            max_count = color_hit[i];
            max_color = i;
        }
    }
    //qDebug() << "Base Color=" << max_color;
    //qDebug() << "Total Colors=" << color_count;

    //Remove minimal loss
    while (color_count > max_colors)
    {
        int64_t min_count = INT64_MAX;
        int min_i = 0;
        int min_j = 0;
        for (int i = 0; i < 64; ++i)
        {
            if (color_hit[i] == 0)
                continue;
            int avg_r = color_summ_r[i] / color_hit[i];
            int avg_g = color_summ_g[i] / color_hit[i];
            int avg_b = color_summ_b[i] / color_hit[i];
            int64_t min_loss = INT64_MAX;
            int min_loss_index = 0;
            for (int j = 0; j < 64; ++j)
            {
                if (i == j || color_hit[j] == 0)
                    continue;
                int delta = ColorDelta(avg_r, avg_g, avg_b, uyv_palette[j*4+0], uyv_palette[j*4+1], uyv_palette[j*4+2]);
                int64_t loss = (int64_t)delta * color_hit[i];
                if (loss < min_loss)
                {
                    min_loss = loss;
                    min_loss_index = j;
                }
            }
            if (min_loss < min_count)
            {
                min_count = min_loss;
                min_i = i;
                min_j = min_loss_index;
            }
        }
        //qDebug() << "Remove: " << min_i;
        color_hit[min_j] += color_hit[min_i];
        color_summ_r[min_j] += color_summ_r[min_i];
        color_summ_g[min_j] += color_summ_g[min_i];
        color_summ_b[min_j] += color_summ_b[min_i];
        color_set.erase(min_i);
        s_palette_skip[min_i] = 1;
        color_hit[min_i] = 0;
        color_count--;
    }

#if 1
    uint8_t palette_index[64];  //index 0..1
    memset(palette_index, 0x00, sizeof(palette_index));
    int palette_index_count = 0;
    uint8_t index2pal[16];
    memset(index2pal, 0x0F, sizeof(index2pal));
    //qDebug() << "Palette Index:";
    for (int i = 0; i < 64; ++i)
    {
        if (color_hit[i] == 0)
            continue;
        //qDebug() << palette_index_count << ":" << i;
        palette_index[i] = palette_index_count;
        index2pal[palette_index_count] = i;
        ++palette_index_count;
    }

    //Apply Palette to index map
    std::vector<uint8_t> index_map(image.width()*image.height());
    std::vector<uint8_t> color_map(image.width()*image.height());
    for (int y = 0; y < image.height(); ++y)
    {
        uint8_t* line_ptr = yuv_image.scanLine(y);
        //uint8_t* dest_ptr = m_image_indexed.scanLine(y);
        uint8_t* index_ptr = index_map.data() + y*image.width();
        uint8_t* color_ptr = color_map.data() + y*image.width();
        for (int x = 0; x < image.width(); ++x)
        {
            int r = line_ptr[x*4+0];
            int g = line_ptr[x*4+1];
            int b = line_ptr[x*4+2];

            int best_n = 0;
            int best_delta = INT32_MAX;
            for (int n = 0; n < 64; ++n)
            {
                if (s_palette_skip[n])
                    continue;
                int delta = ColorDelta(r, g, b, uyv_palette[n*4+0], uyv_palette[n*4+1], uyv_palette[n*4+2]);
                if (delta < best_delta)
                {
                    best_delta = delta;
                    best_n = n;
                }
            }
            //dest_ptr[x*4+0] = palette[best_n] >> 0;
            //dest_ptr[x*4+1] = palette[best_n] >> 8;
            //dest_ptr[x*4+2] = palette[best_n] >>16;
            //dest_ptr[x*4+3] = 255;
            color_ptr[x] = best_n;
            index_ptr[x] = palette_index[best_n];
        }
    }

    //Calculate relations
    int relation[256];
    memset(relation, 0, sizeof(relation));
    for (int y = 0; y < image.height(); ++y)
    {
        uint8_t* index_ptr = index_map.data() + y*image.width();
        for (int x = 0; x < image.width(); x += 8)
        {
            for (int i = 0; i < 8; ++i)
            {
                int a = index_ptr[x+i];
                for (int j = i+1; j < 8; ++j)
                {
                    int b = index_ptr[x+j];
                    //if (a == b)
                        //continue;
                    if (a < b)
                        relation[b*16+a]++;
                    else
                        relation[a*16+b]++;
                }
            }
        }
    }

    //Make palette table
    uint8_t ppu_palette[16];
    uint8_t pal_in_use[64];
    memset(pal_in_use, 0, sizeof(pal_in_use));
    memset(ppu_palette, 0x0f, sizeof(ppu_palette));
    int base_color_index = 0;
    int max_summ = 0;
    //qDebug() << "Ralation summ:";
    for (int a = 0; a < 16; ++a)
    {
        int summ = 0;
        for (int b = 0; b < 16; ++b)
        {
            if (a == b)
                continue;
            if (b > a)
                summ += relation[b*16 + a];
            else
                summ += relation[a*16 + b];
        }
        //qDebug() << index2pal[a] << ":" << summ;
        if (summ > max_summ)
        {
            max_summ = summ;
            base_color_index = a;
        }
    }

    int base_color = index2pal[base_color_index];
    //qDebug() << "BaseColor=" << base_color;
    for (int b = 0; b < 16; ++b)
    {
        relation[b*16 + base_color_index] = 0;
        relation[base_color_index*16 + b] = 0;
    }

    for (int p = 0; p < 4; ++p)
    {
        ppu_palette[p*4+0] = base_color;
        //Found 2 colors
        int max_index = 0;
        int max_count = -1;
        for (int i = 0; i < 256; ++i)
        {
            int cnt = relation[i];
            int a = i / 16;
            int b = i % 16;
            if (a == b)
                continue;
            if (pal_in_use[a] || pal_in_use[b])
                continue;
            cnt += relation[a*16+a];
            cnt += relation[b*16+b];
            if (cnt > max_count)
            {
                max_count = cnt;
                max_index = i;
            }
        }
        int a = max_index / 16;
        int b = max_index % 16;
        ppu_palette[p*4+1] = index2pal[a];
        ppu_palette[p*4+2] = index2pal[b];

        //Found third color
        int max_count3 = -1;
        int max_index3 = 0;
        for (int i = 0; i < 16; ++i)
        {
            if (a == i || b == i)
                continue;
            if (pal_in_use[i])
                continue;
            int cnt;
            if (i < a)
                cnt = relation[a*16+i];
            else
                cnt = relation[i*16+a];

            if (i < b)
                cnt += relation[b*16+i];
            else
                cnt += relation[i*16+b];

            cnt += relation[i*16+i];

            if (cnt > max_count3)
            {
                max_count3 = cnt;
                max_index3 = i;
            }
        }
        ppu_palette[p*4+3] = index2pal[max_index3];
#if 0
        pal_in_use[a] = 1;
        pal_in_use[b] = 1;
        pal_in_use[max_index3] = 1;
#endif
        relation[max_index] = 0;
        relation[a*16+a] = 0;
        relation[b*16+b] = 0;
        relation[max_index3*16+max_index3] = 0;
        relation[a*16+max_index3] = 0;
        relation[max_index3*16+a] = 0;
        relation[b*16+max_index3] = 0;
        relation[max_index3*16+b] = 0;
#if 1
        for (int i = 0; i < 16; ++i)
        {
            relation[a*16+i] /= 4;
            relation[i*16+a] /= 4;

            relation[b*16+i] /= 4;
            relation[i*16+b] /= 4;

            relation[max_index3*16+i] /= 4;
            relation[i*16+max_index3] /= 4;
        }
#endif
    }

    gen_palette.resize(16, 0x0f);
    for (int i = 0; i < 16; ++i)
        gen_palette[i] = ppu_palette[i];

    //qDebug() << "PpuPalette Build:";
    //for (int i = 0; i < 16; ++i)
        //qDebug() << i << ":" << (int)ppu_palette[i];
#endif
}


QImage MainWindow::EvgenyKz_IndexedImage(const QImage &image, std::vector<int> &gen_palette, int brightness, int saturation, EDietherMethod diether_method, int noise_power)
{
    //Image to yuv
    QImage yuv_image = ImageToYuv(image, brightness, saturation, diether_method, noise_power);

    //Palette to uyv
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    uint8_t uyv_palette[64*4];
    for (int i = 0; i < 64; ++i)
    {
        int r = (int)((palette[i]>> 0) & 0xFF);
        int g = (int)((palette[i]>> 8) & 0xFF);
        int b = (int)((palette[i]>>16) & 0xFF);
#if 1
        uyv_palette[i*4+0] =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
        uyv_palette[i*4+1] = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
        uyv_palette[i*4+2] = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;
#else
        uyv_palette[i*4+0] = r;
        uyv_palette[i*4+1] = g;
        uyv_palette[i*4+2] = b;
#endif
    }

    //Apply Palette
    QImage indexed_image(image.width(), image.height(), QImage::Format_ARGB32);
    for (int y = 0; y < yuv_image.height(); ++y)
    {
        uint8_t* line_ptr = yuv_image.scanLine(y);
        uint8_t* dest_ptr = indexed_image.scanLine(y);
        for (int x = 0; x < yuv_image.width(); x += 8)
        {
            int best_pal = 0;
            int64_t best_error_summ = INT32_MAX;
            for (int pal = 0; pal < 4; ++pal)
            {
                int64_t error_summ = 0;
                for (int i = 0; i < 8; ++i)
                {
                    int xi = x+i;
                    int r = line_ptr[xi*4+0];
                    int g = line_ptr[xi*4+1];
                    int b = line_ptr[xi*4+2];

                    //int best_n = 0;
                    int best_delta = INT32_MAX;
                    for (int c = 0; c < 4; ++c)
                    {
                        int delta = ColorDelta(r, g, b, uyv_palette[gen_palette[(pal*4)|c]*4+0], uyv_palette[gen_palette[(pal*4)|c]*4+1], uyv_palette[gen_palette[(pal*4)|c]*4+2]);
                        if (delta < best_delta)
                        {
                            best_delta = delta;
                            //best_n = c;
                        }
                    }

                    error_summ += best_delta;
                }
                if (error_summ < best_error_summ)
                {
                    best_error_summ = error_summ;
                    best_pal = pal;
                }
            }

            for (int i = 0; i < 8; ++i)
            {
                int xi = x+i;
                int r = line_ptr[xi*4+0];
                int g = line_ptr[xi*4+1];
                int b = line_ptr[xi*4+2];

                int best_c = 0;
                int best_delta = INT32_MAX;
                for (int c = 0; c < 4; ++c)
                {
                    int delta = ColorDelta(r, g, b, uyv_palette[gen_palette[(best_pal*4)|c]*4+0], uyv_palette[gen_palette[(best_pal*4)|c]*4+1], uyv_palette[gen_palette[(best_pal*4)|c]*4+2]);
                    if (delta < best_delta)
                    {
                        best_delta = delta;
                        best_c = c;
                    }
                }

#if 1
                int cl = gen_palette[(best_pal*4)|best_c];
                dest_ptr[xi*4+0] = palette[cl] >> 0;
                dest_ptr[xi*4+1] = palette[cl] >> 8;
                dest_ptr[xi*4+2] = palette[cl] >>16;
#else
                dest_ptr[xi*4+0] = r;
                dest_ptr[xi*4+1] = g;
                dest_ptr[xi*4+2] = b;
#endif
                dest_ptr[xi*4+3] = 255;
            }
        }
    }

    return indexed_image;
}

QImage MainWindow::EvgenyKz_IndexedImage_16x16(const QImage &image, std::vector<int> &gen_palette, int brightness, int saturation, EDietherMethod diether_method, int noise_power)
{
    //Image to yuv
    QImage yuv_image = ImageToYuv(image, brightness, saturation, diether_method, noise_power);

    //Palette to uyv
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    uint8_t uyv_palette[64*4];
    for (int i = 0; i < 64; ++i)
    {
        int r = (int)((palette[i]>> 0) & 0xFF);
        int g = (int)((palette[i]>> 8) & 0xFF);
        int b = (int)((palette[i]>>16) & 0xFF);
#if 1
        uyv_palette[i*4+0] =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
        uyv_palette[i*4+1] = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
        uyv_palette[i*4+2] = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;
#else
        uyv_palette[i*4+0] = r;
        uyv_palette[i*4+1] = g;
        uyv_palette[i*4+2] = b;
#endif
    }

    //Apply Palette
    QImage indexed_image(image.width(), image.height(), QImage::Format_ARGB32);
    for (int y = 0; y < yuv_image.height(); y += 16)
    {
        for (int x = 0; x < yuv_image.width(); x += 16)
        {
            int best_pal = 0;
            int64_t best_error_summ = INT32_MAX;
            for (int pal = 0; pal < 4; ++pal)
            {
                int64_t error_summ = 0;
                for (int yi = 0; yi < 16; ++yi)
                {
                    uint8_t* line_ptr = yuv_image.scanLine(y + yi) + x*4;
                    //uint8_t* dest_ptr = indexed_image.scanLine(y + yi) + x*4;
                    for (int xi = 0; xi < 16; ++xi)
                    {
                        int r = line_ptr[xi*4+0];
                        int g = line_ptr[xi*4+1];
                        int b = line_ptr[xi*4+2];

                        //int best_n = 0;
                        int best_delta = INT32_MAX;
                        for (int c = 0; c < 4; ++c)
                        {
                            int delta = ColorDelta(r, g, b, uyv_palette[gen_palette[(pal*4)|c]*4+0], uyv_palette[gen_palette[(pal*4)|c]*4+1], uyv_palette[gen_palette[(pal*4)|c]*4+2]);
                            if (delta < best_delta)
                            {
                                best_delta = delta;
                                //best_n = c;
                            }
                        }
                        error_summ += best_delta;
                    }
                }
                if (error_summ < best_error_summ)
                {
                    best_error_summ = error_summ;
                    best_pal = pal;
                }
            }

            for (int yi = 0; yi < 16; ++yi)
            {
                uint8_t* line_ptr = yuv_image.scanLine(y + yi) + x*4;
                uint8_t* dest_ptr = indexed_image.scanLine(y + yi) + x*4;
                for (int xi = 0; xi < 16; ++xi)
                {
                    int r = line_ptr[xi*4+0];
                    int g = line_ptr[xi*4+1];
                    int b = line_ptr[xi*4+2];

                    int best_c = 0;
                    int best_delta = INT32_MAX;
                    for (int c = 0; c < 4; ++c)
                    {
                        int delta = ColorDelta(r, g, b, uyv_palette[gen_palette[(best_pal*4)|c]*4+0], uyv_palette[gen_palette[(best_pal*4)|c]*4+1], uyv_palette[gen_palette[(best_pal*4)|c]*4+2]);
                        if (delta < best_delta)
                        {
                            best_delta = delta;
                            best_c = c;
                        }
                    }
                    int cl = gen_palette[(best_pal*4)|best_c];
                    dest_ptr[xi*4+0] = palette[cl] >> 0;
                    dest_ptr[xi*4+1] = palette[cl] >> 8;
                    dest_ptr[xi*4+2] = palette[cl] >>16;
                    dest_ptr[xi*4+3] = 255;
                }
            }
        }
    }

    return indexed_image;
}


