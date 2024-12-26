#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QIcon>
#include <QMouseEvent>
#include <iostream>
#include <QFileInfo>
#include <QDir>

static std::map<int, QImage> s_block_image_map;
static std::map<int, QImage> s_block_raw_image_map;
static double m_zoom = 1.0;
static std::map<QString, QImage> s_image_hash;

void MainWindow::ScreenWnd_Init()
{
    QList<int> sizes;
    sizes << 250 << 40;
    ui->splitter_screen->setSizes(sizes);
    ui->label_screen_render->installEventFilter(this);
    ui->widget_screen_pattern->setVisible(ui->checkBox_screen_patter->isChecked());

    ui->comboBox_draw_transformation->blockSignals(true);
    ui->comboBox_draw_transformation->addItem("None");
    ui->comboBox_draw_transformation->addItem("1");
    ui->comboBox_draw_transformation->addItem("2");
    ui->comboBox_draw_transformation->addItem("3");
    ui->comboBox_draw_transformation->blockSignals(false);

}

void MainWindow::ScreenWnd_EventFilter(QObject* object, QEvent* event)
{
    if ( (object == ui->label_screen_render || object == ui->widget_screen_left) && event->type() == QEvent::Wheel)
    {
        QWheelEvent* wheel = (QWheelEvent*)event;
        m_zoom += (double)(wheel->angleDelta().y()) / 120.0 * 0.25;
        if (m_zoom < 1.0)
            m_zoom = 1.0;
        if (m_zoom > 4.0)
            m_zoom = 4.0;
        ScreenWnd_RedrawScreen();
    }


    if (object == ui->label_screen_render &&
            (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove))
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x()/m_zoom + ui->scroll_screen_h->value();
            int y = mouse_event->y()/m_zoom + ui->scroll_screen_v->value();

            State state = m_state.back();

            int block_x = x / 16;
            int block_y = y / 16;
            if (state.m_level_type == LevelType_Horizontal)
            {
                if (block_x < state.m_screen_tiles.size() && block_y < state.m_screen_tiles[block_x].size())
                {
                    if (state.m_screen_tiles[block_x][block_y] != ui->listWidget_screen->currentRow())
                    {
                        state.m_screen_tiles[block_x][block_y] = ui->listWidget_screen->currentRow();
                        StatePush(state);
                        ScreenWnd_RedrawScreen();
                    }
                }
            } else if (state.m_level_type == LevelType_VerticalMoveDown)
            {
                if (block_y < state.m_screen_tiles.size() && block_x < state.m_screen_tiles[block_y].size())
                {
                    if (state.m_screen_tiles[block_y][block_x] != ui->listWidget_screen->currentRow())
                    {
                        state.m_screen_tiles[block_y][block_x] = ui->listWidget_screen->currentRow();
                        StatePush(state);
                        ScreenWnd_RedrawScreen();
                    }
                }
            } else
            {
                int invy = state.m_screen_tiles.size() - 1 - block_y;
                if (block_y < state.m_screen_tiles.size() && block_x < state.m_screen_tiles[invy].size())
                {
                    if (state.m_screen_tiles[invy][block_x] != ui->listWidget_screen->currentRow())
                    {
                        state.m_screen_tiles[invy][block_x] = ui->listWidget_screen->currentRow();
                        StatePush(state);
                        ScreenWnd_RedrawScreen();
                    }
                }
            }
        }
    }
}

void MainWindow::ScreenWnd_FullRedraw()
{
    s_block_image_map.clear();
    s_block_raw_image_map.clear();
    State state = m_state.back();

    ui->scroll_screen_h->blockSignals(true);
    ui->scroll_screen_h->setMinimum(0);
    if (state.m_level_type == LevelType_Horizontal)
        ui->scroll_screen_h->setMaximum((state.m_length - YUUREIKUN_WIDTH)*16);
    else
        ui->scroll_screen_h->setMaximum(0);
    ui->scroll_screen_h->blockSignals(false);

    ui->scroll_screen_v->blockSignals(true);
    ui->scroll_screen_v->setMinimum(0);
    if (state.m_level_type == LevelType_Horizontal)
        ui->scroll_screen_v->setMaximum(0);
    else
        ui->scroll_screen_v->setMaximum((state.m_length - YUUREIKUN_HEIGHT)*16);
    ui->scroll_screen_v->blockSignals(false);

    QFileInfo project_file_info(m_project_file_name);
    QDir project_dir(project_file_info.dir());

    const uint32_t* palette = GetPalette(state.m_palette);
    std::vector<int> palette_yuv = GetPaletteYuv(state.m_palette);

    //ToDo
    auto palette_itt = state.m_palette_map.begin();
    if (palette_itt == state.m_palette_map.end())
        return;

    //Build images
    std::map<int, QImage> tileset_hash;
    for (auto block_itt = state.m_block_map.begin(); block_itt != state.m_block_map.end(); ++block_itt)
    {
        int tileset_id = block_itt->second.tileset;
        auto tileset_itt = state.m_tileset_map.find(tileset_id);
        if (tileset_itt == state.m_tileset_map.end())
            continue;
        auto hash_itt = tileset_hash.find(tileset_id);
        if (hash_itt == tileset_hash.end())
        {
            QString file_name = RelativeToAbsolute(tileset_itt->second.file_name);
            QImage img(file_name);
            tileset_hash.insert(std::make_pair(tileset_itt->first, img));
            hash_itt = tileset_hash.find(tileset_itt->first);
        }
        if (hash_itt == tileset_hash.end())
            continue;


        QImage tile_image;
        std::vector<uint8_t> tile_index;
        BuildBlock(block_itt->second, palette, palette_yuv,
                   palette_itt->second.color, hash_itt->second,
                   tile_image, tile_index);

        s_block_raw_image_map.insert(std::make_pair(block_itt->first, tile_image));
        if (!block_itt->second.overlay.isEmpty())
        {
            QImage over(project_dir.absoluteFilePath(block_itt->second.overlay));
            if (over.format() != QImage::Format_ARGB32)
                over = over.convertToFormat(QImage::Format_ARGB32);
            for (int y = 0; y < tile_image.height(); ++y)
            {
                if (y >= over.height())
                    break;
                uint8_t* line_ptr = tile_image.scanLine(y);
                uint8_t* src_ptr = over.scanLine(y);
                for (int x = 0; x < tile_image.width(); ++x)
                {
                    if (x >= over.width())
                        break;
                    line_ptr[x*4+0] = (src_ptr[x*4+3] * src_ptr[x*4+0] + (255 - src_ptr[x*4+3])*line_ptr[x*4+0]) / 255;
                    line_ptr[x*4+1] = (src_ptr[x*4+3] * src_ptr[x*4+1] + (255 - src_ptr[x*4+3])*line_ptr[x*4+1]) / 255;
                    line_ptr[x*4+2] = (src_ptr[x*4+3] * src_ptr[x*4+2] + (255 - src_ptr[x*4+3])*line_ptr[x*4+2]) / 255;
                }
            }
        }
        s_block_image_map.insert(std::make_pair(block_itt->first, tile_image));
    }

    ui->listWidget_screen->blockSignals(true);
    ui->listWidget_screen->clear();
    ui->listWidget_screen->setIconSize(QSize(32, 32));
    for (auto block_itt = state.m_block_map.begin(); block_itt != state.m_block_map.end(); ++block_itt)
    {
        auto image_itt = s_block_image_map.find(block_itt->first);
        if (image_itt == s_block_image_map.end())
            continue;
        ui->listWidget_screen->addItem( new QListWidgetItem(QIcon(QPixmap::fromImage(image_itt->second.scaled(32, 32))),
                    QString("%1:").arg( block_itt->first, 2, 16, QChar('0') ).toUpper() + block_itt->second.name) );

    }
    ui->listWidget_screen->blockSignals(false);

    ScreenWnd_RedrawScreen();
}

void MainWindow::on_checkBox_screen_draw_grid_clicked()
{
    ScreenWnd_RedrawScreen();
}

void MainWindow::on_scroll_screen_v_valueChanged(int value)
{
    ScreenWnd_RedrawScreen();
}

void MainWindow::on_scroll_screen_h_valueChanged(int value)
{
   ScreenWnd_RedrawScreen();
}


void MainWindow::on_checkBox_screen_show_block_index_clicked()
{
    ScreenWnd_RedrawScreen();
}

void MainWindow::on_checkBox_screen_draw_overlay_clicked()
{
    ScreenWnd_RedrawScreen();
}



void MainWindow::ScreenWnd_RedrawScreen()
{
    QImage image(YUUREIKUN_WIDTH*16, YUUREIKUN_HEIGHT*16, QImage::Format_ARGB32);
    image.fill(0xFF000000);
    State state = m_state.back();

    QImage tile_cach;
    int tile_cach_x = -1;
    int tile_cach_y = -1;
    int draw_mode = ui->comboBox_draw_transformation->currentIndex();

    if (state.m_level_type == LevelType_Horizontal)
    {
        for (int y = 0; y < image.height(); ++y)
        {
            int world_y = y + ui->scroll_screen_v->value();
            int block_y = world_y / 16;
            int pixel_y = world_y & 15;
            uint32_t* image_line = (uint32_t*)image.scanLine(y);

            for (int x = 0; x < image.width(); ++x)
            {
                int world_x = x + ui->scroll_screen_h->value();
                int block_x = world_x / 16;
                int pixel_x = world_x & 15;
                if (block_x != tile_cach_x || block_y != tile_cach_y)
                {
                    if (block_x < 0 || block_x >= state.m_screen_tiles.size())
                        continue;
                    if (block_y < 0 || block_y >= state.m_screen_tiles[block_x].size())
                        continue;
                    int block_id = state.m_screen_tiles[block_x][block_y];

                    if (draw_mode > 0)
                    {
                        int depth = draw_mode;
                        while (depth > 0)
                        {
                            depth--;
                            auto block_itt = state.m_block_map.find(block_id);
                            if (block_itt != state.m_block_map.end() && block_itt->second.transform_index >= 0)
                                block_id = block_itt->second.transform_index;
                        }
                    }

                    auto itt = s_block_raw_image_map.find(block_id);
                    if (itt == s_block_raw_image_map.end())
                        continue;

                    tile_cach_x = block_x;
                    tile_cach_y = block_y;
                    tile_cach = itt->second;
                }
                image_line[x] = tile_cach.pixel(pixel_x, pixel_y);
            }
        }
    } else
    {
        for (int y = 0; y < image.height(); ++y)
        {
            int world_y = y + ui->scroll_screen_v->value();
            int block_y = world_y / 16;
            int pixel_y = world_y & 15;
            uint32_t* image_line = (uint32_t*)image.scanLine(y);

            if (block_y < 0 || block_y >= state.m_screen_tiles.size())
                continue;

            if (state.m_level_type == LevelType_VerticalMoveUp)
                block_y = state.m_screen_tiles.size() - 1 - block_y;

            for (int x = 0; x < image.width(); ++x)
            {
                int world_x = x + ui->scroll_screen_h->value();
                int block_x = world_x / 16;
                int pixel_x = world_x & 15;
                if (block_x != tile_cach_x || block_y != tile_cach_y)
                {
                    if (block_x < 0 || block_x >= state.m_screen_tiles[block_y].size())
                        continue;
                    int block_id = state.m_screen_tiles[block_y][block_x];
                    if (draw_mode > 0)
                    {
                        int depth = draw_mode;
                        while (depth > 0)
                        {
                            depth--;
                            auto block_itt = state.m_block_map.find(block_id);
                            if (block_itt != state.m_block_map.end() && block_itt->second.transform_index >= 0)
                                block_id = block_itt->second.transform_index;
                        }
                    }

                    auto itt = s_block_raw_image_map.find(block_id);
                    if (itt == s_block_raw_image_map.end())
                        continue;

                    tile_cach_x = block_x;
                    tile_cach_y = block_y;
                    tile_cach = itt->second;
                }
                image_line[x] = tile_cach.pixel(pixel_x, pixel_y);
            }
        }
    }

    QImage scaled = image.scaled(image.width()*m_zoom, image.height()*m_zoom);
    if (ui->checkBox_screen_draw_grid->isChecked())
    {
        QPainter painter(&scaled);
        painter.setPen(QColor(0xFF00FF00));
        for (int y = 0; y < 240; ++y)
        {
            int world_y = y + ui->scroll_screen_v->value();
            if ((int)(world_y & 15) == 0)
                painter.drawLine(0, y*m_zoom, scaled.width(), y*m_zoom);
        }
        for (int x = 0; x < 256; ++x)
        {
            int world_x = x + ui->scroll_screen_h->value();
            if ((int)(world_x & 15) == 0)
                painter.drawLine(x*m_zoom, 0, x*m_zoom, scaled.height());
        }
    }

    if (ui->checkBox_screen_show_block_index->isChecked())
    {
        QPainter painter(&scaled);
        painter.setPen(QColor(0xFFFFFFFF));

        if (state.m_level_type == LevelType_Horizontal)
        {
            for (int y = 0; y < YUUREIKUN_HEIGHT; ++y)
            {
                int screen_y = y*16 - ui->scroll_screen_v->value();
                if (screen_y < 0 || screen_y > YUUREIKUN_HEIGHT*16)
                    continue;
                for (int x = 0; x < state.m_length; ++x)
                {
                    int screen_x = x*16 - ui->scroll_screen_h->value();
                    if (screen_x < 0 || screen_x > YUUREIKUN_WIDTH*16)
                        continue;
                    if (x >= state.m_screen_tiles.size())
                        continue;
                    if (y >= state.m_screen_tiles[x].size())
                        continue;
                    painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)state.m_screen_tiles[x][y], 2, 16, QChar('0') ).toUpper());
                }
            }
        } else
        {
            for (int y = 0; y < YUUREIKUN_HEIGHT; ++y)
            {
                int screen_y = y*16 - ui->scroll_screen_v->value();
                if (screen_y < 0 || screen_y > YUUREIKUN_HEIGHT*16)
                    continue;
                for (int x = 0; x < state.m_length; ++x)
                {
                    int screen_x = x*16 - ui->scroll_screen_h->value();
                    if (screen_x < 0 || screen_x > YUUREIKUN_WIDTH*16)
                        continue;
                    if (y >= state.m_screen_tiles.size())
                        continue;
                    if (x >= state.m_screen_tiles[y].size())
                        continue;
                    painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)state.m_screen_tiles[y][x], 2, 16, QChar('0') ).toUpper());
                }
            }
        }
    }

    ui->label_screen_render->setPixmap(QPixmap::fromImage(scaled));
    ui->label_screen_render->setMinimumSize(scaled.size());
    ui->label_screen_render->setMaximumSize(scaled.size());
}

void MainWindow::on_checkBox_screen_patter_clicked()
{
    ui->widget_screen_pattern->setVisible(ui->checkBox_screen_patter->isChecked());
}

void MainWindow::on_btn_screen_fill_clicked()
{
    State state = m_state.back();
    int fill_id = ui->listWidget_screen->currentRow();
    for (size_t n = 0; n < state.m_screen_tiles.size(); ++n)
    {
        for (int i = 0; i < state.m_screen_tiles[n].size(); ++i)
            state.m_screen_tiles[n][i] = fill_id;
    }
    StatePush(state);
    ScreenWnd_RedrawScreen();
}


void MainWindow::on_comboBox_draw_transformation_currentIndexChanged(int index)
{
    ScreenWnd_RedrawScreen();
}

