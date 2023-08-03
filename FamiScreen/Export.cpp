#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string.h>
#include <memory.h>
#include <iomanip>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

struct Entry
{
    uint8_t header;
    std::vector<uint8_t> data;
    uint16_t outpos;

    Entry(uint8_t h) : header(h) {}
    Entry(uint8_t h, const std::vector<uint8_t> &d) : header(h), data(d) {}
};

void Flush(const std::vector<uint8_t>& buf, std::vector<Entry>& pack)
{
    if (buf.size() == 1)
    {
        if (buf[0] == 0x00)
        {
            pack.push_back(Entry(1 | 0x40));
        }
        else if (buf[0] == 0xFF)
        {
            pack.push_back(Entry(1 | 0x80));
        }
        else
        {
            pack.push_back(Entry(1 | 0xC0, buf));
        }
    } else if (buf.size() == 2 && buf[0] == buf[1])
    {
        if (buf[0] == 0x00)
        {
            pack.push_back(Entry(2 | 0x40));
        } else if (buf[0] == 0xFF)
        {
            pack.push_back(Entry(2 | 0x80));
        }
        else
        {
            std::vector<uint8_t> v;
            v.push_back(buf[0]);
            pack.push_back(Entry(0xC2, v));
        }
    }
    else
    {
        pack.push_back(Entry(buf.size(), buf));
    }
}

void FlushEq(uint8_t val, uint8_t count, std::vector<Entry>& pack)
{
    if (val == 0x00)
    {
        pack.push_back(Entry(count | 0x40));
    } else if (val == 0xFF)
    {
        pack.push_back(Entry(count | 0x80));
    }
    else
    {
        std::vector<uint8_t> v;
        v.push_back(val);
        pack.push_back(Entry(count | 0xC0, v));
    }
}

std::vector<uint8_t> RlePack(const std::vector<uint8_t>& buffer, bool rle4)
{
    std::vector<Entry> pack;
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
        if (count > 2)
        {
            //Flush
            if (buf.size() > 0)
            {
                Flush(buf, pack);
                buf.resize(0);
            }
            FlushEq(src[i], count, pack);
            i += count;
        }
        else
        {
            buf.push_back(src[i]);
            //Flush
            if (buf.size() >= 63)
            {
                Flush(buf, pack);
                buf.resize(0);
            }
            ++i;
        }
    }
    if (buf.size() > 0)
    {
        Flush(buf, pack);
        buf.resize(0);
    }
    //pack.push_back(0);
    //return pack;

    //convert to buffer, apply copy method
    std::vector<uint8_t> result;
    for (size_t i = 0; i < pack.size(); ++i)
    {
        pack[i].outpos = result.size();
        uint8_t type = pack[i].header & 0xC0;
        if (type == 0 && pack[i].data.size() > 2)
        {
            size_t eq_id = i;
            if (rle4)
            {
                for (size_t n = 0; n < i; ++n)
                {
                    if (pack[i].header != pack[n].header)
                        continue;
                    if (memcmp(pack[i].data.data(), pack[n].data.data(), pack[i].data.size()) == 0)
                    {
                        eq_id = n;
                        break;
                    }
                }
            }
            if (eq_id == i)
            {
                result.push_back(pack[i].header);
                result.insert(result.end(), pack[i].data.begin(), pack[i].data.end());
            } else
            {
                result.push_back(0x40);
                result.push_back(pack[eq_id].outpos & 0xFF);
                result.push_back(pack[eq_id].outpos >> 8);
                pack[i].header = 0;
            }
        } else
        {
            result.push_back(pack[i].header);
            if (pack[i].data.size())
                result.insert(result.end(), pack[i].data.begin(), pack[i].data.end());
        }
    }
    result.push_back(0);
    return result;
}

void MainWindow::on_pushButton_export_clicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Asm include file", QDir::currentPath(), "*.inc");
    if (file_name.isEmpty())
        return;

    RedrawAttributeTab();

    ECompression compresion = (ECompression)ui->comboBox_compression->itemData(ui->comboBox_compression->currentIndex()).toInt();
    EChrAlign align = (EChrAlign)ui->comboBox_chr_align->itemData(ui->comboBox_chr_align->currentIndex()).toInt();

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

    std::map<std::vector<uint8_t>, int> tile_map;
    std::vector<uint8_t> tile_vector;
    std::vector<uint8_t> tile_vector_blink;
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

    for (size_t n = 0; n < 32*30; ++n)
    {
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
        auto itt = tile_map.find(tile);
        if (itt == tile_map.end())
        {
            int index = (int)tile_vector.size() / 16;
            tile_map.insert(std::make_pair(tile, index));
            if (ui->checkBox_two_frames_blinking->isChecked())
            {
                tile_vector.insert(tile_vector.end(), tile.begin(), tile.begin()+16);
                tile_vector_blink.insert(tile_vector_blink.end(), tile.begin()+16, tile.begin()+32);
            } else
                tile_vector.insert(tile_vector.end(), tile.begin(), tile.end());
            nametable[n] = index;
        } else
            nametable[n] = itt->second;
        size_t y = n / 32;
        size_t x = n % 32;
        uint8_t attr = m_screen_attribute[(y/2)*16 + (x/2)];
        if ((x/2) % 2 == 1)
            attr <<= 2;
        if ((y/2) % 2 == 1)
            attr <<= 4;
        nametable[960 + (y/4)*8 + x/4] |= attr;
    }


    if (compresion != Compression_None)
    {
        std::vector<uint8_t> rle = RlePack(nametable, compresion == Compression_Rle4);
        nametable = rle;
    }

    {
        std::stringstream stream;
        stream << ui->lineEdit_name->text().toStdString() << ":  ;" << std::dec << nametable.size() << " bytes" << std::endl;
        int pos = 0;
        for (size_t n = 0; n < nametable.size(); ++n)
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
            stream << "$" << std::hex << std::setw(2) << std::setfill('0') << ((int)(nametable[n] & 0xFF));
            pos ++;
        }

        QFile file(file_name);
        file.open(QFile::WriteOnly);
        file.write(stream.str().c_str(), stream.str().length());
        file.close();
    }

    QString chr_file_name = QFileDialog::getSaveFileName(this, "Character binary file", QDir::currentPath(), "*.chr");
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
        tile_vector.resize((tile_vector.size() + align_size - 1) & (~(align_size - 1)), 0xFF);
        if (ui->checkBox_two_frames_blinking->isChecked())
            tile_vector_blink.resize((tile_vector_blink.size() + align_size - 1) & (~(align_size - 1)), 0xFF);
    }

    {
        QFile file(chr_file_name);
        file.open(QFile::WriteOnly);
        file.write((const char*)tile_vector.data(), tile_vector.size());
        if (ui->checkBox_two_frames_blinking->isChecked())
            file.write((const char*)tile_vector_blink.data(), tile_vector_blink.size());
        file.close();
    }

    /*
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
    }*/
}




