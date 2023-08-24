#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "QMouseEvent"

#define PALETTE_CELL_SIZE 64

void MainWindow::PaletteWnd_Init()
{
     ui->label_palette_set_current->installEventFilter(this);
}

void MainWindow::PaletteWnd_FullRedraw()
{
    State state = m_state.back();

    ui->comboBox_palette_set->blockSignals(true);
    ui->comboBox_palette_set->clear();
    for (auto itt = state.m_palette_map.begin(); itt != state.m_palette_map.end(); ++itt)
        ui->comboBox_palette_set->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_palette_set->setCurrentIndex(state.m_palette_map_index);
    ui->comboBox_palette_set->blockSignals(false);
    ui->lineEdit_palette_set->setText(ui->comboBox_palette_set->currentText());

    PaletteWnd_RedrawPalette();
}

void MainWindow::on_comboBox_palette_set_currentIndexChanged(int index)
{
    State state = m_state.back();
    if (state.m_palette_map_index == index)
        return;
    state.m_palette_map_index = index;
    StatePush(state);
    ui->lineEdit_palette_set->setText(ui->comboBox_palette_set->currentText());
    PaletteWnd_RedrawPalette();
}

void MainWindow::on_btn_palette_set_add_clicked()
{
    State state = m_state.back();
    int found_id = 0;
    for (int id = 0; id < 1024; ++id)
    {
        if (state.m_palette_map.find(id) == state.m_palette_map.end())
        {
            found_id = id;
            break;
        }
    }
    Palette pal;
    memset(pal.color, 0x0F, sizeof(pal.color));
    pal.name = QString("palette-%1").arg(found_id);
    state.m_palette_map.insert(std::make_pair(found_id, pal));

    ui->comboBox_palette_set->blockSignals(true);
    ui->comboBox_palette_set->addItem(pal.name, QVariant(found_id));
    ui->comboBox_palette_set->setCurrentIndex(ui->comboBox_palette_set->count()-1);
    state.m_palette_map_index = ui->comboBox_palette_set->count()-1;
    ui->comboBox_palette_set->blockSignals(false);
    ui->lineEdit_palette_set->setText(ui->comboBox_palette_set->currentText());
    StatePush(state);
    PaletteWnd_RedrawPalette();
}

static int s_palatte_pick_cell_index = 0;
void MainWindow::PaletteWnd_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->label_palette_set_current && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / PALETTE_CELL_SIZE;
            int y = mouse_event->y() / PALETTE_CELL_SIZE;
            if (x < 4 && y < 4)
            {
                State state = m_state.back();
                const uint32_t* palette = GetPalette(state.m_palette);
                s_palatte_pick_cell_index = x + y*4;
                int id = ui->comboBox_palette_set->itemData(state.m_palette_map_index).toInt();
                auto itt = state.m_palette_map.find(id);
                if (itt != state.m_palette_map.end())
                {
                    m_pick_color_dialog.SetPaletteIndex(itt->second.color[s_palatte_pick_cell_index]);
                    m_pick_color_dialog.SetPalette(palette);
                    m_pick_color_dialog.UpdatePalette();
                    m_pick_color_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                    connect(&m_pick_color_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteFamiWindow_Slot_PaletteSelect(int)), Qt::UniqueConnection);
                    m_pick_color_dialog.exec();
                }
            }
        }
    }
}

void MainWindow::PaletteFamiWindow_Slot_PaletteSelect(int index)
{
    State state = m_state.back();
    int id = ui->comboBox_palette_set->itemData(state.m_palette_map_index).toInt();
    auto itt = state.m_palette_map.find(id);
    if (itt == state.m_palette_map.end())
        return;
    itt->second.color[s_palatte_pick_cell_index] = index;
    StatePush(state);
    PaletteWnd_RedrawPalette();
}

void MainWindow::on_btn_palette_set_remove_clicked()
{
    State state = m_state.back();
    int id = ui->comboBox_palette_set->itemData(state.m_palette_map_index).toInt();
    auto itt = state.m_palette_map.find(id);
    if (itt == state.m_palette_map.end())
        return;
    state.m_palette_map.erase(itt);
    ui->comboBox_palette_set->blockSignals(true);
    ui->comboBox_palette_set->removeItem(state.m_palette_map_index);
    state.m_palette_map_index = ui->comboBox_palette_set->currentIndex();
    ui->comboBox_palette_set->blockSignals(false);
    StatePush(state);
    ui->lineEdit_palette_set->setText(ui->comboBox_palette_set->currentText());
    PaletteWnd_RedrawPalette();
}

void MainWindow::on_lineEdit_palette_set_editingFinished()
{
    State state = m_state.back();
    int id = ui->comboBox_palette_set->itemData(state.m_palette_map_index).toInt();
    auto itt = state.m_palette_map.find(id);
    if (itt == state.m_palette_map.end())
        return;
    if (itt->second.name == ui->lineEdit_palette_set->text())
        return;

    itt->second.name = ui->lineEdit_palette_set->text();
    ui->comboBox_palette_set->setItemText(state.m_palette_map_index,  ui->lineEdit_palette_set->text());
    StatePush(state);
}

void MainWindow::PaletteWnd_RedrawPalette()
{
    State state = m_state.back();
    int id = ui->comboBox_palette_set->itemData(state.m_palette_map_index).toInt();
    auto itt = state.m_palette_map.find(id);
    if (itt == state.m_palette_map.end())
    {
        ui->label_palette_set_current->setVisible(false);
        return;
    }

    const uint32_t* palette = GetPalette(state.m_palette);
    QImage image(4*PALETTE_CELL_SIZE, 4*PALETTE_CELL_SIZE, QImage::Format_ARGB32);
    image.fill(0x00);
    {
        QPainter painter(&image);
        painter.setPen(QColor(0xFFFFFFFF));
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                painter.setPen(QColor(0xFFFFFFFF));
                painter.setBrush(QColor(palette[itt->second.color[x+y*4]] | 0xFF000000));
                painter.drawRect(x*PALETTE_CELL_SIZE, y*PALETTE_CELL_SIZE, PALETTE_CELL_SIZE, PALETTE_CELL_SIZE);
                painter.drawText(x*PALETTE_CELL_SIZE+PALETTE_CELL_SIZE/2-4,
                                 y*PALETTE_CELL_SIZE+PALETTE_CELL_SIZE/2+4,
                                 QString("%1").arg((int)itt->second.color[x+y*4], 2, 16, QChar('0')).toUpper());
            }
        }
    }
    ui->label_palette_set_current->setPixmap(QPixmap::fromImage(image));
    ui->label_palette_set_current->setMinimumSize(image.size());
    ui->label_palette_set_current->setMaximumSize(image.size());
    ui->label_palette_set_current->setVisible(true);
}



