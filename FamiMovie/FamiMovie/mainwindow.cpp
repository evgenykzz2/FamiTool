#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFile>
#include <QMessageBox>
#include <QBitmap>
#include "picojson.h"
#include <set>
#include <memory>
#include <Windows.h>
#include <gdiplus.h>

#include "nQuant/DivQuantizer.h"
#include "nQuant/Dl3Quantizer.h"
#include "nQuant/EdgeAwareSQuantizer.h"
#include "nQuant/MedianCut.h"
#include "nQuant/MoDEQuantizer.h"
#include "nQuant/NeuQuantizer.h"
#include "nQuant/PnnLABQuantizer.h"
#include "nQuant/PnnQuantizer.h"
#include "nQuant/SpatialQuantizer.h"
#include "nQuant/WuQuantizer.h"

static QLabel* s_attribute_tab_render;
static QLabel* s_oam_tab_render;
static const int s_corner_size = 8;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    GeneralTab_Init();
    VideoTab_Init();

    //-------------------------------------------------------------------------------
    m_attribute_tab_zoom = 1;
    s_attribute_tab_render = new QLabel();
    s_attribute_tab_render->move(0, 0);
    ui->scrollArea_attribute->setWidget(s_attribute_tab_render);
    s_attribute_tab_render->installEventFilter(this);
    s_attribute_tab_render->setMouseTracking(true);
    ui->scrollArea_attribute->installEventFilter(this);

    //-------------------------------------------------------------------------------
    m_oam_tab_zoom = 8;
    s_oam_tab_render = new QLabel();
    s_oam_tab_render->move(0, 0);
    ui->scrollArea_OAM->setWidget(s_oam_tab_render);
    s_oam_tab_render->installEventFilter(this);
    s_oam_tab_render->setMouseTracking(true);
    ui->scrollArea_OAM->installEventFilter(this);
    m_oam_selected = -1;

    //-------------------------------------------------------------------------------
    ui->comboBox_compression->blockSignals(true);
    ui->comboBox_compression->addItem("None", QVariant((int)Compression_None));
    ui->comboBox_compression->addItem("Rle3", QVariant((int)Compression_Rle3));
    ui->comboBox_compression->addItem("Rle4", QVariant((int)Compression_Rle4));
    ui->comboBox_compression->blockSignals(false);

    ui->comboBox_chr_align->blockSignals(true);
    ui->comboBox_chr_align->addItem("None", QVariant((int)ChrAlign_None));
    ui->comboBox_chr_align->addItem("Align 1KB", QVariant((int)ChrAlign_1K));
    ui->comboBox_chr_align->addItem("Align 2KB", QVariant((int)ChrAlign_2K));
    ui->comboBox_chr_align->addItem("Align 4KB", QVariant((int)ChrAlign_4K));
    ui->comboBox_chr_align->addItem("Align 8KB", QVariant((int)ChrAlign_8K));
    ui->comboBox_chr_align->blockSignals(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::Image2Index(const QImage &image, std::vector<uint8_t>& index)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    index.resize(image.width()*image.height());


    /*{
        for (int y = 0; y < image.height(); ++y)
        {
            const uchar* line_ptr = image.constScanLine(y);
            uint8_t* dest_ptr = index.data() + y*image.width();
            for (int x = 0; x < image.width(); ++x)
            {
                uint32_t color = ((const uint32_t*)line_ptr)[x] & 0xFFFFFF;
                auto itt = m_palette_cvt_rule.find(color);
                if (itt != m_palette_cvt_rule.end())
                {
                    dest_ptr[x] = itt->second;
                    continue;
                }

                int r = color & 0xFF;
                int g = (color >> 8) & 0xFF;
                int b = (color >> 16) & 0xFF;
                int best_index = 0;
                uint64_t best_diff = UINT64_MAX;
                for (int c = 0; c < 16; ++c)
                {
                    uint32_t pcol = palette[m_palette_set[c >> 2].c[c & 3]];
                    int pr = pcol & 0xFF;
                    int pg = (pcol >> 8) & 0xFF;
                    int pb = (pcol >> 16) & 0xFF;
                    uint64_t diff = (r-pr)*(r-pr) + (g-pg)*(g-pg) + (b-pb)*(b-pb);
                    if (diff < best_diff)
                    {
                        best_diff = diff;
                        best_index = c;
                    }
                }
                dest_ptr[x] = best_index;
            }
        }
    }*/
}

void MainWindow::Index2Image(const std::vector<uint8_t>& index, QImage &image, int width, int height)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    if (image.width() != width || image.height() != height)
        image = QImage(width, height, QImage::Format_ARGB32);


    /*{
        for (int y = 0; y < height; ++y)
        {
            uchar* line_ptr = image.scanLine(y);
            const uint8_t* index_ptr = index.data() + y*width;
            for (int x = 0; x < width; ++x)
            {
                int c = index_ptr[x];
                ((uint32_t*)line_ptr)[x] = palette[m_palette_set[c >> 2].c[c & 3]];
            }
        }
    }*/
}

void MainWindow::UpdateIndexedImage()
{
    // Initialize GDI+.
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    if (gdiplusToken == 0)
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if (ui->checkBox_make_indexed->isChecked())
    {
        std::unique_ptr<Gdiplus::Bitmap> src_bitmap( new Gdiplus::Bitmap(m_image_original.width(), m_image_original.height(), PixelFormat32bppARGB) );
        Gdiplus::BitmapData src_data;
        memset(&src_data, 0, sizeof(src_data));
        Gdiplus::Rect image_rect(0, 0, src_bitmap->GetWidth(), src_bitmap->GetHeight());
        Gdiplus::Status status = src_bitmap->LockBits(&image_rect, Gdiplus::ImageLockModeWrite, src_bitmap->GetPixelFormat(), &src_data);
        if (status != 0)
        {
            qDebug() << "src_bitmap.LockBits" << status;
            return;
        }

        for (int y = 0; y < m_image_original.height(); ++y)
        {
            uint8_t* line_ptr = m_image_original.scanLine(y);
            memcpy((uint8_t*)src_data.Scan0 + src_data.Stride*y, line_ptr, m_image_original.width()*4);
        }
        src_bitmap->UnlockBits(&src_data);

        UINT max_colors = ui->lineEdit_palette_color_count->text().toInt();
        Gdiplus::Bitmap dst_bitmap(m_image_original.width(), m_image_original.height(), PixelFormat8bppIndexed);

        if (ui->comboBox_palette_method->currentText() == "DivQuantizer")
        {
            DivQuant::DivQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "Dl3Quantizer")
        {
            Dl3Quant::Dl3Quantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "EdgeAwareSQuantizer")
        {
            EdgeAwareSQuant::EdgeAwareSQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "MedianCut")
        {
            MedianCutQuant::MedianCut quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "MoDEQuantizer")
        {
            MoDEQuant::MoDEQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "NeuQuantizer")
        {
            NeuralNet::NeuQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "PnnLABQuantizer")
        {
            PnnLABQuant::PnnLABQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "PnnQuantizer")
        {
            PnnQuant::PnnQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "SpatialQuantizer")
        {
            SpatialQuant::SpatialQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }
        if (ui->comboBox_palette_method->currentText() == "WuQuantizer")
        {
            nQuant::WuQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, ui->checkBox_palette_deither->isChecked());
        }

        dst_bitmap.LockBits(&image_rect, ImageLockModeRead, dst_bitmap.GetPixelFormat(), &src_data);
        int palette_size_bytes = dst_bitmap.GetPaletteSize();
        std::vector<uint8_t> palette_data(2048, 0);
        ColorPalette* palette = (ColorPalette*)palette_data.data();
        dst_bitmap.GetPalette(palette, palette_size_bytes);

        m_image_indexed = QImage(m_image_original.width(), m_image_original.height(), QImage::Format_ARGB32);
        //m_image_indexed.setColorCount(palette->Count);
        //for (UINT i = 0; i < palette->Count; ++i)
        //    m_image_indexed.setColor(i, QRgb(palette->Entries[i]));

        for (int y = 0; y < m_image_indexed.height(); ++y)
        {
            int offset = y*src_data.Stride;
            uint8_t* line_ptr = m_image_indexed.scanLine(y);
            for (int x = 0; x < m_image_indexed.width(); ++x)
            {
                uint8_t index = ((uint8_t*)src_data.Scan0)[offset + x];
                line_ptr[x*4+0] = palette->Entries[index] >> 0;
                line_ptr[x*4+1] = palette->Entries[index] >> 8;
                line_ptr[x*4+2] = palette->Entries[index] >> 16;
                line_ptr[x*4+3] = 0xFF;
            }
        }
        dst_bitmap.UnlockBits(&src_data);
    } else
    {
        m_image_indexed = m_image_original;
    }
}



bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        /*for (int i = 0; i < 16; ++i)
        {
            if (object == m_palette_label[i])
            {
                m_pick_pallete_index = i;
                const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
                m_pick_color_dialog.SetPalette(palette);
                m_pick_color_dialog.SetPaletteIndex(m_palette_set[m_pick_pallete_index/4].c[m_pick_pallete_index & 3]);
                m_pick_color_dialog.UpdatePalette();
                m_pick_color_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                connect(&m_pick_color_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteWindow_Slot_PaletteSelect(int)), Qt::UniqueConnection);
                m_pick_color_dialog.exec();
                break;
            }
        }*/
    }

    VideoTab_EventFilter(object, event);


    if (object == ui->scrollArea_attribute)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                m_attribute_tab_zoom ++;
                if (m_attribute_tab_zoom > 4)
                    m_attribute_tab_zoom = 4;
                else
                    RedrawAttributeTab();
            }
            if(key->key() == Qt::Key_Minus)
            {
                m_attribute_tab_zoom --;
                if (m_attribute_tab_zoom < 1)
                    m_attribute_tab_zoom = 1;
                else
                    RedrawAttributeTab();
            }
        }
    }
    if (object == ui->scrollArea_OAM)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                m_oam_tab_zoom ++;
                if (m_oam_tab_zoom > 16)
                    m_oam_tab_zoom = 16;
                else
                    RedrawOamTab();
            }
            if(key->key() == Qt::Key_Minus)
            {
                m_oam_tab_zoom --;
                if (m_oam_tab_zoom < 1)
                    m_oam_tab_zoom = 1;
                else
                    RedrawOamTab();
            }
        }
    }
    if (object == s_attribute_tab_render)
    {
    }
    if (object == s_oam_tab_render)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x()/(m_oam_tab_zoom);
            int y = mouse_event->y()/(m_oam_tab_zoom);
            if ( (int)(mouse_event->buttons() & Qt::RightButton) != 0 )
            {
                if (ui->radioButton_oam_draw_original->isChecked())
                {
                    if (!m_image_indexed.isNull()
                            && x < m_image_indexed.width()
                            && y < m_image_indexed.height())
                    {
                        uint32_t color = ((const uint32_t*)m_image_indexed.constScanLine(y))[x];
                        color &= 0xFFFFFF;
                        m_pick_palette_cvt_color = color;
                        auto itt = m_palette_sprite_cvt_rule.find(m_pick_palette_cvt_color);
                        int index = -1;
                        if (itt != m_palette_sprite_cvt_rule.end())
                            index = itt->second;
                        m_pick_fami_palette_dialog.SetOriginalColor(m_pick_palette_cvt_color);
                        //const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
                        //m_pick_fami_palette_dialog.SetPalette(palette, m_palette_sprite);
                        m_pick_fami_palette_dialog.SetPaletteIndex(index);
                        m_pick_fami_palette_dialog.UpdatePalette();
                        m_pick_fami_palette_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                        connect(&m_pick_fami_palette_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteFamiWindow_Slot_PaletteSpriteSelect(int)), Qt::UniqueConnection);
                        m_pick_fami_palette_dialog.exec();
                    }
                } else
                {
                    if (x >= m_image_indexed.width() || y >= m_image_indexed.height())
                        return QWidget::eventFilter( object, event );
                    OAM oam;
                    oam.x = x;
                    oam.y = y;
                    //ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                    //int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                    oam.palette = 0;
                    if (ui->radioButton_oam_pal1->isChecked())
                        oam.palette = 1;
                    else if (ui->radioButton_oam_pal2->isChecked())
                        oam.palette = 2;
                    else if (ui->radioButton_oam_pal3->isChecked())
                        oam.palette = 3;
                    oam.mode = ui->checkBox_oam_fill_any_color->isChecked() ? 1 : 0;
                    m_oam_vector.push_back(oam);
                    RedrawOamTab();
                }
            }

            /*if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
            {
                m_oam_start_point.setX(x);
                m_oam_start_point.setY(y);
                ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                m_oam_selected = -1;
                for (size_t n = 0; n <m_oam_vector.size(); ++n)
                {
                    if (x >= m_oam_vector[n].x && x < m_oam_vector[n].x+8 &&
                        y >= m_oam_vector[n].y && y < m_oam_vector[n].y+sprite_height)
                    {
                        m_oam_selected = n;
                        break;
                    }
                }
                if (m_oam_selected >= 0)
                {
                    ui->checkBox_oam_fill_any_color->setChecked(m_oam_vector[m_oam_selected].mode);
                    ui->radioButton_oam_pal0->setChecked(m_oam_vector[m_oam_selected].palette == 0);
                    ui->radioButton_oam_pal1->setChecked(m_oam_vector[m_oam_selected].palette == 1);
                    ui->radioButton_oam_pal2->setChecked(m_oam_vector[m_oam_selected].palette == 2);
                    ui->radioButton_oam_pal3->setChecked(m_oam_vector[m_oam_selected].palette == 3);
                }
                RedrawOamTab();
            }*/
        }
        /*if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x()/(m_oam_tab_zoom);
            int y = mouse_event->y()/(m_oam_tab_zoom);
            if (m_oam_selected >= 0 && (int)(mouse_event->buttons() & Qt::LeftButton) != 0)
            {
                int dx = x - m_oam_start_point.x();
                int dy = y - m_oam_start_point.y();
                if (dx == 0 && dy == 0)
                    return QWidget::eventFilter( object, event );
                m_oam_start_point.setX(x);
                m_oam_start_point.setY(y);
                ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                m_oam_vector[m_oam_selected].x += dx;
                if (m_oam_vector[m_oam_selected].x < -7)
                    m_oam_vector[m_oam_selected].x = -7;
                if (m_oam_vector[m_oam_selected].x+1 > m_image_indexed.width())
                    m_oam_vector[m_oam_selected].x = m_image_indexed.width()-1;

                m_oam_vector[m_oam_selected].y += dy;
                if (m_oam_vector[m_oam_selected].y < -sprite_height+1)
                    m_oam_vector[m_oam_selected].y = -sprite_height+1;
                if (m_oam_vector[m_oam_selected].y+1 > m_image_indexed.height())
                    m_oam_vector[m_oam_selected].y = m_image_indexed.height()-1;
                RedrawOamTab();
            }
        }*/
    }
    return QWidget::eventFilter( object, event );
}

void MainWindow::PaletteWindow_Slot_PaletteSelect(int index)
{
}

void MainWindow::PaletteFamiWindow_Slot_PaletteSelect(int index)
{
    //qDebug() << "Pick" << index;
    auto itt = m_palette_cvt_rule.find(m_pick_palette_cvt_color);
    if (itt != m_palette_cvt_rule.end())
        itt->second = index;
    else
        m_palette_cvt_rule.insert(std::make_pair(m_pick_palette_cvt_color, index));
    VideoTab_Redraw();
}

void MainWindow::PaletteFamiWindow_Slot_PaletteSpriteSelect(int index)
{
    auto itt = m_palette_sprite_cvt_rule.find(m_pick_palette_cvt_color);
    if (itt != m_palette_sprite_cvt_rule.end())
        itt->second = index;
    else
        m_palette_sprite_cvt_rule.insert(std::make_pair(m_pick_palette_cvt_color, index));
    VideoTab_Redraw();
}

void MainWindow::on_pushButton_clear_colormapping_clicked()
{
    m_palette_cvt_rule.clear();
    VideoTab_Redraw();
}



void MainWindow::on_checkBox_palette_draw_cvt_stateChanged(int)
{
    VideoTab_Redraw();
}


void MainWindow::on_actionExit_triggered()
{
    this->close();
}


void MainWindow::on_actionOpen_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open project file"), QDir::currentPath(), "*.famimovie");
     if (file_name.isEmpty())
         return;
    m_project_file_name = file_name;
    LoadProject(file_name);
}

void MainWindow::on_actionSave_triggered()
{
    if (m_project_file_name.isEmpty())
        on_actionSave_as_triggered();
    SaveProject(m_project_file_name);
}


void MainWindow::on_actionSave_as_triggered()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save project file", QDir::currentPath(), "*.famimovie");
    if (file_name.isEmpty())
        return;
    m_project_file_name = file_name;
    SaveProject(m_project_file_name);
}

void MainWindow::SaveProject(const QString &file_name)
{
    picojson::value json;
    json.set<picojson::object>(picojson::object());
    json.get<picojson::object>()["palette_mode"] = picojson::value( (double)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt() );
    //json.get<picojson::object>()["palette_0_0"] = picojson::value( (double)m_palette_set[0].c[0] );

    json.get<picojson::object>()["movie_file_name"] = picojson::value( m_movie_file_name.toUtf8().toBase64().data() );
    json.get<picojson::object>()["name"] = picojson::value( ui->lineEdit_name->text().toUtf8().toBase64().data() );
    json.get<picojson::object>()["compression"] = picojson::value( (double)ui->comboBox_compression->itemData(ui->comboBox_compression->currentIndex()).toInt() );
    json.get<picojson::object>()["chr_align"] = picojson::value( (double)ui->comboBox_chr_align->itemData(ui->comboBox_chr_align->currentIndex()).toInt() );
    json.get<picojson::object>()["base_tiles"] = picojson::value( ui->lineEdit_base_tiles->text().toUtf8().toBase64().data() );

    json.get<picojson::object>()["indexed_apply"] = picojson::value( (bool)ui->checkBox_make_indexed->isChecked() );
    json.get<picojson::object>()["indexed_method"] = picojson::value( ui->comboBox_palette_method->currentText().toStdString() );
    json.get<picojson::object>()["indexed_diether"] = picojson::value( (bool)ui->checkBox_palette_deither->isChecked() );
    json.get<picojson::object>()["indexed_colors"] = picojson::value( (double)ui->lineEdit_palette_color_count->text().toInt() );

    int crop_x = ui->lineEdit_crop_x->text().toInt();
    int crop_y = ui->lineEdit_crop_y->text().toInt();
    int crop_width = ui->lineEdit_crop_width->text().toInt();
    int crop_height = ui->lineEdit_crop_height->text().toInt();
    json.get<picojson::object>()["movie_crop_x"] = picojson::value( (double)crop_x );
    json.get<picojson::object>()["movie_crop_y"] = picojson::value( (double)crop_y );
    json.get<picojson::object>()["movie_crop_width"] = picojson::value( (double)crop_width );
    json.get<picojson::object>()["movie_crop_height"] = picojson::value( (double)crop_height );

    int target_width = ui->lineEdit_target_width->text().toInt();
    int target_height = ui->lineEdit_target_height->text().toInt();
    json.get<picojson::object>()["movie_target_width"] = picojson::value( (double)target_width );
    json.get<picojson::object>()["movie_target_height"] = picojson::value( (double)target_height );

    picojson::array items = picojson::array();
    for (auto itt = m_palette_cvt_rule.begin(); itt != m_palette_cvt_rule.end(); ++itt)
    {
            picojson::object item_obj;
            item_obj["r"] = picojson::value( (double)(itt->first & 0xFF) );
            item_obj["g"] = picojson::value( (double)((itt->first >> 8) & 0xFF) );
            item_obj["b"] = picojson::value( (double)((itt->first >> 16) & 0xFF) );
            //item_obj["a"] = picojson::value( (double)((itt->first >> 24) & 0xFF) );
            item_obj["i"] = picojson::value( (double)itt->second );
            items.push_back(picojson::value(item_obj));
    }
    json.get<picojson::object>()["color_map"] = picojson::value(items);

    {
        picojson::array items = picojson::array();
        for (auto itt = m_palette_sprite_cvt_rule.begin(); itt != m_palette_sprite_cvt_rule.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["r"] = picojson::value( (double)(itt->first & 0xFF) );
                item_obj["g"] = picojson::value( (double)((itt->first >> 8) & 0xFF) );
                item_obj["b"] = picojson::value( (double)((itt->first >> 16) & 0xFF) );
                //item_obj["a"] = picojson::value( (double)((itt->first >> 24) & 0xFF) );
                item_obj["i"] = picojson::value( (double)itt->second );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["color_sprite_map"] = picojson::value(items);
    }

    {
        picojson::array items = picojson::array();
        for (size_t n = 0; n < m_oam_vector.size(); ++n)
        {
            picojson::object item_obj;
            item_obj["x"] = picojson::value( (double)m_oam_vector[n].x );
            item_obj["y"] = picojson::value( (double)m_oam_vector[n].y );
            item_obj["p"] = picojson::value( (double)m_oam_vector[n].palette );
            item_obj["m"] = picojson::value( (double)m_oam_vector[n].mode);
            items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["oam"] = picojson::value(items);
    }

    QString json_str = QString::fromStdString(json.serialize(true));
    QFile file(file_name);
    file.open(QFile::WriteOnly);
    if (file.error())
    {
        QMessageBox::critical(0, "Error", "Can't write file: " + file.errorString());
        return;
    }
    file.write(json_str.toLocal8Bit());
    file.close();
}

void MainWindow::LoadProject(const QString &file_name)
{
    QString json_str;
    QFile file(file_name);
    if (file.open(QFile::ReadOnly))
    {
        json_str = QString(file.readAll());
        file.close();
    } else
    {
        QMessageBox::critical(0, "Error", "Can't read file: " + file.errorString());
        return;
    }

    picojson::value json;
    picojson::parse(json, json_str.toStdString());
    int palette_mode = Palette_2C02;
    if (json.contains("palette_mode"))
        palette_mode = json.get<picojson::object>()["palette_mode"].get<double>();
    ui->comboBox_palette_mode->blockSignals(true);
    for (int i = 0; i < ui->comboBox_palette_mode->count(); ++i)
    {
        if (ui->comboBox_palette_mode->itemData(i).toInt() == palette_mode)
        {
            ui->comboBox_palette_mode->setCurrentIndex(i);
            break;
        }
    }
    ui->comboBox_palette_mode->blockSignals(false);

    {
        int compression = Compression_None;
        if (json.contains("compression"))
            palette_mode = json.get<picojson::object>()["compression"].get<double>();
        ui->comboBox_compression->blockSignals(true);
        for (int i = 0; i < ui->comboBox_compression->count(); ++i)
        {
            if (ui->comboBox_compression->itemData(i).toInt() == compression)
            {
                ui->comboBox_compression->setCurrentIndex(i);
                break;
            }
        }
        ui->comboBox_compression->blockSignals(false);
    }

    {
        int compression = ChrAlign_None;
        if (json.contains("chr_align"))
            palette_mode = json.get<picojson::object>()["chr_align"].get<double>();
        ui->comboBox_chr_align->blockSignals(true);
        for (int i = 0; i < ui->comboBox_chr_align->count(); ++i)
        {
            if (ui->comboBox_chr_align->itemData(i).toInt() == compression)
            {
                ui->comboBox_chr_align->setCurrentIndex(i);
                break;
            }
        }
        ui->comboBox_chr_align->blockSignals(false);
    }

    QString name;
    if (json.contains("name"))
        name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["name"].get<std::string>().c_str()));
    ui->lineEdit_name->setText(name);

    m_movie_file_name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["movie_file_name"].get<std::string>().c_str()));
    ui->lineEdit_move_file_name->setText(m_movie_file_name);

    try {
        m_avi_reader = std::make_shared<AviReader>(m_movie_file_name.toStdWString().c_str());
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

    if (json.contains("base_tiles"))
        ui->lineEdit_base_tiles->setText( QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["base_tiles"].get<std::string>().c_str())) );

    if (json.contains("movie_crop_x"))
        ui->lineEdit_crop_x->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_crop_x"].get<double>()));
    if (json.contains("movie_crop_y"))
        ui->lineEdit_crop_x->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_crop_y"].get<double>()));
    if (json.contains("movie_crop_width"))
        ui->lineEdit_crop_width->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_crop_width"].get<double>()));
    if (json.contains("movie_crop_height"))
        ui->lineEdit_crop_height->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_crop_height"].get<double>()));

    if (json.contains("movie_target_width"))
        ui->lineEdit_target_width->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_target_width"].get<double>()));
    if (json.contains("movie_target_height"))
        ui->lineEdit_target_height->setText(QString("%1").arg((int)json.get<picojson::object>()["movie_target_height"].get<double>()));

    m_palette_cvt_rule.clear();
    if (json.contains("color_map"))
    {
        picojson::array items = json.get<picojson::object>()["color_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            uint32_t r = (uint32_t)(itt->get<picojson::object>()["r"].get<double>());
            uint32_t g = (uint32_t)(itt->get<picojson::object>()["g"].get<double>());
            uint32_t b = (uint32_t)(itt->get<picojson::object>()["b"].get<double>());
            uint32_t a = 0x00;//(uint32_t)(itt->get<picojson::object>()["a"].get<double>());
            int i = (int)(itt->get<picojson::object>()["i"].get<double>());
            m_palette_cvt_rule.insert(std::make_pair(r | (g<<8) | (b<<16) | (a<<24), i));
        }
    }

    m_palette_sprite_cvt_rule.clear();
    if (json.contains("color_sprite_map"))
    {
        picojson::array items = json.get<picojson::object>()["color_sprite_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            uint32_t r = (uint32_t)(itt->get<picojson::object>()["r"].get<double>());
            uint32_t g = (uint32_t)(itt->get<picojson::object>()["g"].get<double>());
            uint32_t b = (uint32_t)(itt->get<picojson::object>()["b"].get<double>());
            uint32_t a = 0x00;//(uint32_t)(itt->get<picojson::object>()["a"].get<double>());
            int i = (int)(itt->get<picojson::object>()["i"].get<double>());
            m_palette_sprite_cvt_rule.insert(std::make_pair(r | (g<<8) | (b<<16) | (a<<24), i));
        }
    }

    if (json.contains("indexed_apply"))
        ui->checkBox_make_indexed->setChecked(json.get<picojson::object>()["indexed_apply"].get<bool>());
    if (json.contains("indexed_diether"))
        ui->checkBox_palette_deither->setChecked(json.get<picojson::object>()["indexed_diether"].get<bool>());
    if (json.contains("indexed_colors"))
        ui->lineEdit_palette_color_count->setText(QString("%1").arg((int)json.get<picojson::object>()["indexed_colors"].get<double>()));
    if (json.contains("indexed_method"))
    {
        QString method = QString::fromStdString(json.get<picojson::object>()["indexed_method"].get<std::string>());
        ui->comboBox_palette_method->blockSignals(true);
        for (int i = 0; i < ui->comboBox_palette_method->count(); ++i)
        {
            if (ui->comboBox_palette_method->itemText(i) == method)
            {
                ui->comboBox_palette_method->setCurrentIndex(i);
                break;
            }
        }
        ui->comboBox_palette_method->blockSignals(false);
    }

    m_oam_vector.clear();
    if (json.contains("oam"))
    {
        picojson::array items = json.get<picojson::object>()["oam"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            OAM oam;
            oam.x = (int)(itt->get<picojson::object>()["x"].get<double>());
            oam.y = (int)(itt->get<picojson::object>()["y"].get<double>());
            oam.palette = (int)(itt->get<picojson::object>()["p"].get<double>());
            oam.mode = (int)(itt->get<picojson::object>()["m"].get<double>());
            m_oam_vector.push_back(oam);
        }
    }

    GeneralTab_UpdateMovieInfo();
    GeneralTab_ReloadBaseTiles();
    VideoTab_FullUpdate();
    UpdateIndexedImage();
    GeneralTab_UpdatePalette();
    RedrawAttributeTab();
    FullUpdateOamTab();
}

void MainWindow::RedrawAttributeTab()
{
    if (m_image_original.isNull())
        return;

    /*const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    m_image_screen = QImage(256, 240, QImage::Format_ARGB32);
    m_image_screen.fill(palette[m_palette_set[0].c[0]]);
    {
        QPainter painter(&m_image_screen);
        painter.drawImage(0, 0, m_image_indexed);
    }
    std::vector<uint8_t> palette_best(16*16, 0);
    std::vector<uint8_t> palette_index(16*16, 0);
    std::vector<uint8_t> palette_c_best(16*16, 0);
    std::vector<uint8_t> palette_c(16*16, 0);
    m_screen_attribute.resize(16*16, 0);
    m_screen_tiles.resize(32*30*16, 0);
    memset(m_screen_tiles.data(), 0, m_screen_tiles.size());
    for (int y = 0; y < 240; y += 16)
    {
        for (int x = 0; x < 256; x += 16)
        {
            int best_pal = 0;
            int64_t best_pal_err = INT64_MAX;
            for (int pal = 0; pal < 4; ++pal)
            {
                int64_t cell_error_sum = 0;
                for (int yi = 0; yi < 16; ++yi)
                {
                    if (y+yi >= 240)
                        continue;
                    const uint8_t* src_line = m_image_screen.scanLine(y+yi) + x*4;
                    for (int xi = 0; xi < 16; ++xi)
                    {
                        uint32_t color = ((const uint32_t*)src_line)[xi] & 0xFFFFFF;
                        auto itt = m_palette_cvt_rule.find(color);
                        if (itt != m_palette_cvt_rule.end())
                        {
                            int c = itt->second;
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[0])
                            {
                                palette_index[xi + yi*16] = m_palette_set[pal].c[0];
                                palette_c[xi + yi*16] = 0;
                                continue;
                            }
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[1])
                            {
                                palette_index[xi + yi*16] = m_palette_set[pal].c[1];
                                palette_c[xi + yi*16] = 1;
                                continue;
                            }
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[2])
                            {
                                palette_index[xi + yi*16] = m_palette_set[pal].c[2];
                                palette_c[xi + yi*16] = 2;
                                continue;
                            }
                            if (m_palette_set[c/4].c[c%4] == m_palette_set[pal].c[3])
                            {
                                palette_index[xi + yi*16] = m_palette_set[pal].c[3];
                                palette_c[xi + yi*16] = 3;
                                continue;
                            }
                        }
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
                                palette_index[xi + yi*16] = ci;
                                palette_c[xi + yi*16] = c;
                                best_diff = d;
                            }
                        }
                        cell_error_sum += best_diff;
                    }
                }
                if (cell_error_sum < best_pal_err)
                {
                    best_pal_err = cell_error_sum;
                    best_pal = pal;
                    memcpy(palette_best.data(), palette_index.data(), palette_index.size() * sizeof(palette_index[0]));
                    memcpy(palette_c_best.data(), palette_c.data(), palette_c.size() * sizeof(palette_c[0]));
                }
            }
            m_screen_attribute[(y/16) * 16 + x/16] = best_pal;
            //Apply palette
            for (int yi = 0; yi < 16; ++yi)
            {
                if (y+yi >= 240)
                    continue;
                uint8_t* line = m_image_screen.scanLine(y+yi) + x*4;
                for (int xi = 0; xi < 16; ++xi)
                {
                    ((uint32_t*)line)[xi] = palette[palette_best[xi + yi*16]];
                    int tile_y = (y + yi) / 8;
                    int tile_dy = (y + yi) % 8;
                    int tile_x = (x + xi) / 8;
                    int tile_dx = (x + xi) % 8;
                    if (palette_c_best[xi + yi*16] == 1)
                    {
                        m_screen_tiles[(tile_y*32 + tile_x)*16 + tile_dy    ] |= 0x80 >> tile_dx;
                    } else if (palette_c_best[xi + yi*16] == 2)
                    {
                        m_screen_tiles[(tile_y*32 + tile_x)*16 + tile_dy + 8] |= 0x80 >> tile_dx;
                    } else if (palette_c_best[xi + yi*16] == 3)
                    {
                        m_screen_tiles[(tile_y*32 + tile_x)*16 + tile_dy    ] |= 0x80 >> tile_dx;
                        m_screen_tiles[(tile_y*32 + tile_x)*16 + tile_dy + 8] |= 0x80 >> tile_dx;
                    }
                }
            }
        }
    }
    //m_image_screen = m_image_screen.copy(0, 0, 256, 240);
    std::set<std::vector<uint8_t>> tile_set;
    for (size_t n = 0; n < 32*30; ++n)
    {
        std::vector<uint8_t> tile(16);
        memcpy(tile.data(), m_screen_tiles.data() + n*16, 16);
        tile_set.insert(tile);
    }
    ui->label_attribute_tiles->setText(QString("%1 Tiles").arg(tile_set.size()));

    QImage image = m_image_screen.scaled(m_image_screen.width()*m_attribute_tab_zoom, m_image_screen.height()*m_attribute_tab_zoom);
    if (ui->checkBox_attribute_grid->isChecked())
    {
        QPainter painter(&image);
        painter.setPen(QColor(0xFF00FF00));
        for (int y = 16; y < 240; y += 16)
            painter.drawLine(0, y*m_attribute_tab_zoom, image.width(), y*m_attribute_tab_zoom);
        for (int x = 16; x < 256; x += 16)
            painter.drawLine(x*m_attribute_tab_zoom, 0, x*m_attribute_tab_zoom, image.height());
    }
    s_attribute_tab_render->setPixmap(QPixmap::fromImage(image));
    s_attribute_tab_render->resize(image.width(), image.height());
    s_attribute_tab_render->setMaximumSize(image.width(), image.height());
    s_attribute_tab_render->setMinimumSize(image.width(), image.height());*/
}

void MainWindow::on_checkBox_attribute_grid_clicked()
{
    RedrawAttributeTab();
}

void MainWindow::on_tabWidget_currentChanged(int)
{
    if (ui->tabWidget->currentWidget() == ui->tab_video)
    {
        VideoTab_FullUpdate();
    }
    if (ui->tabWidget->currentWidget() == ui->tab_audio)
    {
        RedrawAttributeTab();
    }
    if (ui->tabWidget->currentWidget() == ui->tab_OAM)
    {
        FullUpdateOamTab();
    }
}

void MainWindow::FullUpdateOamTab()
{
    m_oam_selected = -1;
    /*for (size_t n = 0; n < m_slice_vector.size(); ++n)
    {
        if (m_slice_vector[n].caption == m_oam_slice_last)
            select_item = 0;
        ui->comboBox_oam_slice->addItem(m_slice_vector[n].caption);
    }
    if (m_slice_vector.size())
        ui->comboBox_oam_slice->setCurrentIndex(select_item);*/
    RedrawOamTab();
}

void MainWindow::RedrawOamTab()
{
    if (m_image_indexed.isNull() || m_image_screen.isNull())
        return;
    ui->label_oam_couner->setText(QString("%1").arg(m_oam_vector.size()));


    /*ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
    int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;

    QImage render(m_image_indexed.width(), m_image_indexed.height(), QImage::Format_ARGB32);
    //render.fill(m_bg_color);
    {
        QPainter painter(&render);
        if (ui->radioButton_oam_draw_original->isChecked())
            painter.drawImage(0, 0, m_image_indexed);
        else
            painter.drawImage(0, 0, m_image_screen);
    }

    //radioButton_oam_draw_background

    if (ui->radioButton_oam_draw_result->isChecked())
    {
        const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
        for (size_t n = 0; n <m_oam_vector.size(); ++n)
        {
            int pindex = m_oam_vector[n].palette;
            Palette pal = m_palette_sprite[pindex];
            for (int y = 0; y < sprite_height; ++y)
            {
                int yline = y + m_oam_vector[n].y;
                if (yline < 0 || yline > render.height())
                    continue;
                const uint32_t* scan_line = (const uint32_t*)m_image_indexed.scanLine(yline);
                uint32_t* render_line = (uint32_t*)render.scanLine(yline);
                for (int x = 0; x < 8; ++x)
                {
                    int xp = x + m_oam_vector[n].x;
                    if (xp < 0 || xp > render.width())
                        continue;
                    //if (scan_line[xp] == m_bg_color)
                    //    continue;
                    uint32_t color = scan_line[xp] & 0x00FFFFFF;
                    auto itt = m_palette_sprite_cvt_rule.find(color);
                    if (itt != m_palette_sprite_cvt_rule.end())
                    {
                        if (itt->second == pindex*4 + 1)
                        {
                            render_line[xp] = palette[pal.c[1]];
                        } else if (itt->second == pindex*4 + 2)
                        {
                            render_line[xp] = palette[pal.c[2]];
                        } else if (itt->second == pindex*4 + 3)
                        {
                            render_line[xp] = palette[pal.c[3]];
                        } else if (m_oam_vector[n].mode != 0)
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
                            render_line[xp] = palette[pal.c[best_c]];
                        }
                    } else
                    {
                        if (color == palette[pal.c[1]])
                        {
                            render_line[xp] = color;
                        } else if (color == palette[pal.c[2]])
                        {
                            render_line[xp] = color;
                        } else if (color == palette[pal.c[3]])
                        {
                            render_line[xp] = color;
                        } else if (m_oam_vector[n].mode != 0)
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
                            render_line[xp] = palette[pal.c[best_c]];
                        }
                    }
                }
            }
        }
    }

    QImage image = render.scaled(render.width()*m_oam_tab_zoom, render.height()*m_oam_tab_zoom);
    {
        QPainter painter(&image);
        painter.setBrush(Qt::NoBrush);
        if (ui->checkBox_draw_grid->isChecked())
        {
            painter.setPen(QColor(0xFF007F00));
            for (int y = 1; y < render.height()/16; ++y)
                painter.drawLine(0, y*16*m_oam_tab_zoom, image.width(), y*16*m_oam_tab_zoom);
            for (int x = 1; x < render.width()/16; ++x)
                painter.drawLine(x*16*m_oam_tab_zoom, 0, x*16*m_oam_tab_zoom, image.height());
        }
        for (size_t n = 0; n < m_oam_vector.size(); ++n)
        {
            if (m_oam_vector[n].palette == 0)
                painter.setPen(QColor(0xFF00FF00));
            else if (m_oam_vector[n].palette == 1)
                painter.setPen(QColor(0xFFFF0000));
            else if (m_oam_vector[n].palette == 2)
                painter.setPen(QColor(0xFF0000FF));
            else if (m_oam_vector[n].palette == 3)
                painter.setPen(QColor(0xFFFFFF00));
            if (n == m_oam_selected)
                painter.setPen(QColor(0xFFFFFFFF));
            //int dx = (m_oam_vector[n].palette+1) * render.width();
            painter.drawRect(m_oam_vector[n].x*m_oam_tab_zoom, m_oam_vector[n].y*m_oam_tab_zoom, 8*m_oam_tab_zoom, sprite_height*m_oam_tab_zoom);
            //painter.drawRect((m_oam_vector[n].x + dx)*m_oam_tab_zoom ,m_oam_vector[n].y*m_oam_tab_zoom, 8*m_oam_tab_zoom, sprite_height*m_oam_tab_zoom);
        }
    }
    s_oam_tab_render->setPixmap(QPixmap::fromImage(image));
    s_oam_tab_render->setMinimumSize(image.size());
    s_oam_tab_render->setMaximumSize(image.size());*/
}

void MainWindow::on_radioButton_oam_pal0_clicked()
{
    if (m_oam_selected < 0)
        return;
    m_oam_vector[m_oam_selected].palette = 0;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal1_clicked()
{
    if (m_oam_selected < 0)
        return;
    m_oam_vector[m_oam_selected].palette = 1;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal2_clicked()
{
    if (m_oam_selected < 0)
        return;
    m_oam_vector[m_oam_selected].palette = 2;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal3_clicked()
{
    if (m_oam_selected < 0)
        return;
    m_oam_vector[m_oam_selected].palette = 3;
    RedrawOamTab();
}


void MainWindow::on_checkBox_oam_fill_any_color_clicked()
{
    if (m_oam_selected < 0)
        return;
    m_oam_vector[m_oam_selected].mode = ui->checkBox_oam_fill_any_color->isChecked();
    RedrawOamTab();
}

void MainWindow::on_lineEdit_palette_color_count_editingFinished()
{
    UpdateIndexedImage();
    VideoTab_Redraw();
}

void MainWindow::on_checkBox_palette_deither_clicked()
{
    UpdateIndexedImage();
    VideoTab_Redraw();
}

void MainWindow::on_comboBox_palette_method_currentIndexChanged(int)
{
    UpdateIndexedImage();
    VideoTab_Redraw();
}

void MainWindow::on_checkBox_make_indexed_clicked()
{
    UpdateIndexedImage();
    VideoTab_Redraw();
}


void MainWindow::on_checkBox_draw_grid_clicked()
{
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_draw_original_clicked()
{
    RedrawOamTab();
}

void MainWindow::on_radioButton_oam_draw_result_clicked()
{
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_draw_background_clicked()
{
    RedrawOamTab();
}


