#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "QMouseEvent"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include "picojson.h"

static QLabel* s_block_tab_render;
static QImage s_tileset_image;
static int s_block_tab_zoom = 1;

static std::map<QString, QImage> s_image_hash;

void MainWindow::BlockWnd_Init()
{
    ui->comboBox_block_set_palette->addItem("0", QVariant((int)0));
    ui->comboBox_block_set_palette->addItem("1", QVariant((int)1));
    ui->comboBox_block_set_palette->addItem("2", QVariant((int)2));
    ui->comboBox_block_set_palette->addItem("3", QVariant((int)3));
    ui->label_block_set_view->installEventFilter(this);

    s_block_tab_render = new QLabel();
    s_block_tab_render->move(0, 0);
    ui->scrollArea_block->setWidget(s_block_tab_render);
    s_block_tab_render->installEventFilter(this);
    s_block_tab_render->setMouseTracking(true);
    ui->scrollArea_block->installEventFilter(this);
}

void MainWindow::BlockWnd_FullRedraw()
{
    State state = m_state.back();

    ui->comboBox_block_tileset->blockSignals(true);
    ui->comboBox_block_tileset->clear();
    for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt )
        ui->comboBox_block_tileset->addItem(itt->second.file_name, QVariant(itt->first));
    ui->comboBox_block_tileset->blockSignals(false);

    ui->comboBox_block_chrbank->blockSignals(true);
    ui->comboBox_block_chrbank->clear();
    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt )
        ui->comboBox_block_chrbank->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_block_chrbank->blockSignals(false);

    ui->comboBox_block_set->blockSignals(true);
    ui->comboBox_block_set->clear();
    for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt )
        ui->comboBox_block_set->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_block_set->setCurrentIndex(state.m_block_map_index);
    ui->comboBox_block_set->blockSignals(false);

    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());

    BlockWnd_RedrawTileset();
    BlockWnd_RedrawBlock();
}

void MainWindow::BlockWnd_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->label_block_set_view && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            /*int x = mouse_event->x() / 128;
            int y = mouse_event->y() / 128;
            if (x < 2 && y < 2)
            {
                int index = y*2 + x;
                State state = m_state.back();
                if (state.m_block_tile_slot != index)
                {
                    state.m_block_tile_slot = index;
                    StatePush(state);
                    BlockWnd_RedrawBlock();
                }
            }*/
        }
    }

    if (object == ui->scrollArea_block)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                s_block_tab_zoom ++;
                if (s_block_tab_zoom > 4)
                    s_block_tab_zoom = 4;
                else
                    BlockWnd_RedrawTileset();
            }
            if(key->key() == Qt::Key_Minus)
            {
                s_block_tab_zoom --;
                if (s_block_tab_zoom < 1)
                    s_block_tab_zoom = 1;
                else
                    BlockWnd_RedrawTileset();
            }
        }
    }

    if (object == s_block_tab_render && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / s_block_tab_zoom;
            int y = mouse_event->y() / s_block_tab_zoom;

            State state = m_state.back();
            int block_id = ui->comboBox_block_set->itemData(ui->comboBox_block_set->currentIndex()).toInt();
            auto block_itt = state.m_block_map.find(block_id);
            if (block_itt != state.m_block_map.end())
            {
                block_itt->second.tile_x = x & 0xFFFFF0;
                block_itt->second.tile_y = y & 0xFFFFF0;
                StatePush(state);
                BlockWnd_RedrawBlock();
            }
        }
    }

    /*if (object == ui->label_block_chr0 && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / 32;
            int y = mouse_event->y() / 32;
            if (x < 16 && y < 8)
            {
                int index = y*16 + x;
                State state = m_state.back();
                int block_id = ui->comboBox_block_set->itemData(ui->comboBox_block_set->currentIndex()).toInt();
                auto block_itt = state.m_block_map.find(block_id);
                if (block_itt != state.m_block_map.end())
                {
                    block_itt->second.tile_id[state.m_block_tile_slot] = index;
                    StatePush(state);
                    BlockWnd_RedrawBlock();
                }
            }
        }
    }*/
}

void MainWindow::on_comboBox_block_set_currentIndexChanged(int index)
{
    State state = m_state.back();
    if (state.m_block_map_index == index)
        return;
    state.m_block_map_index = index;
    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());
    StatePush(state);
    BlockWnd_RedrawBlock();
}

void MainWindow::on_lineEdit_block_set_editingFinished()
{
    State state = m_state.back();
    int id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto itt = state.m_block_map.find(id);
    if (itt == state.m_block_map.end())
        return;
    if (itt->second.name == ui->lineEdit_block_set->text())
        return;
    itt->second.name = ui->lineEdit_block_set->text();
    ui->comboBox_block_set->setItemText(state.m_block_map_index,  ui->lineEdit_block_set->text());
    StatePush(state);
}


void MainWindow::on_btn_block_set_add_clicked()
{
    State state = m_state.back();
    int found_id = 0;
    for (int id = 0; id < 1024; ++id)
    {
        if (state.m_block_map.find(id) == state.m_block_map.end())
        {
            found_id = id;
            break;
        }
    }
    Block block;
    block.name = QString("block-%1").arg(found_id);
    block.palette = ui->comboBox_block_set_palette->itemData(ui->comboBox_block_set_palette->currentIndex()).toInt();
    block.tile_x = 0;
    block.tile_y = 0;
    block.tileset = ui->comboBox_block_tileset->itemData(ui->comboBox_block_tileset->currentIndex()).toInt();
    block.chrbank = ui->comboBox_block_chrbank->itemData(ui->comboBox_block_chrbank->currentIndex()).toInt();

    //memset(block.tile_id, 0, sizeof(block.tile_id));
    state.m_block_map.insert(std::make_pair(found_id, block));

    ui->comboBox_block_set->blockSignals(true);
    ui->comboBox_block_set->addItem(block.name, QVariant(found_id));
    ui->comboBox_block_set->setCurrentIndex(ui->comboBox_block_set->count()-1);
    state.m_block_map_index = ui->comboBox_block_set->count()-1;
    ui->comboBox_block_set->blockSignals(false);
    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());

    StatePush(state);
    BlockWnd_RedrawBlock();
}


void MainWindow::on_btn_block_set_remove_clicked()
{
    State state = m_state.back();
    int id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto itt = state.m_block_map.find(id);
    if (itt == state.m_block_map.end())
        return;
    state.m_block_map.erase(itt);
    ui->comboBox_block_set->blockSignals(true);
    ui->comboBox_block_set->removeItem(state.m_block_map_index);
    state.m_block_map_index = ui->comboBox_block_set->currentIndex();
    ui->comboBox_block_set->blockSignals(false);
    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());

    StatePush(state);
    BlockWnd_RedrawBlock();
}

void MainWindow::on_comboBox_block_set_palette_currentIndexChanged(int index)
{
    State state = m_state.back();
    int id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto itt = state.m_block_map.find(id);
    if (itt == state.m_block_map.end())
        return;
    itt->second.palette = index;
    StatePush(state);
    BlockWnd_RedrawBlock();
}

void MainWindow::on_comboBox_block_tileset_currentIndexChanged(int index)
{
    BlockWnd_RedrawTileset();

    State state = m_state.back();
    int id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto itt = state.m_block_map.find(id);
    if (itt == state.m_block_map.end())
        return;
    itt->second.tileset = ui->comboBox_block_tileset->itemData(ui->comboBox_block_tileset->currentIndex()).toInt();
    StatePush(state);
    BlockWnd_RedrawBlock();
}

void MainWindow::BlockWnd_RedrawTileset()
{
    int tileset_id = ui->comboBox_block_tileset->itemData(ui->comboBox_block_tileset->currentIndex()).toInt();
    QString file_name = RelativeToAbsolute(ui->comboBox_block_tileset->currentText());
    QImage img(file_name);
    if (img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    s_tileset_image = img;

    QImage scaled = img.scaled(img.width()*s_block_tab_zoom, img.height()*s_block_tab_zoom);

    {
        QPainter painter(&scaled);
        painter.setPen(QColor(0xFF00FF00));
        for (int y = 0; y < img.height() / 16; ++y)
            painter.drawLine(0, y*16*s_block_tab_zoom, scaled.width(), y*16*s_block_tab_zoom);
        for (int x = 0; x < img.width() / 16; ++x)
            painter.drawLine(x*16*s_block_tab_zoom, 0, x*16*s_block_tab_zoom, scaled.height());
    }

    if (ui->checkBox_block_mark_used_tiles->isChecked())
    {
        State state = m_state.back();
        std::vector<uint8_t> indexed_image(img.width()*img.height(), 0);
        auto palette_itt = state.m_palette_map.begin();
        if (palette_itt == state.m_palette_map.end())
            return;
        std::vector<int> palette = GetPaletteYuv(state.m_palette);
        for (int y = 0; y < img.height(); ++y)
        {
            uint8_t* src = img.scanLine(y);
            uint8_t* dst = indexed_image.data() + y*img.width();
            for (int x = 0; x < img.width(); ++x)
            {
                int r = src[x*4+0];
                int g = src[x*4+1];
                int b = src[x*4+2];
                int cy =  ((int)(0.299   *1024)*r + (int)(0.587   *1024)*g + (int)(0.144   *1024)*b) >> 10;
                int cu = (((int)(-0.14713*1024)*r + (int)(-0.28886*1024)*g + (int)(0.436   *1024)*b) >> 10) + 128;
                int cv = (((int)(0.615   *1024)*r + (int)(-0.51499*1024)*g + (int)(-0.10001*1024)*b) >> 10) + 128;

                int best_diff = std::numeric_limits<int>::max();
                int best_c = 0;
                for (int c = 0; c < 64; ++c)
                {
                    int dr = cy - palette[c*4+0];
                    int dg = cu - palette[c*4+1];
                    int db = cv - palette[c*4+2];
                    int diff = dr*dr + dg*dg + db*db;
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_c = c;
                    }
                }
                dst[x] = best_c;
            }
        }

        std::map<std::vector<uint8_t>, int> tile_mapa;
        for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt)
        {
            if (itt->second.tileset != tileset_id)
                continue;
            std::vector<uint8_t> tile(16*16, 0);
            for (int y = 0; y < 16; ++y)
            {
                int offsy = y + itt->second.tile_y;
                if (offsy < 0 || offsy >= img.height())
                    continue;
                offsy *= img.width();
                for (int x = 0; x < 16; ++x)
                {
                    int offsx = x + itt->second.tile_x;
                    if (offsx < 0 || offsx >= img.width())
                        continue;
                    tile[x + y*16] = indexed_image[offsy + offsx];
                }
            }
            tile_mapa.insert(std::make_pair(tile, itt->first));
        }


        QPainter painter(&scaled);
        for (int by = 0; by < img.height(); by+=16)
        {
            for (int bx = 0; bx < img.width(); bx+=16)
            {
                std::vector<uint8_t> tile(16*16, 0);
                for (int y = 0; y < 16; ++y)
                {
                    int offsy = y + by;
                    if (offsy < 0 || offsy >= img.height())
                        continue;
                    offsy *= img.width();
                    for (int x = 0; x < 16; ++x)
                    {
                        int offsx = x + bx;
                        if (offsx < 0 || offsx >= img.width())
                            continue;
                        tile[x + y*16] = indexed_image[offsy + offsx];
                    }
                }

                auto itt = tile_mapa.find(tile);
                if (itt != tile_mapa.end())
                {
                    painter.setPen(QColor(0xFFFFFFFF));
                    painter.drawRect(bx*s_block_tab_zoom+1, by*s_block_tab_zoom+1, 16*s_block_tab_zoom-2, 16*s_block_tab_zoom-2);
                    painter.setPen(QColor(0xFF00FF00));
                    painter.drawRect(bx*s_block_tab_zoom+2, by*s_block_tab_zoom+2, 16*s_block_tab_zoom-4, 16*s_block_tab_zoom-4);
                    painter.setPen(QColor(0xFF000000));
                    painter.drawRect(bx*s_block_tab_zoom+3, by*s_block_tab_zoom+3, 16*s_block_tab_zoom-6, 16*s_block_tab_zoom-6);
                    painter.setPen(QColor(0xFFFFFFFF));
                    painter.drawRect(bx*s_block_tab_zoom+4, by*s_block_tab_zoom+4, 16*s_block_tab_zoom-8, 16*s_block_tab_zoom-8);
                }
            }
        }
    }

    //Mark all selected tiles
    if (ui->checkBox_block_mark_tiles->isChecked())
    {
        State state = m_state.back();
        QPainter painter(&scaled);
        for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt)
        {
            if (itt->second.tileset != tileset_id)
                continue;
            painter.setPen(QColor(0xFFFFFFFF));
            painter.drawRect(itt->second.tile_x*s_block_tab_zoom+1, itt->second.tile_y*s_block_tab_zoom+1, 16*s_block_tab_zoom-2, 16*s_block_tab_zoom-2);
            painter.setPen(QColor(0xFFFF0000));
            painter.drawRect(itt->second.tile_x*s_block_tab_zoom+2, itt->second.tile_y*s_block_tab_zoom+2, 16*s_block_tab_zoom-4, 16*s_block_tab_zoom-4);
            painter.setPen(QColor(0xFF000000));
            painter.drawRect(itt->second.tile_x*s_block_tab_zoom+3, itt->second.tile_y*s_block_tab_zoom+3, 16*s_block_tab_zoom-6, 16*s_block_tab_zoom-6);
            painter.setPen(QColor(0xFFFFFFFF));
            painter.drawRect(itt->second.tile_x*s_block_tab_zoom+4, itt->second.tile_y*s_block_tab_zoom+4, 16*s_block_tab_zoom-8, 16*s_block_tab_zoom-8);
        }
    }

    s_block_tab_render->setPixmap(QPixmap::fromImage(scaled));
    s_block_tab_render->setMinimumSize(scaled.size());
    s_block_tab_render->setMaximumSize(scaled.size());
}

/*void MainWindow::BlockWnd_RedrawChr0()
{
    State state = m_state.back();
    int chr_id = ui->comboBox_block_chr0->itemData(state.m_block_chr0).toInt();
    auto itt = state.m_chr_map.find(chr_id);
    auto palette_itt = state.m_palette_map.begin();

    int block_id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto block_itt = state.m_block_map.find(block_id);

    if (itt == state.m_chr_map.end() ||
        palette_itt == state.m_palette_map.end() ||
        block_itt == state.m_block_map.end())
    {
        ui->label_block_chr0->setVisible(false);
        return;
    }

    uint32_t color[4];
    const uint32_t* palette = GetPalette(state.m_palette);

    for (int i = 0; i < 4; ++i)
        color[i] = palette[palette_itt->second.color[block_itt->second.palette * 4 + i]] | 0xFF000000;

    QImage img = ChrRender(itt->second.chr_data->data(), 16, 8, 4, color);
    {
        QPainter painter(&img);
        painter.setPen(QColor(0xFFFFFFFF));
        for (int i = 1; i < 16; ++i)
        {
            painter.drawLine(0, i*4*8, img.width(), i*4*8);
            painter.drawLine(i*4*8, 0, i*4*8, img.height());
        }
    }
    ui->label_block_chr0->setPixmap(QPixmap::fromImage(img));
    ui->label_block_chr0->setMinimumSize(img.size());
    ui->label_block_chr0->setMaximumSize(img.size());
    ui->label_block_chr0->setVisible(true);
}*/


void MainWindow::BlockWnd_RedrawBlock()
{
    State state = m_state.back();
    int block_id = ui->comboBox_block_set->itemData(ui->comboBox_block_set->currentIndex()).toInt();
    auto block_itt = state.m_block_map.find(block_id);
    if (block_itt == state.m_block_map.end())
    {
        ui->widget_block_param->setVisible(false);
        return;
    }

    ui->comboBox_block_set_palette->blockSignals(true);
    ui->comboBox_block_set_palette->setCurrentIndex(block_itt->second.palette);
    ui->comboBox_block_set_palette->blockSignals(false);

    ui->comboBox_block_chrbank->blockSignals(true);
    for (int i = 0; i < ui->comboBox_block_chrbank->count(); ++i)
    {
        if (ui->comboBox_block_chrbank->itemText(i) == block_itt->second.chrbank)
        {
            ui->comboBox_block_chrbank->setCurrentIndex(i);
            break;
        }
    }
    ui->comboBox_block_chrbank->blockSignals(false);

    if (block_itt->second.overlay != ui->lineEdit_block_overlay->text())
        ui->lineEdit_block_overlay->setText(block_itt->second.overlay);

    auto palette_itt = state.m_palette_map.begin();
    if (palette_itt == state.m_palette_map.end() )
    {
        ui->widget_block_param->setVisible(false);
        return;
    }



    const uint32_t* palette = GetPalette(state.m_palette);
    std::vector<int> palette_yuv = GetPaletteYuv(state.m_palette);
    uint32_t color[4];

    for (int i = 0; i < 4; ++i)
    {
        int index = palette_itt->second.color[block_itt->second.palette * 4 + i];
        color[i] = palette[index] | 0xFF000000;
    }

    {
        QImage palette_img(32*4, 32, QImage::Format_ARGB32);
        for (int y = 0; y < palette_img.height(); ++y)
        {
            uint8_t* line = palette_img.scanLine(y);
            for (int x = 0; x < palette_img.width(); ++x)
            {
                int c = (x / 32) & 0x03;
                ((uint32_t*)line)[x] = color[c];
            }
        }
        ui->label_block_palette->setPixmap(QPixmap::fromImage(palette_img));
    }

    QImage image;
    std::vector<uint8_t> dest_index;
    BuildBlock(block_itt->second, palette, palette_yuv,
               palette_itt->second.color, s_tileset_image,
               image, dest_index);

    if (!block_itt->second.overlay.isEmpty())
    {
        auto image_itt = s_image_hash.find(block_itt->second.overlay);
        if (image_itt == s_image_hash.end())
        {
            QFileInfo project_file_info(m_project_file_name);
            QDir project_dir(project_file_info.dir());
            QImage image(project_dir.absoluteFilePath(block_itt->second.overlay));
            if (image.format() != QImage::Format_ARGB32)
                image = image.convertToFormat(QImage::Format_ARGB32);
            s_image_hash.insert(std::make_pair(block_itt->second.overlay, image));
            image_itt = s_image_hash.find(block_itt->second.overlay);
        }

        for (int y = 0; y < image.height(); ++y)
        {
            if (y >= image_itt->second.height())
                break;
            uint8_t* line_ptr = image.scanLine(y);
            uint8_t* src_ptr = image_itt->second.scanLine(y);
            for (int x = 0; x < image.width(); ++x)
            {
                if (x >= image_itt->second.width())
                    break;
                line_ptr[x*4+0] = (src_ptr[x*4+3] * src_ptr[x*4+0] + (255 - src_ptr[x*4+3])*line_ptr[x*4+0]) / 255;
                line_ptr[x*4+1] = (src_ptr[x*4+3] * src_ptr[x*4+1] + (255 - src_ptr[x*4+3])*line_ptr[x*4+1]) / 255;
                line_ptr[x*4+2] = (src_ptr[x*4+3] * src_ptr[x*4+2] + (255 - src_ptr[x*4+3])*line_ptr[x*4+2]) / 255;
            }
        }
    }

    int sz = 128;
    image = image.scaled(sz*2, sz*2);

    ui->label_block_set_view->setPixmap(QPixmap::fromImage(image));
    ui->label_block_set_view->setMinimumSize(image.size());
    ui->label_block_set_view->setMaximumSize(image.size());
    ui->widget_block_param->setVisible(true);
}



void MainWindow::on_btn_block_overlay_browse_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Select overlay 16x16 image"), QDir::currentPath(), tr("*.png *.gif *.bmp"));
    if (file_name.isEmpty())
        return;

    QFileInfo project_file_info(m_project_file_name);
    QDir project_dir(project_file_info.dir());
    QFileInfo info(file_name);
    if (info.isAbsolute())
        file_name = project_dir.relativeFilePath(file_name);

    State state = m_state.back();
    int block_id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto block_itt = state.m_block_map.find(block_id);
    if (block_itt == state.m_block_map.end())
        return;

    block_itt->second.overlay = file_name;
    StatePush(state);
    BlockWnd_RedrawBlock();
}


void MainWindow::on_btn_block_overlay_clear_clicked()
{
    State state = m_state.back();
    int block_id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto block_itt = state.m_block_map.find(block_id);
    if (block_itt == state.m_block_map.end())
        return;

    block_itt->second.overlay = "";
    StatePush(state);
    BlockWnd_RedrawBlock();
}




void MainWindow::on_btn_block_export_all_clicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Export blocks", QDir::currentPath(), "*.famiscroll-blocks");
    if (file_name.isEmpty())
        return;

    State state = m_state.back();

    picojson::value json;
    json.set<picojson::object>(picojson::object());

    json.get<picojson::object>()["block_chr0"] = picojson::value( (double)state.m_block_chr0 );
    json.get<picojson::object>()["block_chr1"] = picojson::value( (double)state.m_block_chr1 );
    json.get<picojson::object>()["block_map_index"] = picojson::value( (double)state.m_block_map_index );
    //json.get<picojson::object>()["block_tile_slot"] = picojson::value( (double)state.m_block_tile_slot );
    picojson::array items = picojson::array();
    for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt)
    {
            picojson::object item_obj;
            item_obj["id"] = picojson::value( (double)(itt->first) );
            item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
            //item_obj["t0"] = picojson::value( (double)(itt->second.tile_id[0]) );
            //item_obj["t1"] = picojson::value( (double)(itt->second.tile_id[1]) );
            //item_obj["t2"] = picojson::value( (double)(itt->second.tile_id[2]) );
            //item_obj["t3"] = picojson::value( (double)(itt->second.tile_id[3]) );
            item_obj["pal"] = picojson::value( (double)(itt->second.palette) );
            if (!itt->second.overlay.isEmpty())
                item_obj["overlay"] = picojson::value( itt->second.overlay.toUtf8().toBase64().data() );
            items.push_back(picojson::value(item_obj));
    }
    json.get<picojson::object>()["block_map"] = picojson::value(items);

    QString json_str = QString::fromStdString(json.serialize(true));
    QFile file(file_name);
    file.open(QFile::WriteOnly);
    if (file.error())
    {
        QMessageBox::critical(0, "Error", "Can't write file: " + file.errorString());
        return;
    }
    file.write(json_str.toLocal8Bit());
    file.close();
}


void MainWindow::on_btn_block_import_all_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Import blocks"), QDir::currentPath(), "*.famiscroll-blocks");
     if (file_name.isEmpty())
         return;

    QString json_str;
    QFile file(file_name);
    if (file.open(QFile::ReadOnly))
    {
        json_str = QString(file.readAll());
        file.close();
    } else
    {
        QMessageBox::critical(0, "Error", "Can't read file: " + file.errorString());
        return;
    }

    QFileInfo project_file_info(m_project_file_name);
    QDir project_dir(project_file_info.dir());

    picojson::value json;
    picojson::parse(json, json_str.toStdString());

    State state = m_state.back();
    state.m_block_map_index = 0;
    state.m_block_map.clear();

    if (json.contains("block_map"))
    {
        picojson::array items = json.get<picojson::object>()["block_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            Block block;
            int id = (int)(itt->get<picojson::object>()["id"].get<double>());
            block.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            //block.tile_id[0] = (int)itt->get<picojson::object>()["t0"].get<double>();
            //block.tile_id[1] = (int)itt->get<picojson::object>()["t1"].get<double>();
            //block.tile_id[2] = (int)itt->get<picojson::object>()["t2"].get<double>();
            //block.tile_id[3] = (int)itt->get<picojson::object>()["t3"].get<double>();
            block.palette = (int)itt->get<picojson::object>()["pal"].get<double>();
            if (itt->contains("overlay"))
                block.overlay = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["overlay"].get<std::string>() .c_str()));
            QFileInfo info(block.overlay);
            if (info.isAbsolute())
                block.overlay = project_dir.relativeFilePath(block.overlay);
            state.m_block_map.insert(std::make_pair(id, block));
        }
    }
    StatePush(state);
    BlockWnd_FullRedraw();
}



void MainWindow::on_checkBox_block_mark_tiles_clicked()
{
    BlockWnd_RedrawTileset();
}


void MainWindow::on_checkBox_block_mark_used_tiles_clicked()
{
    BlockWnd_RedrawTileset();
}
