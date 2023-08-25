#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QIcon>
#include <QMouseEvent>
#include <iostream>

static std::map<int, QImage> s_block_image_map;

void MainWindow::ScreenWnd_Init()
{
    QList<int> sizes;
    sizes << 250 << 40;
    ui->splitter_screen->setSizes(sizes);
    ui->label_screen_render->installEventFilter(this);
}

void MainWindow::ScreenWnd_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->label_screen_render &&
            (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove))
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x()/1 + ui->slider_screen_h->value();
            int y = mouse_event->y()/1 + ui->slider_screen_v->value();

            State state = m_state.back();

            int screen_y = y / 256;
            int screen_x = x / 256;
            auto itt = state.m_world.find(std::make_pair(screen_x, screen_y));
            if (itt != state.m_world.end())
            {
                int block_x = (x & 255) / 16;
                int block_y = (y & 255) / 16;
                int index = block_x | (block_y*16);
                if (itt->second.block[index] != ui->listWidget_screen->currentRow())
                {
                    itt->second.block[index] = ui->listWidget_screen->currentRow();
                    StatePush(state);
                    ScreenWnd_RedrawScreen();
                }
            }
        }
    }
}

void MainWindow::ScreenWnd_FullRedraw()
{
    s_block_image_map.clear();
    State state = m_state.back();

    ui->slider_screen_h->setMinimum(0);
    ui->slider_screen_h->setMaximum(state.m_width_screens*256 - 256);

    ui->slider_screen_v->setMinimum(0);
    ui->slider_screen_v->setMaximum(state.m_height_screens*256 - 240);

    //Build images
    {
        auto chr0_itt = state.m_chr_map.begin();
        for (int i = 0; i < state.m_block_chr0; ++i)
        {
            if (chr0_itt == state.m_chr_map.end())
                break;
            ++chr0_itt;
        }

        auto chr1_itt = state.m_chr_map.begin();
        for (int i = 0; i < state.m_block_chr1; ++i)
        {
            if (chr0_itt == state.m_chr_map.end())
                break;
            ++chr1_itt;
        }

        auto palette_itt = state.m_palette_map.begin();
        if (palette_itt != state.m_palette_map.end() &&
            chr0_itt != state.m_chr_map.end() &&
            chr1_itt != state.m_chr_map.end() )
        {
            uint32_t color[4];
            const uint32_t* palette = GetPalette(state.m_palette);
            for (auto block_itt = state.m_block_map.begin(); block_itt != state.m_block_map.end(); ++block_itt)
            {
                for (int i = 0; i < 4; ++i)
                    color[i] = palette[palette_itt->second.color[block_itt->second.palette * 4 + i]] | 0xFF000000;
                QImage image(16, 16, QImage::Format_ARGB32);
                image.fill(0xFF000000);
                for (int i = 0; i < 4; ++i)
                {
                    int tile_id = block_itt->second.tile_id[i];
                    const uint8_t* chr = tile_id < 128 ? chr0_itt->second.chr_data->data() : chr1_itt->second.chr_data->data();
                    tile_id &= 0x7F;
                    int xp = (i & 1) * 8;
                    int yp = ((i & 2) >> 1) * 8;
                    const uint8_t* tile_ptr = (const uint8_t*)chr + tile_id*16;
                    for (int y = 0; y < 8; ++y)
                    {
                        uint8_t* image_ptr = image.scanLine(yp + y) + (xp)*4;
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
                s_block_image_map.insert(std::make_pair(block_itt->first, image));
            }
        }
    }

    ui->listWidget_screen->blockSignals(true);
    ui->listWidget_screen->clear();
    ui->listWidget_screen->setIconSize(QSize(32, 32));
    for (auto block_itt = state.m_block_map.begin(); block_itt != state.m_block_map.end(); ++block_itt)
    {
        auto image_itt = s_block_image_map.find(block_itt->first);
        if (image_itt == s_block_image_map.end())
            continue;
        ui->listWidget_screen->addItem( new QListWidgetItem(QIcon(QPixmap::fromImage(image_itt->second.scaled(32, 32))), block_itt->second.name) );

    }
    ui->listWidget_screen->blockSignals(false);

    ScreenWnd_RedrawScreen();
}


void MainWindow::ScreenWnd_RedrawScreen()
{
    QImage image(256, 240, QImage::Format_ARGB32);
    image.fill(0xFF000000);
    State state = m_state.back();

    auto chr0_itt = state.m_chr_map.begin();
    for (int i = 0; i < state.m_block_chr0; ++i)
    {
        if (chr0_itt == state.m_chr_map.end())
            break;
        ++chr0_itt;
    }
    int base_chr0 = chr0_itt == state.m_chr_map.end() ? 0 : chr0_itt->first;

    auto chr1_itt = state.m_chr_map.begin();
    for (int i = 0; i < state.m_block_chr1; ++i)
    {
        if (chr0_itt == state.m_chr_map.end())
            break;
        ++chr1_itt;
    }
    int base_chr1 = chr1_itt == state.m_chr_map.end() ? 1 : chr1_itt->first;

    auto palette_itt = state.m_palette_map.begin();
    const uint32_t* palette = GetPalette(state.m_palette);

    std::pair<int, int> screen_key = std::make_pair(0, 0);
    auto screen_itt = state.m_world.find(screen_key);
    if (screen_itt == state.m_world.end())
    {
        Screen screen;
        memset(screen.block, 0, sizeof(screen.block));
        if (!state.m_palette_map.empty())
            screen.palette_id = state.m_palette_map.begin()->first;
        else
            screen.palette_id = 0;
        screen.chr0_id = base_chr0;
        screen.chr1_id = base_chr1;
        state.m_world.insert(std::make_pair(screen_key, screen));
        screen_itt = state.m_world.find(screen_key);
        StatePush(state);
        chr0_itt = state.m_chr_map.find(screen_itt->second.chr0_id);
        chr1_itt = state.m_chr_map.find(screen_itt->second.chr1_id);
        palette_itt = state.m_palette_map.find(screen_itt->second.palette_id);
    }

    for (int y = 0; y < 240; ++y)
    {
        int world_y = y + ui->slider_screen_v->value();
        int screen_y = world_y / 256;
        if (screen_y < 0 || screen_y >= state.m_height_screens)
            continue;
        int inscreen_y = world_y & 0xFF;
        int block_y = inscreen_y / 16;
        int inblock_y = inscreen_y & 15;
        int pixel_y = inblock_y & 7;
        uint32_t* image_line = (uint32_t*)image.scanLine(y);

        for (int x = 0; x < 256; ++x)
        {
            int world_x = x + ui->slider_screen_h->value();
            int screen_x = world_x / 256;
            if (screen_x < 0 || screen_x >= state.m_width_screens)
                continue;
            if (screen_x != screen_key.first || screen_y != screen_key.second)
            {
                screen_key = std::make_pair(screen_x, screen_y);
                screen_itt = state.m_world.find(screen_key);
                if (screen_itt == state.m_world.end())
                {
                    Screen screen;
                    memset(screen.block, 0, sizeof(screen.block));
                    if (!state.m_palette_map.empty())
                        screen.palette_id = state.m_palette_map.begin()->first;
                    else
                        screen.palette_id = 0;
                    screen.chr0_id = base_chr0;
                    screen.chr1_id = base_chr1;
                    state.m_world.insert(std::make_pair(screen_key, screen));
                    screen_itt = state.m_world.find(screen_key);
                    StatePush(state);
                }
                chr0_itt = state.m_chr_map.find(screen_itt->second.chr0_id);
                chr1_itt = state.m_chr_map.find(screen_itt->second.chr1_id);
                palette_itt = state.m_palette_map.find(screen_itt->second.palette_id);
            }
            int inscreen_x = world_x & 0xFF;
            int block_x = inscreen_x / 16;
            int inblock_x = inscreen_x & 15;
            int pixel_x = inblock_x & 7;

            int block_id = screen_itt->second.block[block_x + block_y*16];
            auto block_itt = state.m_block_map.find(block_id);
            if (block_itt == state.m_block_map.end() || palette_itt == state.m_palette_map.end())
                continue;

            if (chr0_itt == state.m_chr_map.end() || chr1_itt == state.m_chr_map.end())
                continue;

            int slot_index = (((inscreen_y/8)&1)*2) | ((inscreen_x/8)&1);
            int tile_id = block_itt->second.tile_id[slot_index];
            const uint8_t* tile_set = tile_id < 128 ? chr0_itt->second.chr_data->data() : chr1_itt->second.chr_data->data();
            tile_set += (tile_id & 0x7F) * 16;
            int c = 0;
            if ((uint8_t)(tile_set[pixel_y+0] & (0x80 >> pixel_x)) != 0)
                c |= 1;
            if ((uint8_t)(tile_set[pixel_y+8] & (0x80 >> pixel_x)) != 0)
                c |= 2;

            uint8_t color = palette_itt->second.color[block_itt->second.palette*4 | c];
            image_line[x] = palette[color];
        }
    }

    ui->label_screen_render->setPixmap(QPixmap::fromImage(image));
    ui->label_screen_render->setMinimumSize(image.size());
    ui->label_screen_render->setMaximumSize(image.size());
}






