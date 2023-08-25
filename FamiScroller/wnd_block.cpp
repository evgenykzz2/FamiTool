#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "QMouseEvent"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>


void MainWindow::BlockWnd_Init()
{
    ui->comboBox_block_set_palette->addItem("0");
    ui->comboBox_block_set_palette->addItem("1");
    ui->comboBox_block_set_palette->addItem("2");
    ui->comboBox_block_set_palette->addItem("3");
    ui->label_block_set_view->installEventFilter(this);
    ui->label_block_chr0->installEventFilter(this);
    ui->label_block_chr1->installEventFilter(this);
}

void MainWindow::BlockWnd_FullRedraw()
{
    State state = m_state.back();

    ui->comboBox_block_chr0->blockSignals(true);
    ui->comboBox_block_chr0->clear();
    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt )
        ui->comboBox_block_chr0->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_block_chr0->setCurrentIndex(state.m_block_chr0);
    ui->comboBox_block_chr0->blockSignals(false);

    ui->comboBox_block_chr1->blockSignals(true);
    ui->comboBox_block_chr1->clear();
    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt )
        ui->comboBox_block_chr1->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_block_chr1->setCurrentIndex(state.m_block_chr1);
    ui->comboBox_block_chr1->blockSignals(false);

    ui->comboBox_block_set->blockSignals(true);
    ui->comboBox_block_set->clear();
    for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt )
        ui->comboBox_block_set->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_block_set->setCurrentIndex(state.m_block_map_index);
    ui->comboBox_block_set->blockSignals(false);

    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());

    BlockWnd_RedrawBlock();
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
}

void MainWindow::BlockWnd_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->label_block_set_view && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / 128;
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
            }
        }
    }

    if (object == ui->label_block_chr0 && event->type() == QEvent::MouseButtonPress)
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
    }

    if (object == ui->label_block_chr1 && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / 32;
            int y = mouse_event->y() / 32;
            if (x < 16 && y < 8)
            {
                int index = y*16 + x + 128;
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
    }
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
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
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
    block.palette = 0;
    memset(block.tile_id, 0, sizeof(block.tile_id));
    state.m_block_map.insert(std::make_pair(found_id, block));

    ui->comboBox_block_set->blockSignals(true);
    ui->comboBox_block_set->addItem(block.name, QVariant(found_id));
    ui->comboBox_block_set->setCurrentIndex(ui->comboBox_block_set->count()-1);
    state.m_block_map_index = ui->comboBox_block_set->count()-1;
    ui->comboBox_block_set->blockSignals(false);
    ui->lineEdit_block_set->setText(ui->comboBox_block_set->currentText());

    StatePush(state);
    BlockWnd_RedrawBlock();
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
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
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
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
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
}

void MainWindow::on_comboBox_block_chr0_currentIndexChanged(int index)
{
    State state = m_state.back();
    state.m_block_chr0 = index;
    StatePush(state);
    BlockWnd_RedrawChr0();
    BlockWnd_RedrawChr1();
}

void MainWindow::on_comboBox_block_chr1_currentIndexChanged(int index)
{
    State state = m_state.back();
    state.m_block_chr1 = index;
    StatePush(state);
    BlockWnd_RedrawChr1();
}

void MainWindow::BlockWnd_RedrawChr0()
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
}

void MainWindow::BlockWnd_RedrawChr1()
{
    State state = m_state.back();
    int chr_id = ui->comboBox_block_chr1->itemData(state.m_block_chr1).toInt();
    auto itt = state.m_chr_map.find(chr_id);
    auto palette_itt = state.m_palette_map.begin();

    int block_id = ui->comboBox_block_set->itemData(state.m_block_map_index).toInt();
    auto block_itt = state.m_block_map.find(block_id);

    if (itt == state.m_chr_map.end() ||
        palette_itt == state.m_palette_map.end() ||
        block_itt == state.m_block_map.end())
    {
        ui->label_block_chr1->setVisible(false);
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
    ui->label_block_chr1->setPixmap(QPixmap::fromImage(img));
    ui->label_block_chr1->setMinimumSize(img.size());
    ui->label_block_chr1->setMaximumSize(img.size());
    ui->label_block_chr1->setVisible(true);
}

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
    //ui->comboBox_block_set_palette->setCurrentIndex(block_itt->second.palette);

    int chr0_id = ui->comboBox_block_chr0->itemData(state.m_block_chr0).toInt();
    int chr1_id = ui->comboBox_block_chr1->itemData(state.m_block_chr1).toInt();

    auto chr0_itt = state.m_chr_map.find(chr0_id);
    auto chr1_itt = state.m_chr_map.find(chr1_id);

    auto palette_itt = state.m_palette_map.begin();
    if (palette_itt == state.m_palette_map.end() ||
        chr0_itt == state.m_chr_map.end() ||
        chr1_itt == state.m_chr_map.end() )
    {
        ui->widget_block_param->setVisible(false);
        return;
    }

    uint32_t color[4];
    const uint32_t* palette = GetPalette(state.m_palette);

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
    int sz = 128;
    image = image.scaled(sz*2, sz*2);
    {
        QPainter painter(&image);
        painter.setPen(QColor(0xFFFFFFFF));
        painter.drawText(sz/2-8, sz/2-4, QString("%1").arg(block_itt->second.tile_id[0], 2, 16, QChar('0').toUpper()));
        painter.drawText(sz + sz/2-8, sz/2-4, QString("%1").arg(block_itt->second.tile_id[1], 2, 16, QChar('0').toUpper()));
        painter.drawText(sz/2-8, sz+sz/2-4, QString("%1").arg(block_itt->second.tile_id[2], 2, 16, QChar('0').toUpper()));
        painter.drawText(sz + sz/2-8, sz+sz/2-4, QString("%1").arg(block_itt->second.tile_id[3], 2, 16, QChar('0').toUpper()));
        painter.setBrush(Qt::transparent);
        if (state.m_block_tile_slot == 0)
            painter.drawRect(0, 0, sz-1, sz-1);
        if (state.m_block_tile_slot == 1)
            painter.drawRect(sz, 0, sz-1, sz-1);
        if (state.m_block_tile_slot == 2)
            painter.drawRect(0, sz, sz-1, sz-1);
        if (state.m_block_tile_slot == 3)
            painter.drawRect(sz, sz, sz-1, sz-1);
    }

    ui->label_block_set_view->setPixmap(QPixmap::fromImage(image));
    ui->label_block_set_view->setMinimumSize(image.size());
    ui->label_block_set_view->setMaximumSize(image.size());
    ui->widget_block_param->setVisible(true);
}


