#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string.h>
#include <memory.h>
#include <iomanip>
#include <QFile>
#include <QFileDialog>

void MainWindow::Export_SpriteConvert()
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
    int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
    bool blink = ui->checkBox_palette_blink->isChecked();

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        QImage cut = m_spriteset_indexed.copy(m_slice_vector[index].x, m_slice_vector[index].y, m_slice_vector[index].width, m_slice_vector[index].height);
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
        {
            int pindex = m_slice_vector[index].oam[n].palette;
            Palette pal = m_palette_set[pindex];
            m_slice_vector[index].oam[n].chr_export.resize(sprite_height*2);
            memset(m_slice_vector[index].oam[n].chr_export.data(), 0, m_slice_vector[index].oam[n].chr_export.size());
            uint8_t* chr_cvt = m_slice_vector[index].oam[n].chr_export.data();
            memset(chr_cvt, 0, sprite_height*2);
            m_slice_vector[index].oam[n].chr_export_blink.resize(sprite_height*2);
            memset(m_slice_vector[index].oam[n].chr_export_blink.data(), 0, m_slice_vector[index].oam[n].chr_export_blink.size());
            uint8_t* chr_blink_cvt = m_slice_vector[index].oam[n].chr_export_blink.data();
            memset(chr_blink_cvt, 0, sprite_height*2);
            for (int y = 0; y < sprite_height; ++y)
            {
                int yline = y + m_slice_vector[index].oam[n].y;
                if (yline < 0 || yline >= cut.height())
                    continue;
                const uint32_t* scan_line = (const uint32_t*)cut.scanLine(yline);
                uint8_t* bit0 = y < 8 ? chr_cvt+y : chr_cvt+8+y;
                uint8_t* bit1 = y < 8 ? chr_cvt+8+y : chr_cvt+16+y;
                uint8_t* bitb0 = y < 8 ? chr_blink_cvt+y : chr_blink_cvt+8+y;
                uint8_t* bitb1 = y < 8 ? chr_blink_cvt+8+y : chr_blink_cvt+16+y;
                uint8_t mask = 0x80;
                for (int x = 0; x < 8; ++x)
                {
                    int xp = x + m_slice_vector[index].oam[n].x;
                    if (xp < 0 || xp >= cut.width() || scan_line[xp] == m_bg_color)
                    {
                        mask >>=1;
                        continue;
                    }
                    uint32_t color = scan_line[xp];
                    if (blink)
                    {
                        int found = -1;
                        for (int i = 0; i < 16; ++i)
                        {
                            if (!m_blink_palette.enable[pindex*16+i])
                                continue;
                            if (color == m_blink_palette.color[pindex*16+i])
                            {
                                found = i;
                                break;
                            }
                        }
                        if (found < 0 && m_slice_vector[index].oam[n].mode != 0)
                        {
                            //Find any Color
                            int64_t best_diff = INT64_MAX;
                            for (int c = 0; c < 16; ++c)
                            {
                                if (!m_blink_palette.enable[pindex*16+c])
                                    continue;
                                int dr = (color & 0xFF) - (m_blink_palette.color[pindex*16+c] & 0xFF);
                                int dg = ((color >> 8) & 0xFF) -  ((m_blink_palette.color[pindex*16+c] >> 8) & 0xFF);
                                int db = ((color >> 16) & 0xFF) - ((m_blink_palette.color[pindex*16+c] >> 16) & 0xFF);
                                int64_t diff = dr*dr + dg*dg + db*db;
                                if (diff < best_diff)
                                {
                                    best_diff = diff;
                                    found = c;
                                }
                            }
                        }
                        if (found >= 0)
                        {
                            uint8_t c0 = found / 4;
                            uint8_t c1 = found % 4;
                            if ((m_slice_vector[index].oam[n].y + y) % 2 != 0)
                            {
                                uint8_t tmp = c0;
                                c0 = c1;
                                c1 = tmp;
                            }
                            if (c0 == 1)
                                *bit0 |= mask;
                            else if (c0 == 2)
                                *bit1 |= mask;
                            else if (c0 == 3)
                            {
                                *bit0 |= mask;
                                *bit1 |= mask;
                            }

                            if (c1 == 1)
                                *bitb0 |= mask;
                            else if (c1 == 2)
                                *bitb1 |= mask;
                            else if (c1 == 3)
                            {
                                *bitb0 |= mask;
                                *bitb1 |= mask;
                            }
                        }
                    } else
                    {
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
    stream << unit_name << "_oam_low:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db low(" << unit_name << "_oam_" << std::dec << index << ")" << std::endl;
    stream << unit_name << "_oam_high:" << std::endl;
    for (size_t index = 0; index < m_slice_vector.size(); ++index)
        stream << "    .db high(" << unit_name << "_oam_" << std::dec << index << ")" << std::endl;

    //stream << unit_name << "_det_low:" << std::endl;
    //for (size_t index = 0; index < m_slice_vector.size(); ++index)
    //    stream << "    .db low(" << unit_name << "_det_oam_" << std::dec << index << ")" << std::endl;
    //stream << unit_name << "_det_high:" << std::endl;
    //for (size_t index = 0; index < m_slice_vector.size(); ++index)
    //    stream << "    .db high(" << unit_name << "_det_oam_" << std::dec << index << ")" << std::endl;

    for (size_t index = 0; index < m_slice_vector.size(); ++index)
    {
        stream << unit_name << "_oam_" << std::dec << index << ":" << std::endl;
        stream << "    .db " << std::dec << m_slice_vector[index].oam.size() << " ;oam count" << std::endl;
        for (int n = m_slice_vector[index].oam.size()-1; n >= 0 ; --n)
        {
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
        if (ui->checkBox_palette_blink->isChecked())
        {
            std::vector<uint8_t> chr;
            for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
                chr.insert(chr.end(), m_slice_vector[index].oam[n].chr_export_blink.begin(), m_slice_vector[index].oam[n].chr_export_blink.end());
            chr.resize(1024, 0xFF);
            QString fname = QString("%1_%2_blink.bin").arg(unit_name.c_str()).arg(index);
            QFile file(fname);
            file.open(QFile::WriteOnly);
            file.write((const char*)chr.data(), chr.size());
            file.close();
        }
    }
}


void MainWindow::on_btn_export_clicked()
{
    if (m_spriteset_original.isNull())
        return;
    Image2Index(m_spriteset_original, m_spriteset_index);
    Index2Image(m_spriteset_index, m_spriteset_indexed, m_spriteset_original.width(), m_spriteset_original.height());
    Export_SpriteConvert();

    ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
    //int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
    //bool blink = ui->checkBox_palette_blink->isChecked();

    int oam_offset = ui->combo_export_offset->itemData(ui->combo_export_offset->currentIndex()).toInt();

    std::stringstream stream;
    std::vector<uint8_t> chrset;
    std::map<std::vector<uint8_t>, int> chr_map;
    for (size_t slice = 0; slice < m_slice_vector.size(); ++slice)
    {
        if (m_slice_vector[slice].oam.empty())
            continue;

        stream << m_slice_vector[slice].caption.toStdString() << ":" << std::endl;
        stream << "    .db " << std::dec << m_slice_vector[slice].oam.size() << std::endl;

        for (size_t i = 0; i < m_slice_vector[slice].oam.size(); ++i)
        //for (int i =  m_slice_vector[slice].oam.size()-1; i >= 0; --i)
        {
            std::vector<uint8_t> bin = m_slice_vector[slice].oam[i].chr_export;
            std::vector<uint8_t> bin_v(bin.size());
            if (sprite_mode == SpriteMode_8x8)
            {
                for (size_t n = 0; n < 8; ++n)
                {
                    bin_v[n]   = bin[7-n];
                    bin_v[8+n] = bin[8+7-n];
                }
            } else
            {
                for (size_t n = 0; n < 8; ++n)
                {
                    bin_v[n]   = bin[16+7-n];
                    bin_v[8+n] = bin[16+8+7-n];
                    bin_v[16+n]   = bin[7-n];
                    bin_v[16+8+n] = bin[8+7-n];
                }
            }

            std::vector<uint8_t> bin_h(bin.size(), 0);
            std::vector<uint8_t> bin_hv(bin.size(), 0);
            for (size_t n = 0; n < bin.size(); ++n)
            {
                for (size_t b = 0; b < 8; ++b)
                {
                    if ((uint8_t)(bin[n] & (1 << b)) != 0)
                        bin_h[n] |= 0x80 >> b;
                    if ((uint8_t)(bin_v[n] & (1 << b)) != 0)
                        bin_hv[n] |= 0x80 >> b;
                }
            }


            int index = -1;
            uint8_t flag = 0;
            auto itt = chr_map.find(bin);
            if (itt != chr_map.end())
                index = itt->second;
            else
            {
                itt = chr_map.find(bin_h);
                if (itt != chr_map.end())
                {
                    index = itt->second;
                    flag |= 0x40;
                } else
                {
                    itt = chr_map.find(bin_v);
                    if (itt != chr_map.end())
                    {
                        index = itt->second;
                        flag |= 0x80;
                    } else
                    {
                        itt = chr_map.find(bin_hv);
                        if (itt != chr_map.end())
                        {
                            index = itt->second;
                            flag |= 0x80 | 0x40;
                        } else
                        {
                            index = chr_map.size();
                            chr_map.insert(std::make_pair(bin, index));
                            chrset.insert(chrset.end(), bin.begin(), bin.end());
                        }
                    }
                }
            }

            flag |= m_slice_vector[slice].oam[i].palette;
            if (m_slice_vector[slice].oam[i].behind_background)
                flag |= 0x20;

            if (sprite_mode == SpriteMode_8x8)
            {
                index = (index + (oam_offset >> 4)) & 0xFF;
            } else if (sprite_mode == SpriteMode_8x16)
            {
                index *= 2;
                index = (index + (oam_offset >> 4)) & 0xFF;
                if (oam_offset >= 0x1000)
                    index += 1;
            }
            stream << "    .db ";
            stream << "$" << std::hex << std::setw(2) << std::setfill('0') << (((uint32_t)m_slice_vector[slice].oam[i].y + m_slice_vector[slice].dy) & 0xFF) << ", ";
            stream << "$" << std::hex << std::setw(2) << std::setfill('0') << (((uint32_t)index) & 0xFF) << ", ";
            stream << "$" << std::hex << std::setw(2) << std::setfill('0') << (((uint32_t)flag) & 0xFF) << ", ";
            stream << "$" << std::hex << std::setw(2) << std::setfill('0') << (((uint32_t)m_slice_vector[slice].oam[i].x + m_slice_vector[slice].dx) & 0xFF) << std::endl;
        }
        stream << std::endl;
    }


    int align_size = ui->combo_export_align_chr->itemData(ui->combo_export_align_chr->currentIndex()).toInt();
    chrset.resize((chrset.size() + align_size - 1) & (~(align_size - 1)), 0xFF);


    {
        QString file_name = QFileDialog::getSaveFileName(this, "Asm include file", "", "*.inc");
        if (file_name.isEmpty())
            return;

        QFile file(file_name);
        file.open(QFile::WriteOnly);
        file.write(stream.str().c_str(), stream.str().length());
        file.close();
    }

    {
        QString file_name = QFileDialog::getSaveFileName(this, "Character binary file", "", "*.chr");
        if (file_name.isEmpty())
            return;

        QFile file(file_name);
        file.open(QFile::WriteOnly);
        file.write((const char*)chrset.data(), chrset.size());
        file.close();
    }
}



















