#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "QMouseEvent"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>

#define CHR_WIDTH_TILES 16
#define CHR_HEIGHT_TILES 8
#define CHR_ZOOM 4

struct TChrRender
{
    std::vector<uint8_t> chr_bits;
    std::map<std::vector<uint8_t>, int> tile_map;
    QImage chr_image;
};

std::map<int, TChrRender> s_chr_render;

void MainWindow::ChrWnd_Init()
{
    ui->comboBox_chr_size->addItem("1KB", QVariant((int)1024));
    ui->comboBox_chr_size->addItem("2KB", QVariant((int)2048));
    ui->comboBox_chr_size->addItem("4KB", QVariant((int)4096));
}

void MainWindow::ChrWnd_FullRedraw()
{
    State state = m_state.back();

    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->clear();
    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt)
        ui->comboBox_chr_set->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_chr_set->setCurrentIndex(state.m_chr_map_index);
    ui->comboBox_chr_set->blockSignals(false);

    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());

    ChrWnd_Recalculate();
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::ChrWnd_RedrawImage()
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
    {
        ui->label_chr_render->setVisible(false);
        return;
    }

    auto render_itt = s_chr_render.find(id);
    if (render_itt == s_chr_render.end())
    {
        ui->label_chr_render->setVisible(false);
        return;
    }

    QImage image = render_itt->second.chr_image.scaled(render_itt->second.chr_image.width()*4, render_itt->second.chr_image.height()*4);

    ui->label_chr_render->setMinimumSize(image.size());
    ui->label_chr_render->setMaximumSize(image.size());
    ui->label_chr_render->setPixmap(QPixmap::fromImage(image));
    ui->label_chr_render->setVisible(true);
}

void MainWindow::ChrWnd_RedrawFileName()
{
    State state = m_state.back();
    if (state.m_chr_map.empty())
        ui->widget_chr_set_item->setVisible(false);
    else
        ui->widget_chr_set_item->setVisible(true);

    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt != state.m_chr_map.end())
    {
        ui->comboBox_chr_size->blockSignals(true);
        for (int i = 0; i < ui->comboBox_chr_size->count(); ++i)
        {
            if (ui->comboBox_chr_size->itemData(i).toInt() == itt->second.chr_size)
            {
                ui->comboBox_chr_size->setCurrentIndex(i);
                break;
            }
        }
        ui->comboBox_chr_size->blockSignals(false);
    }
}

void MainWindow::on_comboBox_chr_size_currentIndexChanged(int index)
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
        return;
    itt->second.chr_size = ui->comboBox_chr_size->itemData(ui->comboBox_chr_size->currentIndex()).toInt();
    StatePush(state);
    ChrWnd_Recalculate();
    ChrWnd_RedrawImage();
}


void MainWindow::on_comboBox_chr_set_currentIndexChanged(int index)
{
    State state = m_state.back();
    if (state.m_chr_map_index == index)
        return;
    state.m_chr_map_index = index;
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());
    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::on_lineEdit_chr_set_editingFinished()
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
        return;
    if (itt->second.name == ui->lineEdit_chr_set->text())
        return;
    itt->second.name = ui->lineEdit_chr_set->text();
    ui->comboBox_chr_set->setItemText(state.m_chr_map_index,  ui->lineEdit_chr_set->text());
    StatePush(state);
}

void MainWindow::on_btn_chr_set_add_clicked()
{
    State state = m_state.back();
    int found_id = 0;
    for (int id = 0; id < 1024; ++id)
    {
        if (state.m_chr_map.find(id) == state.m_chr_map.end())
        {
            found_id = id;
            break;
        }
    }
    ChrSet chr;
    chr.name = QString("chr-%1").arg(found_id);
    state.m_chr_map.insert(std::make_pair(found_id, chr));

    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->addItem(chr.name, QVariant(found_id));
    ui->comboBox_chr_set->setCurrentIndex(ui->comboBox_chr_set->count()-1);
    state.m_chr_map_index = ui->comboBox_chr_set->count()-1;
    ui->comboBox_chr_set->blockSignals(false);
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());

    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}


void MainWindow::on_btn_chr_set_remove_clicked()
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
        return;
    state.m_chr_map.erase(itt);
    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->removeItem(state.m_chr_map_index);
    state.m_chr_map_index = ui->comboBox_chr_set->currentIndex();
    ui->comboBox_chr_set->blockSignals(false);
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());
    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::ChrWnd_Recalculate()
{
    State state = m_state.back();
    s_chr_render.clear();

    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt)
    {
        TChrRender render;
        render.chr_image = QImage(128, itt->second.chr_size / 32, QImage::Format_ARGB32);
        render.chr_image.fill(0xFF00FF00);
        s_chr_render.insert(std::make_pair(itt->first, render));
    }

    auto palette_itt = state.m_palette_map.begin();
    if (palette_itt == state.m_palette_map.end())
        return;

    const uint32_t* palette = GetPalette(state.m_palette);
    std::vector<int> palette_yuv = GetPaletteYuv(state.m_palette);

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

        auto render_itt = s_chr_render.find(block_itt->second.chrbank);
        if (render_itt == s_chr_render.end())
            continue;

        QImage tile_image;
        std::vector<uint8_t> dest_index;
        BuildBlock(block_itt->second, palette, palette_yuv,
                   palette_itt->second.color, hash_itt->second,
                   tile_image, dest_index);

        for (int yb = 0; yb < 2; ++yb)
        {
            for (int xb = 0; xb < 2; ++xb)
            {
                QImage image = tile_image.copy(xb*8, yb*8, 8, 8);
                std::vector<uint8_t> tile(16, 0);
                for (int y = 0; y < 8; ++y)
                {
                    for (int x = 0; x < 8; ++x)
                    {
                        int index = dest_index[x + xb*8 + (y + yb*8)*16];
                        if ( (int)(index & 1) == 1)
                            tile[y+0] |= (0x80 >> x);
                        if ( (int)(index & 2) == 2)
                            tile[y+8] |= (0x80 >> x);
                    }
                }

                auto tile_itt = render_itt->second.tile_map.find(tile);
                if (tile_itt == render_itt->second.tile_map.end())
                {
                    int index = render_itt->second.tile_map.size();
                    render_itt->second.tile_map.insert(std::make_pair(tile, index));
                    render_itt->second.chr_bits.insert(render_itt->second.chr_bits.end(), tile.begin(), tile.end());
                    QPainter painter(&render_itt->second.chr_image);
                    painter.drawImage((index % 16) * 8, (index / 16) * 8, image);
                }
            }
        }

    }
}






