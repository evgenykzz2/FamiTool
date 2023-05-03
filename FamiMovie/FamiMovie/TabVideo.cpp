#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
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

static QLabel* s_video_tab_render;
static std::vector<uint8_t> s_video_frame_original;
static int s_video_tab_zoom = 1;
static QLabel* s_palette_labels[16];
static int s_hash_movie_frame_number = -1;
static QImage s_hash_movie_frame_original;

void MainWindow::VideoTab_Init()
{
    s_video_tab_render = new QLabel();
    s_video_tab_render->move(0, 0);
    ui->scrollArea_video_tab->setWidget(s_video_tab_render);
    s_video_tab_render->installEventFilter(this);
    s_video_tab_render->setMouseTracking(true);
    ui->scrollArea_video_tab->installEventFilter(this);

    ui->comboBox_frame_mode->blockSignals(true);
    ui->comboBox_frame_mode->addItem("Skip", (int)FrameMode_Skip);
    ui->comboBox_frame_mode->addItem("Black", (int)FrameMode_Black);
    ui->comboBox_frame_mode->addItem("KeyFrame", (int)FrameMode_KeyFrame);
    ui->comboBox_frame_mode->addItem("Inter", (int)FrameMode_Inter);
    ui->comboBox_frame_mode->blockSignals(false);

    QValidator* validator_frame_number = new QIntValidator(0, 999999, this);
    ui->lineEdit_video_frame_number->setValidator(validator_frame_number);
    ui->lineEdit_palette_color_count->setValidator(validator_frame_number);

    ui->comboBox_palette_method->addItem("DivQuantizer", (int)IndexMethod_DivQuantizer);
    ui->comboBox_palette_method->addItem("Dl3Quantizer", (int)IndexMethod_Dl3Quantizer);
    //ui->combo_method->addItem("EdgeAwareSQuantizer", (int)IndexMethod_EdgeAwareSQuantizer);
    ui->comboBox_palette_method->addItem("MedianCut", (int)IndexMethod_MedianCut);
    //ui->combo_method->addItem("MoDEQuantizer", (int)IndexMethod_MoDEQuantizer);
    ui->comboBox_palette_method->addItem("NeuQuantizer", (int)IndexMethod_NeuQuantizer);
    ui->comboBox_palette_method->addItem("PnnLABQuantizer", (int)IndexMethod_PnnQuantizer);
    ui->comboBox_palette_method->addItem("PnnQuantizer", (int)IndexMethod_DivQuantizer);
    ui->comboBox_palette_method->addItem("SpatialQuantizer", (int)IndexMethod_SpatialQuantizer);
    ui->comboBox_palette_method->addItem("WuQuantizer", (int)IndexMethod_WuQuantizer);

    ui->comboBox_video_view_mode->addItem("Original", (int)ViewMode_Original);
    ui->comboBox_video_view_mode->addItem("Crop", (int)ViewMode_Crop);
    ui->comboBox_video_view_mode->addItem("Indexed", (int)ViewMode_Indexed);
    ui->comboBox_video_view_mode->addItem("NesPalette", (int)ViewMode_NesPalette);
    ui->comboBox_video_view_mode->addItem("Tiles", (int)ViewMode_TileConverted);
    ui->comboBox_video_view_mode->addItem("Tiles Final", (int)ViewMode_TileOptimized);

    s_palette_labels[0] = ui->label_frame_pal_0_0;
    s_palette_labels[1] = ui->label_frame_pal_0_1;
    s_palette_labels[2] = ui->label_frame_pal_0_2;
    s_palette_labels[3] = ui->label_frame_pal_0_3;

    s_palette_labels[4] = ui->label_frame_pal_1_0;
    s_palette_labels[5] = ui->label_frame_pal_1_1;
    s_palette_labels[6] = ui->label_frame_pal_1_2;
    s_palette_labels[7] = ui->label_frame_pal_1_3;

    s_palette_labels[ 8] = ui->label_frame_pal_2_0;
    s_palette_labels[ 9] = ui->label_frame_pal_2_1;
    s_palette_labels[10] = ui->label_frame_pal_2_2;
    s_palette_labels[11] = ui->label_frame_pal_2_3;

    s_palette_labels[12] = ui->label_frame_pal_3_0;
    s_palette_labels[13] = ui->label_frame_pal_3_1;
    s_palette_labels[14] = ui->label_frame_pal_3_2;
    s_palette_labels[15] = ui->label_frame_pal_3_3;

    for (int i = 0; i < 16; ++i)
        s_palette_labels[i]->installEventFilter(this);

    // Initialize GDI+.
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    if (gdiplusToken == 0)
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void MainWindow::VideoTab_FullUpdate()
{
    if (!m_avi_reader)
        return;
    ui->horizontalSlider->setMinimum(0);
    ui->horizontalSlider->setMaximum(m_avi_reader->VideoFrameCount()-1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));

    VideoTab_UpdateFrameNumber();
}

void MainWindow::VideoTab_UpdateFrameNumber()
{
    if (!m_avi_reader)
        return;

    uint32_t nes_frame = 60 * ui->horizontalSlider->value() * m_avi_reader->VideoTicksPerFrame() / m_avi_reader->VideoTimeScale();
    ui->label_nes_frame->setText(QString("NTSC frame= %1").arg(nes_frame));
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
    {
        FrameInfo info;
        m_frame_info_map.insert(std::make_pair(ui->horizontalSlider->value(), info));
        itt = m_frame_info_map.find(ui->horizontalSlider->value());
        if (itt == m_frame_info_map.end())
            return;
    }

    ui->comboBox_frame_mode->blockSignals(true);
    for (int i = 0; i < ui->comboBox_frame_mode->count(); ++i)
    {
        if (ui->comboBox_frame_mode->itemData(i) == itt->second.frame_mode)
        {
            ui->comboBox_frame_mode->setCurrentIndex(i);
            break;
        }
    }
    ui->comboBox_frame_mode->blockSignals(false);

    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    for (int i = 0; i < 16; ++i)
    {
        QImage img(s_palette_labels[i]->width(), s_palette_labels[i]->height(), QImage::Format_ARGB32);
        img.fill(palette[itt->second.palette[i]]);
        s_palette_labels[i]->setPixmap(QPixmap::fromImage(img));
    }

    ui->checkBox_make_indexed->setChecked(itt->second.indexed);
    ui->comboBox_palette_method->blockSignals(true);
    for (int i = 0; i < ui->comboBox_palette_method->count(); ++i)
    {
        if (ui->comboBox_palette_method->itemData(i) == itt->second.index_method)
        {
            ui->comboBox_palette_method->setCurrentIndex(i);
            break;
        }
    }
    ui->comboBox_palette_method->blockSignals(false);
    ui->checkBox_palette_deither->setChecked(itt->second.diether);
    ui->lineEdit_palette_color_count->setText(QString("%1").arg(itt->second.colors));
    VideoTab_Redraw();
}

QImage MainWindow::VideoTab_MovieImage(int frame_number)
{
    if (!m_avi_reader->VideoFrameRead(frame_number, s_video_frame_original))
    {
        QImage image(m_avi_reader->VideoWidth(), m_avi_reader->VideoHeight(), QImage::Format_ARGB32);
        image.fill(0xFF000000);
        return image;
    }

    QImage image(m_avi_reader->VideoWidth(), m_avi_reader->VideoHeight(), QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y)
    {
        uint8_t* dst_line = image.scanLine(y);
        const uint8_t* src_line = s_video_frame_original.data() + y * image.width()*3;
        for (int x = 0; x < image.width(); ++x)
        {
            if (src_line+x*3+2 >= s_video_frame_original.data() + s_video_frame_original.size())
                continue;
            dst_line[x*4+0] = src_line[x*3+0];
            dst_line[x*4+1] = src_line[x*3+1];
            dst_line[x*4+2] = src_line[x*3+2];
            dst_line[x*4+3] = 0xFF;
        }
    }
    return image;
}

QImage MainWindow::VideoTab_CropImage(const QImage& original)
{
    int movie_crop_x = ui->lineEdit_crop_x->text().toInt();
    int movie_crop_y = ui->lineEdit_crop_y->text().toInt();
    int movie_crop_width = ui->lineEdit_crop_width->text().toInt();
    int movie_crop_hight = ui->lineEdit_crop_height->text().toInt();
    if (movie_crop_width < 8)
        movie_crop_width = 8;
    if (movie_crop_width > original.width())
        movie_crop_width = original.width();
    if (movie_crop_hight < 8)
        movie_crop_hight = 8;
    if (movie_crop_hight > original.height())
        movie_crop_hight = original.height();

    int movie_target_width = ui->lineEdit_target_width->text().toInt();
    int movie_target_height = ui->lineEdit_target_height->text().toInt();
    if (movie_target_width > 256)
        movie_target_width = 256;
    if (movie_target_height > 256)
        movie_target_height = 256;
    movie_target_width = movie_target_width & 0xFFF8;
    movie_target_height = movie_target_height & 0xFFF8;

    QImage image_crop(256, 240, QImage::Format_ARGB32);
    image_crop.fill(0xFF000000);
    {
        QImage scale_image = original.copy(movie_crop_x, movie_crop_y, movie_crop_width, movie_crop_hight);
        scale_image = scale_image.scaled(movie_target_width, movie_target_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        int putx = ((image_crop.width() - movie_target_width) / 2) & 0xFFF8;
        int puty = ((image_crop.height() - movie_target_height) / 2) & 0xFFF8;
        QPainter painter(&image_crop);
        painter.drawImage(putx, puty, scale_image);
    }
    return image_crop;
}

void MainWindow::VideoTab_BuildBlackFrame(std::shared_ptr<FrameImageCompilation>& build)
{
    build = std::make_shared<FrameImageCompilation>();
    build->indexed = QImage(256, 240, QImage::Format_Indexed8);
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    build->indexed.setColorCount(64);
    for (UINT i = 0; i < 64; ++i)
        build->indexed.setColor(i, QRgb(0xFF000000 | palette[i]));
    build->indexed.fill(0x0F);
    build->palette_convert = build->indexed;
    build->tile_convert = build->indexed;
}

bool MainWindow::VideoTab_GetBuildFrame(int frame_number, std::shared_ptr<FrameImageCompilation>& build, int depth)
{
    if (frame_number < 0 || depth > 20)
    {
        VideoTab_BuildBlackFrame(build);
        return false;
    }

    auto info_itt = m_frame_info_map.find(frame_number);
    if (info_itt == m_frame_info_map.end())
    {
        FrameInfo inf;
        m_frame_info_map.insert(std::make_pair(frame_number, inf));
        info_itt = m_frame_info_map.find(frame_number);
    }

    auto itt = m_frame_build.find(frame_number);
    if (itt != m_frame_build.end())
    {
        if (!info_itt->second.force_update)
        {
            build = itt->second;
            return true;
        } else
            info_itt->second.force_update = false;
    }


    if (info_itt->second.frame_mode == FrameMode_Black)
    {
        VideoTab_BuildBlackFrame(build);
        m_frame_build.insert(std::make_pair(frame_number, build));
        return true;
    }

    if (info_itt->second.frame_mode == FrameMode_Skip)
    {
        return VideoTab_GetBuildFrame(frame_number-1, build, depth+1);
    }

    if (!m_avi_reader)
    {
        VideoTab_BuildBlackFrame(build);
        return false;
    }

    if (!m_avi_reader->VideoFrameRead(frame_number, s_video_frame_original))
    {
        VideoTab_BuildBlackFrame(build);
        return false;
    }

    QImage movie_image = VideoTab_MovieImage(frame_number);
    QImage crop_image = VideoTab_CropImage(movie_image);

    if (itt != m_frame_build.end())
        build = itt->second;
    else
        build = std::make_shared<FrameImageCompilation>();
    build->indexed = QImage(256, 240, QImage::Format_Indexed8);

    //if (info_itt->second.indexed)
    {
        std::unique_ptr<Gdiplus::Bitmap> src_bitmap( new Gdiplus::Bitmap(crop_image.width(), crop_image.height(), PixelFormat32bppARGB) );
        Gdiplus::BitmapData src_data;
        memset(&src_data, 0, sizeof(src_data));
        Gdiplus::Rect image_rect(0, 0, src_bitmap->GetWidth(), src_bitmap->GetHeight());
        Gdiplus::Status status = src_bitmap->LockBits(&image_rect, Gdiplus::ImageLockModeWrite, src_bitmap->GetPixelFormat(), &src_data);
        if (status != 0)
        {
            qDebug() << "src_bitmap.LockBits" << status;
            return false;
        }

        for (int y = 0; y < crop_image.height(); ++y)
        {
            uint8_t* line_ptr = crop_image.scanLine(y);
            memcpy((uint8_t*)src_data.Scan0 + src_data.Stride*y, line_ptr, crop_image.width()*4);
        }
        src_bitmap->UnlockBits(&src_data);

        UINT max_colors = info_itt->second.colors;
        Gdiplus::Bitmap dst_bitmap(crop_image.width(), crop_image.height(), PixelFormat8bppIndexed);


        if (info_itt->second.index_method == IndexMethod_DivQuantizer)
        {
            DivQuant::DivQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_Dl3Quantizer)
        {
            Dl3Quant::Dl3Quantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_EdgeAwareSQuantizer)
        {
            EdgeAwareSQuant::EdgeAwareSQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_MedianCut)
        {
            MedianCutQuant::MedianCut quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_MoDEQuantizer)
        {
            MoDEQuant::MoDEQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_NeuQuantizer)
        {
            NeuralNet::NeuQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_PnnLABQuantizer)
        {
            PnnLABQuant::PnnLABQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_PnnQuantizer)
        {
            PnnQuant::PnnQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_SpatialQuantizer)
        {
            SpatialQuant::SpatialQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }
        if (info_itt->second.index_method == IndexMethod_WuQuantizer)
        {
            nQuant::WuQuantizer quantizer;
            quantizer.QuantizeImage(src_bitmap.get(), &dst_bitmap, max_colors, info_itt->second.diether);
        }

        dst_bitmap.LockBits(&image_rect, ImageLockModeRead, dst_bitmap.GetPixelFormat(), &src_data);
        int palette_size_bytes = dst_bitmap.GetPaletteSize();
        std::vector<uint8_t> palette_data(2048, 0);
        ColorPalette* palette = (ColorPalette*)palette_data.data();
        dst_bitmap.GetPalette(palette, palette_size_bytes);

        build->indexed.setColorCount(palette->Count);
        for (UINT i = 0; i < palette->Count; ++i)
            build->indexed.setColor(i, QRgb(palette->Entries[i]));

        for (int y = 0; y < build->indexed.height(); ++y)
        {
            int offset = y*src_data.Stride;
            uint8_t* line_ptr = build->indexed.scanLine(y);
            for (int x = 0; x < build->indexed.width(); ++x)
                line_ptr[x] = ((uint8_t*)src_data.Scan0)[offset + x];
        }
        dst_bitmap.UnlockBits(&src_data);
    }

    {
        /*build->palette_convert = QImage(256, 240, QImage::Format_Indexed8);
        for (int y = 0; y < build->indexed.height(); ++y)
        {
            for (int x = 0; x < build->indexed.width(); ++x)
            {
                line_ptr[x] = ((uint8_t*)src_data.Scan0)[offset + x];
            }
        }*/
    }

    m_frame_build.insert(std::make_pair(frame_number, build));
    return true;
}

void MainWindow::VideoTab_Redraw()
{
    if (!m_avi_reader)
        return;

    int frame_number = ui->horizontalSlider->value();
    if (s_hash_movie_frame_number != frame_number)
    {
        s_hash_movie_frame_original = VideoTab_MovieImage(frame_number);
        s_hash_movie_frame_number = frame_number;
    }
    QImage image_view;
    EViewMode view_mode = (EViewMode)ui->comboBox_video_view_mode->itemData(ui->comboBox_video_view_mode->currentIndex()).toInt();
    if (view_mode == ViewMode_Original)
    {
        image_view = s_hash_movie_frame_original.scaled(s_hash_movie_frame_original.width()*s_video_tab_zoom, s_hash_movie_frame_original.height()*s_video_tab_zoom);
    } else if (view_mode == ViewMode_Crop)
    {
        image_view = VideoTab_CropImage(s_hash_movie_frame_original);
        image_view = image_view.scaled(image_view.width()*s_video_tab_zoom, image_view.height()*s_video_tab_zoom);
    } else
    {
        std::shared_ptr<FrameImageCompilation> build;
        if (!VideoTab_GetBuildFrame(frame_number, build))
            return;
        image_view = QImage(256, 240, QImage::Format_ARGB32);
        {
            QPainter painter(&image_view);
            if (view_mode == ViewMode_Indexed)
                painter.drawImage(0, 0, build->indexed);
            else if (view_mode == ViewMode_NesPalette)
                painter.drawImage(0, 0, build->palette_convert);
            else if (view_mode == ViewMode_TileConverted)
                painter.drawImage(0, 0, build->tile_convert);
            else
                painter.drawImage(0, 0, build->final);
        }
        image_view = image_view.scaled(image_view.width()*s_video_tab_zoom, image_view.height()*s_video_tab_zoom);
    }
    s_video_tab_render->setPixmap(QPixmap::fromImage(image_view));
    s_video_tab_render->resize(image_view.width(), image_view.height());
    s_video_tab_render->setMaximumSize(image_view.width(), image_view.height());
    s_video_tab_render->setMinimumSize(image_view.width(), image_view.height());
}

void MainWindow::VideoTab_EventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (object == s_palette_labels[i])
            {
                auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
                if (itt == m_frame_info_map.end())
                    return;

                m_pick_pallete_index = i;
                const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
                m_pick_color_dialog.SetPalette(palette);
                m_pick_color_dialog.SetPaletteIndex(itt->second.palette[m_pick_pallete_index]);
                m_pick_color_dialog.UpdatePalette();
                m_pick_color_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                connect(&m_pick_color_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteWindow_Slot_PaletteSelect(int)), Qt::UniqueConnection);
                m_pick_color_dialog.exec();
                break;
            }
        }
    }

    if (object == ui->scrollArea_video_tab)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                s_video_tab_zoom ++;
                if (s_video_tab_zoom > 4)
                    s_video_tab_zoom = 4;
                else
                    VideoTab_Redraw();
            }
            if(key->key() == Qt::Key_Minus)
            {
                s_video_tab_zoom --;
                if (s_video_tab_zoom < 1)
                    s_video_tab_zoom = 1;
                else
                    VideoTab_Redraw();
            }
        }
    }

    if (object == s_video_tab_render)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            if ( (int)(mouse_event->buttons() & Qt::RightButton) != 0 )
            {
                int x = mouse_event->x()/(s_video_tab_zoom);
                int y = mouse_event->y()/(s_video_tab_zoom);
                /*if (!m_image_indexed.isNull()
                        && x < m_image_indexed.width()
                        && y < m_image_indexed.height())
                {
                    uint32_t color = ((const uint32_t*)m_image_indexed.constScanLine(y))[x];
                    color &= 0xFFFFFF;
                    m_pick_palette_cvt_color = color;

                    auto itt = m_palette_cvt_rule.find(m_pick_palette_cvt_color);
                    int index = -1;
                    if (itt != m_palette_cvt_rule.end())
                        index = itt->second;
                    m_pick_fami_palette_dialog.SetOriginalColor(m_pick_palette_cvt_color);
                    //const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
                    //m_pick_fami_palette_dialog.SetPalette(palette, m_palette_set);
                    m_pick_fami_palette_dialog.SetPaletteIndex(index);
                    m_pick_fami_palette_dialog.UpdatePalette();
                    m_pick_fami_palette_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                    connect(&m_pick_fami_palette_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteFamiWindow_Slot_PaletteSelect(int)), Qt::UniqueConnection);
                    m_pick_fami_palette_dialog.exec();
                }*/
            }
        }
    }
}

void MainWindow::PaletteWindow_Slot_PaletteSelect(int index)
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;

    itt->second.palette[m_pick_pallete_index] = index;
    itt->second.force_update = true;

    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    QImage img(s_palette_labels[m_pick_pallete_index]->width(), s_palette_labels[m_pick_pallete_index]->height(), QImage::Format_ARGB32);
    img.fill(palette[itt->second.palette[m_pick_pallete_index]]);
    s_palette_labels[m_pick_pallete_index]->setPixmap(QPixmap::fromImage(img));

    VideoTab_Redraw();
}

void MainWindow::on_pushButton_frame_previous_clicked()
{
    if (ui->horizontalSlider->value() == ui->horizontalSlider->minimum())
        return;
    ui->horizontalSlider->setValue(ui->horizontalSlider->value()-1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_UpdateFrameNumber();
}

void MainWindow::on_pushButton_frame_next_clicked()
{
    if (ui->horizontalSlider->value() == ui->horizontalSlider->maximum())
        return;
    ui->horizontalSlider->setValue(ui->horizontalSlider->value()+1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_UpdateFrameNumber();
}

void MainWindow::on_horizontalSlider_sliderMoved(int)
{
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_UpdateFrameNumber();
}


void MainWindow::on_lineEdit_video_frame_number_editingFinished()
{
    bool ok = false;
    int frame_number = ui->lineEdit_video_frame_number->text().toInt(&ok);
    if (!ok)
    {
        ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
        return;
    }
    if (frame_number < ui->horizontalSlider->minimum())
    {
        frame_number = ui->horizontalSlider->minimum();
        ui->lineEdit_video_frame_number->setText(QString("%1").arg(frame_number));
    }
    if (frame_number > ui->horizontalSlider->maximum())
    {
        frame_number = ui->horizontalSlider->maximum();
        ui->lineEdit_video_frame_number->setText(QString("%1").arg(frame_number));
    }
    ui->horizontalSlider->setValue(frame_number);
    VideoTab_UpdateFrameNumber();
}

void MainWindow::on_comboBox_video_view_mode_currentIndexChanged(int )
{
    VideoTab_Redraw();
}

void MainWindow::on_comboBox_frame_mode_currentIndexChanged(int)
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;

    itt->second.frame_mode = (EFrameMode)ui->comboBox_frame_mode->itemData(ui->comboBox_frame_mode->currentIndex()).toInt();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_checkBox_make_indexed_clicked()
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;
    itt->second.indexed = ui->checkBox_make_indexed->isChecked();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_comboBox_palette_method_currentIndexChanged(int)
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;
    itt->second.index_method = (EIndexMethod)ui->comboBox_palette_method->itemData(ui->comboBox_palette_method->currentIndex()).toInt();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_checkBox_palette_deither_clicked()
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;
    itt->second.diether = ui->checkBox_palette_deither->isChecked();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_lineEdit_palette_color_count_editingFinished()
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;
    itt->second.colors = ui->lineEdit_palette_color_count->text().toInt();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_pushButton_clear_colormapping_clicked()
{
    auto itt = m_frame_info_map.find(ui->horizontalSlider->value());
    if (itt == m_frame_info_map.end())
        return;
    itt->second.color_map.clear();
    itt->second.force_update = true;
    VideoTab_Redraw();
}

void MainWindow::on_pushButton_copy_palette_clicked()
{
    int frame_number = ui->horizontalSlider->value();
    auto itt = m_frame_info_map.find(frame_number);
    if (itt == m_frame_info_map.end())
        return;

    auto itt_prev = m_frame_info_map.find(frame_number-1);
    if (itt_prev == m_frame_info_map.end())
        return;
    itt->second.palette = itt_prev->second.palette;
    itt->second.force_update = true;
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    for (int i = 0; i < 16; ++i)
    {
        QImage img(s_palette_labels[i]->width(), s_palette_labels[i]->height(), QImage::Format_ARGB32);
        img.fill(palette[itt->second.palette[i]]);
        s_palette_labels[i]->setPixmap(QPixmap::fromImage(img));
    }
    VideoTab_Redraw();
}
