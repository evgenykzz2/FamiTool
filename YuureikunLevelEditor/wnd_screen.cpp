#include "mainwindow.h"
#include "qdebug.h"
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
static std::vector<QImage> s_event_image_vector;

struct GhostEvent
{
    QString url;
    QString text;
    EEvent event;

    GhostEvent(const QString& _url, const QString& _text, EEvent _event):
        url(_url),
        text(_text),
        event(_event)
    {}
};

static const GhostEvent s_ghost_event[] =
{
    GhostEvent(":/res/res/generic/none.png", "None", Event_None),
    GhostEvent(":/res/res/generic/checkPoint.png", "Checkpoint", Event_CheckPoint),

    GhostEvent(":/res/res/level1/crow.png",         "1-Crow",         Event_Level1_Crow),
    GhostEvent(":/res/res/level1/darkDevil.png",    "1-Dark Devil",   Event_Level1_DarkDevil),
    GhostEvent(":/res/res/level1/humanSoul.png",    "1-Human Soul",   Event_Level1_HumanSoul),
    GhostEvent(":/res/res/level1/bambooShoot.png",  "1-Bamboo Shoot", Event_Level1_BambooShoot),
    GhostEvent(":/res/res/level1/bambooLeaf.png",   "1-Bamboo Leaf",  Event_Level1_BambooLeaf),
    GhostEvent(":/res/res/level1/bossBrick.png",    "1-Boss Brick",   Event_Level1_BossBrick),
};

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

    ui->listWidget_events->setVisible(ui->checkBox_events->isChecked());

    s_event_image_vector.resize(256);
    ui->listWidget_events->blockSignals(true);
    ui->listWidget_events->clear();
    ui->listWidget_events->setIconSize(QSize(32, 32));
    for (int i = 0; i < sizeof(s_ghost_event)/sizeof(s_ghost_event[0]); ++i)
    {
        s_event_image_vector[(int)s_ghost_event[i].event] = QImage(s_ghost_event[i].url);
        ui->listWidget_events->addItem( new QListWidgetItem(QIcon(QPixmap::fromImage(QImage(s_ghost_event[i].url).scaled(32, 32))), s_ghost_event[i].text));
    }

    ui->listWidget_events->blockSignals(false);
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
            if (state.m_level_type == LevelType_VerticalMoveDown)
            {
                int t = block_x;
                block_x = block_y;
                block_y = t;
            } else if (state.m_level_type == LevelType_VerticalMoveUp)
            {
                int t = block_x;
                block_x = state.m_screen_tiles.size() - 1 - block_y;
                block_y = t;
            }

            if (ui->checkBox_events->isChecked())
            {
                if (block_x < state.m_event.size())
                {
                    if (ui->listWidget_events->currentRow() == 0)
                        block_y = 0;
                    if (state.m_event[block_x].first != ui->listWidget_events->currentRow() ||
                        state.m_event[block_x].second != block_y)
                    {
                        state.m_event[block_x].first = ui->listWidget_events->currentRow();
                        state.m_event[block_x].second = block_y;
                        StatePush(state);
                        ScreenWnd_RedrawScreen();
                    }
                }
            } else
            {
                int depth = ui->comboBox_draw_transformation->currentIndex();
                if (depth == 0)
                {
                    if (block_x < state.m_screen_tiles.size() && block_y < state.m_screen_tiles[block_x].size())
                    {
                        if (state.m_screen_tiles[block_x][block_y] != ui->listWidget_screen->currentRow())
                        {
                            state.m_screen_tiles[block_x][block_y] = ui->listWidget_screen->currentRow();
                            ScreenWnd_ValidateDepth(state, block_x, block_y);
                            StatePush(state);
                            ScreenWnd_RedrawScreen();
                        }
                    }
                } else
                {
                    if (block_x < state.m_depth_tiles[depth-1].size() && block_y < state.m_depth_tiles[depth-1][block_x].size())
                    {
                        if (state.m_depth_tiles[depth-1][block_x][block_y] != ui->listWidget_screen->currentRow())
                        {
                            state.m_depth_tiles[depth-1][block_x][block_y] = ui->listWidget_screen->currentRow();
                            ScreenWnd_ValidateDepth(state, block_x, block_y);
                            StatePush(state);
                            ScreenWnd_RedrawScreen();
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::ScreenWnd_ValidateDepth(State &state, int x, int y)
{
    int block_main = state.m_screen_tiles[x][y];
    auto block_main_itt = state.m_block_map.find(block_main);
    if (block_main_itt == state.m_block_map.end())
        return;

    /*if (!BlockLogicCanTransform(block_main_itt->second.block_logic))
    {
        for (int depth = 0; depth < 3; ++depth)
            state.m_depth_tiles[depth][x][y] = -1;
        return;
    }*/

    for (int depth = 0; depth < 3; ++depth)
    {
        int block = state.m_depth_tiles[depth][x][y];
        if ((depth == 0 && block == block_main))
        {
            for (int d = 0; d < 3; ++d)
                state.m_depth_tiles[d][x][y] = -1;
            return;
        }

        if (block < 0)
        {
            for (int d = depth+1; d < 3; ++d)
                state.m_depth_tiles[d][x][y] = -1;
            return;
        }

        auto block_itt = state.m_block_map.find(block);
        if (block_itt == state.m_block_map.end())
        {
            for (int d = depth; d < 3; ++d)
                state.m_depth_tiles[d][x][y] = -1;
            return;
        }
        /*if (!BlockLogicCanTransform(block_itt->second.block_logic))
        {
            for (int d = depth+1; d < 3; ++d)
                state.m_depth_tiles[d][x][y] = -1;
        }*/
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
    int depth = ui->comboBox_draw_transformation->currentIndex();

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

                    if (depth > 0)
                    {
                        for (int d = 0; d < depth; ++d)
                        {
                            int b = state.m_depth_tiles[d][block_x][block_y];
                            if (b >= 0)
                                block_id = b;
                            else
                                break;
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
                    if (depth > 0)
                    {
                        for (int d = 0; d < depth; ++d)
                        {
                            int b = state.m_depth_tiles[d][block_y][block_x];
                            if (b >= 0)
                                block_id = b;
                            else
                                break;
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

    if (ui->checkBox_events->isChecked())
    {
        QPainter painter(&image);
        for (size_t n = 0; n < state.m_event.size(); ++n)
        {
            int block_x = n;
            int block_y = state.m_event[n].second;
            if (state.m_level_type == LevelType_VerticalMoveDown)
            {
                int t = block_x;
                block_x = block_y;
                block_y = t;
            } else if (state.m_level_type == LevelType_VerticalMoveUp)
            {
                int t = block_x;
                block_x = state.m_screen_tiles.size() - 1 - block_y;
                block_y = t;
            }

            int xpos = block_x*16 - ui->scroll_screen_h->value();
            int ypos = block_y*16 - ui->scroll_screen_v->value();
            if (xpos > -16 && xpos < image.width() &&
                ypos > -16 && ypos < image.height())
            {
                //qDebug() << xpos << ypos << state.m_event[n].first
                         //<< s_event_image_vector[state.m_event[n].first].width()
                         //<< s_event_image_vector[state.m_event[n].first].height();
                painter.drawImage(xpos, ypos, s_event_image_vector[state.m_event[n].first]);
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
                    int block_id = state.m_screen_tiles[x][y];
                    if (depth > 0)
                        block_id = state.m_depth_tiles[depth-1][x][y];

                    if (block_id >= 0)
                    {
                        painter.setPen(QColor(0xFF000000));
                        painter.drawText((screen_x + 6)*m_zoom+1, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                        painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom+1, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                        painter.setPen(QColor(0xFFFFFFFF));
                        painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                    }
                }
            }
        } else
        {
            for (int y = 0; y < state.m_length; ++y)
            {
                if (y >= state.m_screen_tiles.size())
                    continue;
                int block_y = y;
                if (state.m_level_type == LevelType_VerticalMoveUp)
                    block_y = state.m_screen_tiles.size() - 1 - block_y;

                int screen_y = y*16 - ui->scroll_screen_v->value();
                if (screen_y < 0 || screen_y > YUUREIKUN_HEIGHT*16)
                    continue;
                for (int x = 0; x < YUUREIKUN_WIDTH; ++x)
                {
                    int screen_x = x*16 - ui->scroll_screen_h->value();
                    if (screen_x < 0 || screen_x > YUUREIKUN_WIDTH*16)
                        continue;
                    if (x >= state.m_screen_tiles[block_y].size())
                        continue;

                    int block_id = state.m_screen_tiles[block_y][x];
                    if (depth > 0)
                        block_id = state.m_depth_tiles[depth-1][block_y][x];

                    if (block_id >= 0)
                    {
                        painter.setPen(QColor(0xFF000000));
                        painter.drawText((screen_x + 6)*m_zoom+1, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                        painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom+1, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                        painter.setPen(QColor(0xFFFFFFFF));
                        painter.drawText((screen_x + 6)*m_zoom, (screen_y + 6)*m_zoom, QString("%1").arg( (uint16_t)block_id, 2, 16, QChar('0') ).toUpper());
                    }
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


void MainWindow::on_checkBox_events_clicked()
{
    ui->listWidget_events->setVisible(ui->checkBox_events->isChecked());
    ui->listWidget_screen->setVisible(!ui->checkBox_events->isChecked());
    ScreenWnd_RedrawScreen();
}
