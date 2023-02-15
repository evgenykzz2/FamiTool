#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>

void MainWindow::GeneralTab_Init()
{
    ui->comboBox_palette_mode->blockSignals(true);
    ui->comboBox_palette_mode->addItem("2C02 (Famicom)", QVariant((int)Palette_2C02));
    ui->comboBox_palette_mode->addItem("2C03", QVariant((int)Palette_2C03));
    ui->comboBox_palette_mode->addItem("2C04", QVariant((int)Palette_2C04));
    ui->comboBox_palette_mode->addItem("fceux", QVariant((int)Palette_fceux));
    ui->comboBox_palette_mode->blockSignals(false);

    QValidator* validator_frame_number = new QIntValidator(0, 9999, this);
    ui->lineEdit_crop_x->setValidator(validator_frame_number);
    ui->lineEdit_crop_y->setValidator(validator_frame_number);
    ui->lineEdit_crop_width->setValidator(validator_frame_number);
    ui->lineEdit_crop_height->setValidator(validator_frame_number);
    ui->lineEdit_crop_height->setValidator(validator_frame_number);
    ui->lineEdit_target_width->setValidator(validator_frame_number);
    ui->lineEdit_target_height->setValidator(validator_frame_number);

    GeneralTab_UpdatePalette();
}

void MainWindow::GeneralTab_UpdatePalette()
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
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

void MainWindow::GeneralTab_UpdateMovieInfo()
{
    if (!m_avi_reader)
        return;
    QString info;
    if (m_avi_reader->VideoCompression() == 0)
        info += "RGB ";
    else
        info += "Invalid ";
    int height = (int)m_avi_reader->VideoHeight();
    info += QString("%1x%2").arg(m_avi_reader->VideoWidth()).arg(abs(height));
    info += QString(" %1fps").arg( (float)m_avi_reader->VideoTimeScale() / (float)m_avi_reader->VideoTicksPerFrame(), 0, 'f', 2);

    ui->label_movie_info->setText(info);
}

void MainWindow::GeneralTab_ReloadBaseTiles()
{
    if (ui->lineEdit_base_tiles->text().isEmpty())
        return;
    QImage image;
    if (!image.load(ui->lineEdit_base_tiles->text()))
    {
        QMessageBox::critical(0, "Error", QString("Can't load base tile image %1").arg(ui->lineEdit_base_tiles->text()));
        return;
    }
    m_base_tile_vector.clear();
    m_base_tile_map.clear();
    for (int y = 0; y < image.height(); y += 8)
    {
        for (int x = 0; x < image.width(); x += 8)
        {
            Tile tile;
            bool skip_tile = false;
            tile.bits.resize(16, 0);
            for (int yi = 0; yi < 8; ++yi)
            {
                if (y+yi >= image.height())
                    continue;
                if (skip_tile)
                    break;
                const uint8_t* src_line = image.scanLine(y+yi) + x*4;
                for (int xi = 0; xi < 8; ++xi)
                {
                    if (x+xi >= image.width())
                        continue;
                    uint32_t color = ((const uint32_t*)src_line)[xi] & 0xFFFFFF;
                    if (color == 0x000000)
                    {
                        //color 0
                    } else if (color == 0xFF0000)
                    {
                        //color 1
                        tile.bits[yi] |= 0x80 >> xi;
                    } else if (color == 0x00FF00)
                    {
                        //color 2
                        tile.bits[yi+8] |= 0x80 >> xi;
                    } else if (color == 0x0000FF)
                    {
                        //color 3
                        tile.bits[yi+0] |= 0x80 >> xi;
                        tile.bits[yi+8] |= 0x80 >> xi;
                    } else
                    {
                        skip_tile = true;
                        break;
                    }
                }
            }
            if (skip_tile)
                continue;
            auto itt = m_base_tile_map.find(tile);
            if (itt == m_base_tile_map.end())
            {
                size_t index = m_base_tile_vector.size();
                m_base_tile_vector.push_back(tile);
                m_base_tile_map.insert(std::make_pair(tile, index));
            }
        }
    }
    ui->label_base_tiles_info->setText(QString("%1 base tiles").arg(m_base_tile_vector.size()));
}

void MainWindow::on_comboBox_palette_mode_currentIndexChanged(int index)
{
    GeneralTab_UpdatePalette();
}

void MainWindow::on_pushButton_movie_browse_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Select video"), QDir::currentPath(), tr("*.avi"));
    if (file_name.isEmpty())
        return;

     ui->lineEdit_move_file_name->setText(file_name);
     m_movie_file_name = file_name;
     try {
         m_avi_reader = std::make_shared<AviReader>(file_name.toStdWString().c_str());
     } catch (const std::exception &ex)
     {
        QMessageBox::critical(0, "Error", QString("Can't read movie: %1").arg(ex.what()));
        return;
     }

     if (m_avi_reader->VideoCompression() != 0)
     {
         QMessageBox::critical(0, "Error", "Video has to be uncompressed");
         m_avi_reader.reset();
         return;
     }

     GeneralTab_UpdateMovieInfo();
     UpdateIndexedImage();
     VideoTab_Redraw();
}

void MainWindow::on_pushButton_base_tiles_browse_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Select base tileset"), QDir::currentPath(), tr("*.png"));
    if (file_name.isEmpty())
        return;
    ui->lineEdit_base_tiles->setText(file_name);
    GeneralTab_ReloadBaseTiles();
}
