#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>

void MainWindow::GeneralWnd_Init()
{
    QValidator* validator = new QIntValidator(1, 256, this);
    ui->edit_width_in_screens->setValidator(validator);
    ui->edit_height_in_screens->setValidator(validator);

    ui->comboBox_palette_mode->blockSignals(true);
    ui->comboBox_palette_mode->addItem("2C02 (Famicom)", QVariant((int)Palette_2C02));
    ui->comboBox_palette_mode->addItem("2C03", QVariant((int)Palette_2C03));
    ui->comboBox_palette_mode->addItem("2C04", QVariant((int)Palette_2C04));
    ui->comboBox_palette_mode->addItem("fceux", QVariant((int)Palette_fceux));
    ui->comboBox_palette_mode->blockSignals(false);
}

void MainWindow::on_edit_width_in_screens_editingFinished()
{
    int width = ui->edit_width_in_screens->text().toInt();
    State state = m_state.back();
    if (state.m_width_screens == width)
        return;
    state.m_width_screens = width;
    StatePush(state);
}

void MainWindow::on_edit_height_in_screens_editingFinished()
{
    int height = ui->edit_height_in_screens->text().toInt();
    State state = m_state.back();
    if (state.m_height_screens == height)
        return;
    state.m_height_screens = height;
    StatePush(state);
}

void MainWindow::on_comboBox_palette_mode_currentIndexChanged(int index)
{
    EPalette palette = (EPalette)ui->comboBox_palette_mode->itemData(index).toInt();
    State state = m_state.back();
    if (state.m_palette == palette)
        return;
    state.m_palette = palette;
    StatePush(state);
    GeneralWnd_RedrawPalette();
}

void MainWindow::GeneralWnd_FullRedraw()
{
    State state = m_state.back();
    ui->edit_width_in_screens->setText(QString("%1").arg(state.m_width_screens));
    ui->edit_height_in_screens->setText(QString("%1").arg(state.m_height_screens));
    GeneralWnd_RedrawPalette();
}

void MainWindow::GeneralWnd_RedrawPalette()
{
    State state = m_state.back();
    const uint32_t* palette = GetPalette(state.m_palette);
    static const int cell_size = 24;
    QImage image(16*cell_size, 4*cell_size, QImage::Format_ARGB32);
    image.fill(0x00);
    {
        QPainter painter(&image);
        painter.setPen(QColor(0xFFFFFFFF));
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 16; ++x)
            {
                painter.setBrush(QColor(palette[x+y*16] | 0xFF000000));
                painter.drawRect(x*cell_size, y*cell_size, cell_size, cell_size);
            }
        }
    }
    ui->label_general_palette->setPixmap(QPixmap::fromImage(image));
    ui->label_general_palette->setMinimumSize(image.size());
    ui->label_general_palette->setMaximumSize(image.size());
}
