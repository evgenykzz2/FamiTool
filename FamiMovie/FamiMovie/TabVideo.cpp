#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QDebug>

static QLabel* s_video_tab_render;
static std::vector<uint8_t> s_video_frame_original;
static int s_video_tab_zoom = 1;

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
}

void MainWindow::VideoTab_FullUpdate()
{
    if (!m_avi_reader)
        return;
    ui->horizontalSlider->setMinimum(0);
    ui->horizontalSlider->setMaximum(m_avi_reader->VideoFrameCount()-1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));

    VideoTab_Redraw();
}

void MainWindow::VideoTab_Redraw()
{
    if (!m_avi_reader)
        return;
    if (!m_avi_reader->VideoFrameRead(ui->horizontalSlider->value(), s_video_frame_original))
        return;

    //qDebug() << m_avi_reader->VideoWidth() << m_avi_reader->VideoHeight();
    QImage image_original(m_avi_reader->VideoWidth(), m_avi_reader->VideoHeight(), QImage::Format_ARGB32);
    for (int y = 0; y < image_original.height(); ++y)
    {
        uint8_t* dst_line = image_original.scanLine(y);
        const uint8_t* src_line = s_video_frame_original.data() + y * image_original.width()*3;
        for (int x = 0; x < image_original.width(); ++x)
        {
            if (src_line+x*3+2 >= s_video_frame_original.data() + s_video_frame_original.size())
                continue;
            dst_line[x*4+0] = src_line[x*3+0];
            dst_line[x*4+1] = src_line[x*3+1];
            dst_line[x*4+2] = src_line[x*3+2];
            dst_line[x*4+3] = 0xFF;
        }
    }

    /*if (ui->checkBox_palette_draw_cvt->isChecked())
    {
        Image2Index(m_image_indexed, m_spriteset_index);
        QImage pal_cvt_image;
        Index2Image(m_spriteset_index, pal_cvt_image, m_image_indexed.width(), m_image_indexed.height());
        image = pal_cvt_image.scaled(pal_cvt_image.width()*m_palette_tab_zoom, pal_cvt_image.height()*m_palette_tab_zoom);
    } else*/
    QImage image = image_original.scaled(image_original.width()*s_video_tab_zoom, image_original.height()*s_video_tab_zoom);
    s_video_tab_render->setPixmap(QPixmap::fromImage(image));
    s_video_tab_render->resize(image.width(), image.height());
    s_video_tab_render->setMaximumSize(image.width(), image.height());
    s_video_tab_render->setMinimumSize(image.width(), image.height());
}

void MainWindow::VideoTab_EventFilter(QObject* object, QEvent* event)
{
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
                if (!m_image_indexed.isNull()
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
                }
            }
        }
    }
}

void MainWindow::on_pushButton_frame_previous_clicked()
{
    if (ui->horizontalSlider->value() == ui->horizontalSlider->minimum())
        return;
    ui->horizontalSlider->setValue(ui->horizontalSlider->value()-1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_Redraw();
}

void MainWindow::on_pushButton_frame_next_clicked()
{
    if (ui->horizontalSlider->value() == ui->horizontalSlider->maximum())
        return;
    ui->horizontalSlider->setValue(ui->horizontalSlider->value()+1);
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_Redraw();
}

void MainWindow::on_horizontalSlider_sliderMoved(int)
{
    ui->lineEdit_video_frame_number->setText(QString("%1").arg(ui->horizontalSlider->value()));
    VideoTab_Redraw();
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
    VideoTab_Redraw();
}
