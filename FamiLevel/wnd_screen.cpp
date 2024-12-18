#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QIcon>
#include <QMouseEvent>
#include <iostream>
#include <QFileInfo>
#include <QDir>

static std::map<int, QImage> s_block_image_map;
static double m_zoom = 1.0;
static std::map<QString, QImage> s_image_hash;

void MainWindow::ScreenWnd_Init()
{
    QList<int> sizes;
    sizes << 250 << 40;
    ui->splitter_screen->setSizes(sizes);
    ui->label_screen_render->installEventFilter(this);
    ui->widget_screen_pattern->setVisible(ui->checkBox_screen_patter->isChecked());
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
        //return QWidget::eventFilter( object, event );
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

    ui->scroll_screen_h->blockSignals(true);
    ui->scroll_screen_h->setMinimum(0);
    ui->scroll_screen_h->setMaximum(state.m_width_screens*256 - 256);
    ui->scroll_screen_h->blockSignals(false);

    ui->scroll_screen_v->blockSignals(true);
    ui->scroll_screen_v->setMinimum(0);
    ui->scroll_screen_v->setMaximum(state.m_height_screens*256 - 240);
    ui->scroll_screen_v->blockSignals(false);

    QFileInfo project_file_info(m_project_file_name);
    QDir project_dir(project_file_info.dir());

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
                /*for (int i = 0; i < 4; ++i)
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
                }*/

                if (!block_itt->second.overlay.isEmpty())
                {
                    QImage over(project_dir.absoluteFilePath(block_itt->second.overlay));
                    if (over.format() != QImage::Format_ARGB32)
                        over = over.convertToFormat(QImage::Format_ARGB32);
                    for (int y = 0; y < image.height(); ++y)
                    {
                        if (y >= over.height())
                            break;
                        uint8_t* line_ptr = image.scanLine(y);
                        uint8_t* src_ptr = over.scanLine(y);
                        for (int x = 0; x < image.width(); ++x)
                        {
                            if (x >= over.width())
                                break;
                            line_ptr[x*4+0] = (src_ptr[x*4+3] * src_ptr[x*4+0] + (255 - src_ptr[x*4+3])*line_ptr[x*4+0]) / 255;
                            line_ptr[x*4+1] = (src_ptr[x*4+3] * src_ptr[x*4+1] + (255 - src_ptr[x*4+3])*line_ptr[x*4+1]) / 255;
                            line_ptr[x*4+2] = (src_ptr[x*4+3] * src_ptr[x*4+2] + (255 - src_ptr[x*4+3])*line_ptr[x*4+2]) / 255;
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

    QFileInfo project_file_info(m_project_file_name);
    QDir project_dir(project_file_info.dir());

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
        int world_y = y + ui->scroll_screen_v->value();
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
            int world_x = x + ui->scroll_screen_h->value();
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
            int tile_id = 0;//block_itt->second.tile_id[slot_index];
            const uint8_t* tile_set = tile_id < 128 ? chr0_itt->second.chr_data->data() : chr1_itt->second.chr_data->data();
            tile_set += (tile_id & 0x7F) * 16;
            int c = 0;
            if ((uint8_t)(tile_set[pixel_y+0] & (0x80 >> pixel_x)) != 0)
                c |= 1;
            if ((uint8_t)(tile_set[pixel_y+8] & (0x80 >> pixel_x)) != 0)
                c |= 2;

            uint8_t color = palette_itt->second.color[block_itt->second.palette*4 | c];
            image_line[x] = palette[color];

            if (ui->checkBox_screen_draw_overlay->isChecked() && !block_itt->second.overlay.isEmpty())
            {
                auto image_itt = s_image_hash.find(block_itt->second.overlay);
                if (image_itt == s_image_hash.end())
                {
                    QImage image(project_dir.absoluteFilePath(block_itt->second.overlay));
                    if (image.format() != QImage::Format_ARGB32)
                        image = image.convertToFormat(QImage::Format_ARGB32);
                    s_image_hash.insert(std::make_pair(block_itt->second.overlay, image));
                    image_itt = s_image_hash.find(block_itt->second.overlay);
                }
                if (inblock_x < image_itt->second.width() && inblock_y < image_itt->second.height())
                {
                    uint8_t* src_ptr = image_itt->second.scanLine(inblock_y);
                    ((uint8_t*)image_line)[x*4+0] = (src_ptr[inblock_x*4+3] * src_ptr[inblock_x*4+0] + (255 - src_ptr[inblock_x*4+3])*((uint8_t*)image_line)[x*4+0]) / 255;
                    ((uint8_t*)image_line)[x*4+1] = (src_ptr[inblock_x*4+3] * src_ptr[inblock_x*4+1] + (255 - src_ptr[inblock_x*4+3])*((uint8_t*)image_line)[x*4+1]) / 255;
                    ((uint8_t*)image_line)[x*4+2] = (src_ptr[inblock_x*4+3] * src_ptr[inblock_x*4+2] + (255 - src_ptr[inblock_x*4+3])*((uint8_t*)image_line)[x*4+2]) / 255;
                }
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

        for (auto itt = state.m_world.begin(); itt != state.m_world.end(); ++itt)
        {
            int screen_x = itt->first.first * 256 - ui->scroll_screen_h->value();
            int screen_y = itt->first.second * 256 - ui->scroll_screen_v->value();
            if (screen_x >= 256 || screen_x < -256 ||
                screen_y >= 240 || screen_y < -256)
                continue;
            for (int i = 0; i < 256; ++i)
            {
                int block_x = (i & 15) * 16 + screen_x;
                int block_y = (i >> 4) * 16 + screen_y;
                if (block_x > 256 || block_y > 240 || block_x < -16 || block_y < -16)
                    continue;
                painter.drawText((block_x + 6)*m_zoom, (block_y + 6)*m_zoom, QString("%1").arg( (uint16_t)itt->second.block[i], 2, 16, QChar('0') ).toUpper());
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

static uint8_t SMB1_GetSky(int x, int y)
{
  if ( y < 2 || y >= 13 )
    return 0;
  uint8_t xi = x%48;
  if ( y == 2 )
  {
    switch( xi )
    {
      case 19:
      case 36:
        return 0x55;
      case 20:
      case 37:
      case 38:
        return 0x56;
      case 21:
      case 39:
        return 0x57;
    }
    return 0;
  } else if ( y == 3 )
  {
    switch( xi )
    {
      case 8:
      case 27:
        return 0x55;
      case 9:
      case 28:
      case 29:
      case 30:
        return 0x56;
      case 10:
      case 31:
        return 0x57;
      case 19:
      case 36:
        return 0x58;
      case 20:
      case 37:
      case 38:
        return 0x59;
      case 21:
      case 39:
        return 0x5A;
    }
    return 0;
  } else if ( y == 4 )
  {
    switch( xi )
    {
      case 8:
      case 27:
        return 0x58;
      case 9:
      case 28:
      case 29:
      case 30:
        return 0x59;
      case 10:
      case 31:
        return 0x5A;
    }
    return 0;
  } else
    return 0;
}

static uint8_t SMB1_GetGrass(int x, int y)
{
  if ( y < 3 || y >= 13 )
    return 0;
  uint8_t xi = x%48;
  if ( y == 10 )
  {
    if (xi == 2)
      return 7;
    else
      return 0;
  } else if ( y == 11 )
  {
    switch(xi)
    {
      case 1:
        return 0x05;
      case 2:
        return 0x09;
      case 3:
        return 0x08;
      case 17:
        return 0x07;
    }
    return 0;
  } else if ( y == 12 )
  {
    static const uint8_t data[24] = {0x59,0xA9,0x80,0x00,0x00,0x02,0x33,0x34,   0x59,0x80,0x00,0x02,0x34,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,0x02,0x34,0x00,0x00};
    if ( (xi&1) == 0 )
      return (data[xi>>1] >> 4);
    else
      return (data[xi>>1] & 0xF);
  } else
    return 0;
}

static uint8_t SMB1_GetSky2(int x, int y)
{
  if ( y < 2 || y >= 13 )
    return 0;
  uint8_t xi = x%48;
  if ( y == 2 )
  {
    switch( xi )
    {
      case 18:
        return 0x55;
      case 19:
      case 20:
        return 0x56;
      case 21:
        return 0x57;
    }
    return 0;
  } else if ( y == 3 )
  {
    switch( xi )
    {
      case 3:
        return 0x55;
      case 4:
      case 5:
        return 0x56;
      case 6:
        return 0x57;

      case 18:
        return 0x58;
      case 19:
      case 20:
        return 0x59;
      case 21:
        return 0x5A;
    }
    return 0;
  } else if ( y == 4 )
  {
    switch( xi )
    {
      case 3:
        return 0x58;
      case 4:
      case 5:
        return 0x59;
      case 6:
        return 0x5A;
    }
    return 0;
  } else if ( y == 6 )
  {
    switch( xi )
    {
      case 38:
        return 0x55;
      case 39:
        return 0x56;
      case 40:
        return 0x57;
    }
    return 0;
  } else if ( y == 7 )
  {
    switch( xi )
    {
      case 9:
        return 0x55;
      case 10:
        return 0x56;
      case 11:
        return 0x57;

    case 35:
      return 0x55;
    case 36:
      return 0x56;
    case 37:
      return 0x57;

    case 38:
      return 0x58;
    case 39:
      return 0x59;
    case 40:
      return 0x5A;

    }
    return 0;
  } else if ( y == 8 )
  {
    switch( xi )
    {
      case 9:
        return 0x58;
      case 10:
        return 0x59;
      case 11:
        return 0x5A;

    case 35:
      return 0x58;
    case 36:
      return 0x59;
    case 37:
      return 0x5A;
    }
    return 0;
  } else if ( y == 11 )
  {
    switch( xi )
    {
      case 28:
        return 0x55;
      case 29:
        return 0x56;
      case 30:
        return 0x57;
    case 46:
      return 0x55;
    case 47:
      return 0x56;
    case 0:
      return 0x57;
    }
    return 0;
  } else if ( y == 12 )
  {
    switch( xi )
    {
      case 28:
        return 0x58;
      case 29:
        return 0x59;
      case 30:
        return 0x5A;
    case 46:
      return 0x58;
    case 47:
      return 0x59;
    case 0:
      return 0x5A;
    }
    return 0;
  }  else
    return 0;
}

void MainWindow::on_btn_screen_smb_bg_clicked()
{
    State state = m_state.back();
    for (auto itt = state.m_world.begin(); itt != state.m_world.end(); ++itt)
    {
        for (int i = 0; i < 256; ++i)
        {
            int block_x = (i & 15);
            int block_y = (i >> 4);

            uint8_t id = 0;
            if (itt->first.second == state.m_height_screens-1)
                id |= SMB1_GetGrass(block_x + itt->first.first*16, block_y);
            id |= SMB1_GetSky(block_x + itt->first.first*16, block_y);
            if (itt->second.block[i] == 0)
                itt->second.block[i] = id;
        }
    }
    StatePush(state);
    ScreenWnd_RedrawScreen();
}


void MainWindow::on_btn_screen_fill_clicked()
{
    State state = m_state.back();
    int fill_id = ui->listWidget_screen->currentRow();
    for (auto itt = state.m_world.begin(); itt != state.m_world.end(); ++itt)
    {
        for (int i = 0; i < 256; ++i)
            itt->second.block[i] = fill_id;
    }
    StatePush(state);
    ScreenWnd_RedrawScreen();
}


void MainWindow::on_btn_screen_smb_sky_clicked()
{
    State state = m_state.back();
    for (auto itt = state.m_world.begin(); itt != state.m_world.end(); ++itt)
    {
        for (int i = 0; i < 256; ++i)
        {
            int block_x = (i & 15);
            int block_y = (i >> 4);
            if (itt->second.block[i] == 0)
                itt->second.block[i] = SMB1_GetSky2(block_x + itt->first.first*16, block_y);
        }
    }
    StatePush(state);
    ScreenWnd_RedrawScreen();
}
