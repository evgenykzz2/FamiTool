#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string.h>
#include <memory.h>
#include <iomanip>
#include <QFile>

void MainWindow::Export_SpriteConvert()
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
    int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        QImage cut = m_spriteset_indexed.copy(m_slice_vector[index].x, m_slice_vector[index].y, m_slice_vector[index].width, m_slice_vector[index].height);
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
        {
            int pindex = m_slice_vector[index].oam[n].palette;
            Palette pal = m_palette_set[pindex];
            m_slice_vector[index].oam[n].chr_export.resize(sprite_height*2);
            uint8_t* chr_cvt = m_slice_vector[index].oam[n].chr_export.data();
            memset(chr_cvt, 0, sprite_height*2);
            for (int y = 0; y < sprite_height; ++y)
            {
                int yline = y + m_slice_vector[index].oam[n].y;
                if (yline < 0 || yline > cut.height())
                    continue;
                const uint32_t* scan_line = (const uint32_t*)cut.scanLine(yline);
                uint8_t* bit0 = y < 8 ? chr_cvt+y : chr_cvt+8+y;
                uint8_t* bit1 = y < 8 ? chr_cvt+8+y : chr_cvt+16+y;
                uint8_t mask = 0x80;
                for (int x = 0; x < 8; ++x)
                {
                    int xp = x + m_slice_vector[index].oam[n].x;
                    if (xp < 0 || xp > cut.width() || scan_line[xp] == m_bg_color)
                    {
                        mask >>=1;
                        continue;
                    }
                    uint32_t color = scan_line[xp];
                    if (color == palette[pal.c[1]])
                        *bit0 |= mask;
                    else if (color == palette[pal.c[2]])
                        *bit1 |= mask;
                    else if (color == palette[pal.c[3]])
                    {
                        *bit0 |= mask;
                        *bit1 |= mask;
                    } else if (m_slice_vector[index].oam[n].mode != 0)
                    {
                        //Find any Color
                        int64_t best_diff = INT64_MAX;
                        int best_c = 1;
                        for (int c = 1; c < 4; ++c)
                        {
                            int dr = (color & 0xFF) - (palette[pal.c[c]] & 0xFF);
                            int dg = ((color >> 8) & 0xFF) -  ((palette[pal.c[c]] >> 8) & 0xFF);
                            int db = ((color >> 16) & 0xFF) - ((palette[pal.c[c]] >> 16) & 0xFF);
                            int64_t diff = dr*dr + dg*dg + db*db;
                            if (diff < best_diff)
                            {
                                best_diff = diff;
                                best_c = c;
                            }
                        }
                        if (best_c==1)
                            *bit0 |= mask;
                        else if (best_c==2)
                            *bit1 |= mask;
                        else if (best_c==3)
                        {
                            *bit0 |= mask;
                            *bit1 |= mask;
                        }
                    }
                    mask >>=1;
                }
            }
        }
    }
}


void MainWindow::Export_Buffer(std::stringstream &stream, const void* data, size_t size)
{
    int pos = 0;
    for (size_t n = 0; n < size; ++n)
    {
        if (pos >= 16)
        {
            stream << std::endl;
            pos = 0;
        }
        if (pos == 0)
            stream << "    .db ";
        else
            stream << ", ";
        stream << "$" << std::hex << std::setw(2) << std::setfill('0') << ((int)(((const uint8_t*)data)[n]) & 0xFF);
        pos ++;
    }
}

void MainWindow::on_pushButton_export_test_clicked()
{
    if (m_spriteset_original.isNull())
        return;
    Image2Index(m_spriteset_original, m_spriteset_index);
    Index2Image(m_spriteset_index, m_spriteset_indexed, m_spriteset_original.width(), m_spriteset_original.height());
    Export_SpriteConvert();

    std::string unit_name = "jax";

    std::stringstream stream;
    stream << unit_name << "_main_low:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db low(" << unit_name << "_main_oam_" << std::dec << index << ")" << std::endl;
    stream << unit_name << "_main_high:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db high(" << unit_name << "_main_oam_" << std::dec << index << ")" << std::endl;

    stream << unit_name << "_det_low:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db low(" << unit_name << "_det_oam_" << std::dec << index << ")" << std::endl;
    stream << unit_name << "_det_high:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db high(" << unit_name << "_det_oam_" << std::dec << index << ")" << std::endl;

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        uint32_t cnt = 0;
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
        {
            if (m_slice_vector[index].oam[n].mode)
                cnt ++;
        }
        stream << unit_name << "_main_oam_" << std::dec << index << ":" << std::endl;
        stream << "    .db " << std::dec << cnt << " ;main oam count" << std::endl;
        for (int n = m_slice_vector[index].oam.size()-1; n >= 0 ; --n)
        {
            if (m_slice_vector[index].oam[n].mode == 0)
                continue;
            int y = 200 - m_slice_vector[index].height + m_slice_vector[index].oam[n].y;
            stream << "    .db $" << std::hex << std::setw(2) << std::setfill('0') << (y&0xFF);     //y
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (n * 2 | 1);            //index
            int attr = m_slice_vector[index].oam[n].palette;
            if (attr == 2)
                attr = 3;
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (attr); //attribute
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (m_slice_vector[index].oam[n].x+100);   //x
            stream << std::endl;
        }
    }

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        uint32_t cnt = 0;
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
        {
            if (m_slice_vector[index].oam[n].mode == 0)
                cnt ++;
        }
        stream << unit_name << "_det_oam_" << std::dec << index << ":" << std::endl;
        stream << "    .db " << std::dec << cnt << " ;detailed oam count" << std::endl;
        for (int n = m_slice_vector[index].oam.size()-1; n >= 0 ; --n)
        {
            if (m_slice_vector[index].oam[n].mode != 0)
                continue;
            int y = 200 - m_slice_vector[index].height + m_slice_vector[index].oam[n].y;
            stream << "    .db $" << std::hex << std::setw(2) << std::setfill('0') << (y&0xFF);     //y
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (n * 2 | 1);            //index
            int attr = m_slice_vector[index].oam[n].palette;
            if (attr == 2)
                attr = 3;
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (attr); //attribute
            stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (m_slice_vector[index].oam[n].x+100);   //x
            stream << std::endl;
        }
    }

    stream << unit_name << "_palette:" << std::endl;
    for (size_t n = 0; n < 4; ++n)
    {
        //stream << "    .db $" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)(m_palette_set[n].c[0]);
        stream << "    .db $21";
        stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)(m_palette_set[n].c[1]);
        stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)(m_palette_set[n].c[2]);
        stream << ", $" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)(m_palette_set[n].c[3]);
        stream << std::endl;
    }

    {
        QFile file(QString("%1.inc").arg(unit_name.c_str()));
        file.open(QFile::WriteOnly);
        file.write(stream.str().c_str(), stream.str().length());
        file.close();
    }

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        std::vector<uint8_t> chr;
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
            chr.insert(chr.end(), m_slice_vector[index].oam[n].chr_export.begin(), m_slice_vector[index].oam[n].chr_export.end());
        chr.resize(1024, 0xFF);
        QString fname = QString("%1_%2.bin").arg(unit_name.c_str()).arg(index);

        QFile file(fname);
        file.open(QFile::WriteOnly);
        file.write((const char*)chr.data(), chr.size());
        file.close();
    }
}




