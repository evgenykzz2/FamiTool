#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFile>
#include <QMessageBox>
#include <iomanip>

#define BLOCK_CNTA 12
//                                          14   30   48       68     92     118
static const int s_blockA_x[BLOCK_CNTA] = {0,25, 0,24, 0,22,   0,21,  0,20,  0,19  };
static const int s_blockA_y[BLOCK_CNTA] = {6,6,  7,7,  8,8,    9,9,   10,10, 11,11 };
static const int s_blockA_l[BLOCK_CNTA] = {7,7,  8,8,  10,10,  11,11, 12,12, 13,13 };

#define BLOCK_CNTB 6
//                                        32  64      90     112
static const int s_blockB_x[BLOCK_CNTB] = {0,  0,   0,19,   0,21  };
static const int s_blockB_y[BLOCK_CNTB] = {12, 13,  14,14,  15,15 };
static const int s_blockB_l[BLOCK_CNTB] = {32, 32,  13,13,  11,11 };

#define BLOCK_CNTC 10
//
static const int s_blockC_x[BLOCK_CNTC] = {0,23,   0,25,  0,27,   0,29,   0,31  };
static const int s_blockC_y[BLOCK_CNTC] = {16,16,  17,17, 18,18, 19,19,  20,20 };
static const int s_blockC_l[BLOCK_CNTC] = {9,9,    7,7,   5,5,   3,3,    1,1   };

void MainWindow::EncodePenguinBlock(const QImage* frame, int block_count, const int* block_x, const int* block_y, const int* block_l,
                               std::vector<uint8_t> &chr, std::vector<uint8_t> &nametable)
{
    int pal = 0;
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());

    for (int f = 0; f < 4; ++f)
    {
        int tile_index = 0;
        for (int block = 0; block < block_count; ++block)
        {
            for (int n = 0; n < block_l[block]; ++n)
            {
                std::vector<uint8_t> tile(16, 0);
                int bx = block_x[block] + n;
                int by = block_y[block];
                for (int yi = 0; yi < 8; ++yi)
                {
                    const uint8_t* src_line = frame[f].scanLine(by*8 + yi) + bx*8*4;
                    for (int xi = 0; xi < 8; ++xi)
                    {
                        uint32_t color = ((const uint32_t*)src_line)[xi] & 0xFFFFFF;
                        int color_index = -1;
                        auto itt = m_palette_cvt_rule.find(color);
                        if (itt != m_palette_cvt_rule.end())
                        {
                            int c = itt->second;
                                color_index = 0;
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[1])
                                color_index = 1;
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[2])
                                color_index = 2;
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[3])
                                color_index = 3;
                        }

                        if (color_index < 0)
                        {
                            int64_t best_diff = INT64_MAX;
                            for (int c = 0; c < 4; ++c)
                            {
                                int ci = m_palette_set[pal].c[c];
                                int dr = (int)(color & 0xFF) - (int)(palette[ci] & 0xFF);
                                int dg = (int)((color >> 8) & 0xFF) - (int)((palette[ci] >> 8)  & 0xFF);
                                int db = (int)((color >> 16)& 0xFF) - (int)((palette[ci] >> 16) & 0xFF);
                                int64_t d = dr*dr + dg*dg + db*db;
                                if (d < best_diff)
                                {
                                    color_index = c;
                                    best_diff = d;
                                }
                            }
                        }

                        if (color_index == 1)
                        {
                            tile[yi    ] |= 0x80 >> xi;
                        } else if (color_index == 2)
                        {
                            tile[yi + 8] |= 0x80 >> xi;
                        } else if (color_index == 3)
                        {
                            tile[yi    ] |= 0x80 >> xi;
                            tile[yi + 8] |= 0x80 >> xi;
                        }
                    }
                }
                chr.insert(chr.end(), tile.begin(), tile.end());
                nametable[by*32+bx] = tile_index;
                tile_index++;
            }
        }

        //next frame -> allign 2K
        size_t size = (chr.size() + 2*1024 - 1) & 0xF800;
        chr.resize(size, 0xFF);
    }
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

void MainWindow::on_btn_penguin_export_clicked()
{
    QImage frame[4];
    frame[0].load(m_spriteset_file_name);
    frame[1].load(ui->lineEdit_animation_browse2->text());
    frame[2].load(ui->lineEdit_animation_browse3->text());
    frame[3].load(ui->lineEdit_animation_browse4->text());

    for (int i = 0; i < 4; ++i)
    {
        if (frame[i].isNull())
        {
            QMessageBox::critical(0, "Error", "Invalid image");
            return;
        }
        if (frame[i].width() != 256 || frame[i].height() != 240)
        {
            QMessageBox::critical(0, "Error", "Invalid image size");
            return;
        }
    }

    std::vector<uint8_t> nametable(1024, 0x00);
    memset(nametable.data(), 0x80, 960);
    std::vector<uint8_t> chr0;
    EncodePenguinBlock(frame, BLOCK_CNTA, s_blockA_x, s_blockA_y, s_blockA_l, chr0, nametable);

    std::vector<uint8_t> chr1;
    EncodePenguinBlock(frame, BLOCK_CNTB, s_blockB_x, s_blockB_y, s_blockB_l, chr1, nametable);

    std::vector<uint8_t> chr2;
    EncodePenguinBlock(frame, BLOCK_CNTC, s_blockC_x, s_blockC_y, s_blockC_l, chr2, nametable);


    QString chr_file_name = QFileDialog::getSaveFileName(this, "Character binary file", "", "*.chr");
    if (chr_file_name.isEmpty())
        return;

    {
        QFile file(chr_file_name + "-0");
        file.open(QFile::WriteOnly);
        file.write((const char*)chr0.data(), chr0.size());
        //file.write((const char*)chr1.data(), chr1.size());
        //file.write((const char*)chr2.data(), chr2.size());
        file.close();
    }

    {
        QFile file(chr_file_name + "-1");
        file.open(QFile::WriteOnly);
        //file.write((const char*)chr0.data(), chr0.size());
        file.write((const char*)chr1.data(), chr1.size());
        //file.write((const char*)chr2.data(), chr2.size());
        file.close();
    }

    {
        QFile file(chr_file_name + "-2");
        file.open(QFile::WriteOnly);
        //file.write((const char*)chr0.data(), chr0.size());
        //file.write((const char*)chr1.data(), chr1.size());
        file.write((const char*)chr2.data(), chr2.size());
        file.close();
    }


    QString file_name = QFileDialog::getSaveFileName(this, "Asm include file", "", "*.inc");
    if (file_name.isEmpty())
        return;

    {
        std::stringstream stream;
        stream << ui->lineEdit_name->text().toStdString() << "_nt:  ;" << std::dec << nametable.size() << " bytes" << std::endl;
        PrintBuffer(stream, nametable);

        QFile file(file_name);
        file.open(QFile::WriteOnly);
        file.write(stream.str().c_str(), stream.str().length());
        file.close();
    }
}





