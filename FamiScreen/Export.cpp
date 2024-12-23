#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string.h>
#include <memory.h>
#include <map>
#include <iomanip>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <qdebug.h>

static void Flush(std::vector<uint8_t>& buf, std::vector<uint8_t>& pack)
{
    if (buf.empty())
        return;

    if (buf.size() == 1)
    {
        if (buf[0] == 0x00)
        {
            pack.push_back(1 | 0x40);
        }
        else if (buf[0] == 0xFF)
        {
            pack.push_back(1 | 0x80);
        }
        else
        {
            pack.push_back(1 | 0xC0);
            pack.push_back(buf[0]);
        }
    }
    else if (buf.size() == 2 && buf[0] == buf[1])
    {
        if (buf[0] == 0x00)
        {
            pack.push_back(2 | 0x40);
        }
        else if (buf[0] == 0xFF)
        {
            pack.push_back(2 | 0x80);
        }
        else
        {
            pack.push_back(0xC2);
            pack.push_back(buf[0]);
        }
    }
    else
    {
        pack.push_back((uint8_t)buf.size());
        for (size_t j = 0; j < buf.size(); ++j)
            pack.push_back(buf[j]);
    }
    buf.resize(0);
}

static void FlushEq(uint8_t val, uint8_t count, std::vector<uint8_t>& pack)
{
    if (val == 0x00)
    {
        pack.push_back(count | 0x40);
    }
    else if (val == 0xFF)
    {
        pack.push_back(count | 0x80);
    }
    else
    {
        pack.push_back(count | 0xC0);
        pack.push_back(val);
    }
}

//Rle3 Original
std::vector<uint8_t> Rle3Encode(const std::vector<uint8_t>& buffer)
{
    std::vector<uint8_t> pack;
    std::vector<uint8_t> buf;
    const uint8_t* src = buffer.data();
    for (size_t i = 0; i < buffer.size(); )
    {
        //look forward
        size_t count = 1;
        for (size_t j = 1; j < 63; ++j)
        {
            if (i + j >= buffer.size())
                break;
            if (src[i] != src[i + j])
                break;
            count = j + 1;
        }
        if (count > 1 && (src[i] == 0x00 || src[i] == 0xFF))
        {
            Flush(buf, pack);
            FlushEq(src[i], (uint8_t)count, pack);
            i += count;
        }
        else if (count == 1 && (src[i] == 0x00 || src[i] == 0xFF) && buf.empty())
        {
            Flush(buf, pack);
            FlushEq(src[i], (uint8_t)count, pack);
            i += count;
        }
        else if (count == 2 && buf.empty())
        {
            Flush(buf, pack);
            FlushEq(src[i], (uint8_t)count, pack);
            i += count;
        }
        else if (count > 2)
        {
            Flush(buf, pack);
            FlushEq(src[i], (uint8_t)count, pack);
            i += count;
        }
        else
        {
            buf.push_back(src[i]);
            //Flush
            if (buf.size() >= 63)
            {
                Flush(buf, pack);
            }
            ++i;
        }
    }
    Flush(buf, pack);
    pack.push_back(0);
    return pack;
}

static void PrintBuffer(std::stringstream &stream, std::vector<uint8_t>& buffer)
{
    int pos = 0;
    for (size_t n = 0; n < buffer.size(); ++n)
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
        stream << "$" << std::hex << std::setw(2) << std::setfill('0') << ((int)(buffer[n] & 0xFF));
        pos ++;
    }
}

void MainWindow::on_pushButton_export_clicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Asm include file", "", "*.inc");
    if (file_name.isEmpty())
        return;

    RedrawAttributeTab();
    RedrawOamTab();

    ECompression compresion = (ECompression)ui->comboBox_compression->itemData(ui->comboBox_compression->currentIndex()).toInt();
    EChrAlign align = (EChrAlign)ui->comboBox_chr_align->itemData(ui->comboBox_chr_align->currentIndex()).toInt();
    int tiles_offset = ui->lineEdit_export_tiles_offset->text().toInt();

    ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();

    if (m_screen_tiles.size() != 32*30*16 ||
        m_screen_attribute.size() != 16*16)
    {
        QMessageBox::critical(0, "Error", "Something went wrong");
        return;
    }

    if (ui->checkBox_two_frames_blinking->isChecked())
    {
        if (m_screen_tiles_blink.size() != 32*30*16)
        {
            QMessageBox::critical(0, "Error", "Something went wrong");
            return;
        }
    }

    std::map<std::vector<uint8_t>, int> tile_map[4];
    std::vector<uint8_t> tile_vector[4];
    std::vector<uint8_t> tile_vector_blink[4];
    std::vector<uint8_t> nametable(1024, 0);

    std::vector<uint8_t> zero_tile;
    if (ui->checkBox_two_frames_blinking->isChecked())
    {
        //zero_tile.resize(32, 0x00);
        //tile_map.insert(std::make_pair(zero_tile, 0));
        //tile_vector.insert(tile_vector.end(), zero_tile.begin(), zero_tile.begin()+16);
        //tile_vector_blink.insert(tile_vector_blink.end(), zero_tile.begin()+16, zero_tile.begin()+32);
    } else
    {
        //zero_tile.resize(16, 0x00);
        //tile_map.insert(std::make_pair(zero_tile, 0));
        //tile_vector.insert(tile_vector.end(), zero_tile.begin(), zero_tile.end());
    }

    int irq0 = ui->edit_irq_0->text().toInt();
    int irq1 = ui->edit_irq_1->text().toInt();

    for (size_t y = 0; y < 30; ++y)
    {
        size_t slot = 0;
        if (irq0 != 0 && irq1 == 0)
        {
            if (y*8 < irq0)
                slot = 0;
            else
                slot = 1;
        } else if (irq0 != 0 && irq1 != 0)
        {
            if (y*8 < irq0)
                slot = 0;
            else if (y*8 < irq1)
                slot = 1;
            else
                slot = 2;
        }

        for (size_t x = 0; x < 32; ++x)
        {
            size_t n = x + y*32;
            std::vector<uint8_t> tile;
            if (ui->checkBox_two_frames_blinking->isChecked())
            {
                tile.resize(32);
                memcpy(tile.data()+00, m_screen_tiles.data() + n*16, 16);
                memcpy(tile.data()+16, m_screen_tiles_blink.data() + n*16, 16);
            } else
            {
                tile.resize(16);
                memcpy(tile.data(), m_screen_tiles.data() + n*16, 16);
            }
            auto itt = tile_map[slot].find(tile);
            if (itt == tile_map[slot].end())
            {
                int index = (int)tile_vector[slot].size() / 16;
                tile_map[slot].insert(std::make_pair(tile, index));
                if (ui->checkBox_two_frames_blinking->isChecked())
                {
                    tile_vector[slot].insert(tile_vector[slot].end(), tile.begin(), tile.begin()+16);
                    tile_vector_blink[slot].insert(tile_vector_blink[slot].end(), tile.begin()+16, tile.begin()+32);
                } else
                    tile_vector[slot].insert(tile_vector[slot].end(), tile.begin(), tile.end());
                nametable[n] = index == 0 ? 0 : index + tiles_offset;
            } else
                nametable[n] = itt->second == 0 ? 0 : itt->second + tiles_offset;

            uint8_t attr = m_screen_attribute[(y/2)*16 + (x/2)];
            if ((x/2) % 2 == 1)
                attr <<= 2;
            if ((y/2) % 2 == 1)
                attr <<= 4;
            nametable[960 + (y/4)*8 + x/4] |= attr;
        }
    }


    if (compresion != Compression_None)
    {
        std::vector<uint8_t> rle = Rle3Encode(nametable);
        nametable = rle;
    }

    std::vector<uint8_t> oam_raw;
    std::vector<uint8_t> oam_chr;
    std::vector<uint8_t> oam_chr_blink;
    std::map<std::vector<uint8_t>, int> oam_map;

    int sprite_base = ui->comboBox_export_sprite_offset->itemData(ui->comboBox_export_sprite_offset->currentIndex()).toInt();

    if (!m_oam_vector.empty())
    {
        for (size_t n = 0; n < m_oam_vector.size(); ++n)
        {
            std::vector<uint8_t> bin = m_oam_vector[n].chr_export;
            if (ui->checkBox_two_frames_blinking->isChecked())
                bin.insert(bin.end(), m_oam_vector[n].chr_export_blink.begin(), m_oam_vector[n].chr_export_blink.end());

            std::vector<uint8_t> bin_v(bin.size());
            if (sprite_mode == SpriteMode_8x8)
            {
                for (size_t n = 0; n < 8; ++n)
                {
                    bin_v[n]   = bin[7-n];
                    bin_v[8+n] = bin[8+7-n];
                    if (ui->checkBox_two_frames_blinking->isChecked())
                    {
                        bin_v[16+n] = bin[16+7-n];
                        bin_v[16+8+n] = bin[16+8+7-n];
                    }
                }
            } else
            {
                for (size_t n = 0; n < 8; ++n)
                {
                    bin_v[n]   = bin[16+7-n];
                    bin_v[8+n] = bin[16+8+7-n];
                    bin_v[16+n]   = bin[7-n];
                    bin_v[16+8+n] = bin[8+7-n];
                    if (ui->checkBox_two_frames_blinking->isChecked())
                    {
                        bin_v[32+n] = bin[32+16+7-n];
                        bin_v[32+8+n] = bin[32+16+8+7-n];
                        bin_v[32+16+n]   = bin[32+7-n];
                        bin_v[32+16+8+n] = bin[32+8+7-n];
                    }
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

            int index;
            uint8_t flag = 0;
            auto itt = oam_map.find(bin);
            if (itt != oam_map.end())
                index = itt->second;
            else
            {
                auto itt = oam_map.find(bin_h);
                if (itt != oam_map.end())
                {
                    index = itt->second;
                    flag |= 0x40;
                } else
                {
                    auto itt = oam_map.find(bin_v);
                    if (itt != oam_map.end())
                    {
                        index = itt->second;
                        flag |= 0x80;
                    } else
                    {
                        auto itt = oam_map.find(bin_hv);
                        if (itt != oam_map.end())
                        {
                            index = itt->second;
                            flag |= 0x80 | 0x40;
                        } else
                        {
                            index = oam_map.size();
                            oam_chr.insert(oam_chr.end(), bin.begin(), bin.end());
                            oam_map.insert(std::make_pair(bin, index));
                        }
                    }
                }
            }

            oam_raw.push_back(m_oam_vector[n].y-1);     //Y
            if (sprite_mode == SpriteMode_8x8)
                oam_raw.push_back(index + (sprite_base & 0xFF));      //Index
            else
            {
                index = index*2 + (sprite_base & 0xFF);
                if (sprite_base >= 0x100)
                    index ++;
                //index ++;  //Index +1 for upper bank
                oam_raw.push_back(index);
            }
            oam_raw.push_back(m_oam_vector[n].palette | flag); //flags (VHp000PP) p=priority(0=front, 1=back)
            oam_raw.push_back(m_oam_vector[n].x);       //X
        }
    }

    std::vector<uint8_t> palette_raw;
    {
        for (int p = 0; p < 4; ++p)
        {
            palette_raw.push_back(m_palette_set[p].c[0]);
            palette_raw.push_back(m_palette_set[p].c[1]);
            palette_raw.push_back(m_palette_set[p].c[2]);
            palette_raw.push_back(m_palette_set[p].c[3]);
        }
        for (int p = 0; p < 4; ++p)
        {
            palette_raw.push_back(m_palette_sprite[p].c[0]);
            palette_raw.push_back(m_palette_sprite[p].c[1]);
            palette_raw.push_back(m_palette_sprite[p].c[2]);
            palette_raw.push_back(m_palette_sprite[p].c[3]);
        }
    }

    {
        std::stringstream stream;
        if (ui->checkBox_export_nobg->isChecked() == false)
        {
            stream << ui->lineEdit_name->text().toStdString() << "_nt:  ;" << std::dec << nametable.size() << " bytes" << std::endl;
            PrintBuffer(stream, nametable);
        }

        if (!oam_raw.empty())
        {
            stream << std::endl;
            stream << ui->lineEdit_name->text().toStdString() << "_oam:  ;" << std::dec << oam_raw.size() << " bytes" << std::endl;
            PrintBuffer(stream, oam_raw);
        }

        stream << std::endl;
        stream << ui->lineEdit_name->text().toStdString() << "_palette:  ;" << std::dec << palette_raw.size() << " bytes" << std::endl;
        PrintBuffer(stream, palette_raw);

        QFile file(file_name);
        file.open(QFile::WriteOnly);
        file.write(stream.str().c_str(), stream.str().length());
        file.close();
    }

    QString chr_file_name = QFileDialog::getSaveFileName(this, "Character binary file", "", "*.chr");
    if (chr_file_name.isEmpty())
        return;

    size_t align_size = 0;
    if (align == ChrAlign_1K)
        align_size = 1*1024;
    else if (align == ChrAlign_2K)
        align_size = 2*1024;
    else if (align == ChrAlign_4K)
        align_size = 4*1024;
    else if (align == ChrAlign_8K)
        align_size = 8*1024;

    if (align_size != 0)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            if (tile_vector[i].size() != 0)
                tile_vector[i].resize((tile_vector[i].size() + align_size - 1) & (~(align_size - 1)), 0xFF);
            if (ui->checkBox_two_frames_blinking->isChecked() && tile_vector_blink[i].size() != 0)
                tile_vector_blink[i].resize((tile_vector_blink[i].size() + align_size - 1) & (~(align_size - 1)), 0xFF);
        }

        if (!oam_chr.empty())
        {
            oam_chr.resize((oam_chr.size() + align_size - 1) & (~(align_size - 1)), 0xFF);
            if (ui->checkBox_two_frames_blinking->isChecked())
                oam_chr_blink.resize((oam_chr_blink.size() + align_size - 1) & (~(align_size - 1)), 0xFF);
        }
    }

    {
        QFile file(chr_file_name);
        file.open(QFile::WriteOnly);
        if (ui->checkBox_export_nobg->isChecked() == false)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                if (tile_vector[i].size() != 0)
                    file.write((const char*)tile_vector[i].data(), tile_vector[i].size());
                if (ui->checkBox_two_frames_blinking->isChecked() && tile_vector_blink[i].size() != 0)
                    file.write((const char*)tile_vector_blink[i].data(), tile_vector_blink[i].size());
            }
        }

        if (!oam_chr.empty())
        {
            file.write((const char*)oam_chr.data(), oam_chr.size());
            if (ui->checkBox_two_frames_blinking->isChecked())
                file.write((const char*)oam_chr_blink.data(), oam_chr_blink.size());
        }
        file.close();
    }
}




