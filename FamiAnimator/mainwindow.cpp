#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include "picojson.h"

static QLabel* s_palette_tab_render;
static QLabel* s_slice_tab_render;
static QLabel* s_oam_tab_render;
static const int s_corner_size = 8;
static const int s_blink_palette_block_size = 24;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_palette_label[0] = ui->label_palette_0_0;
    m_palette_label[1] = ui->label_palette_0_1;
    m_palette_label[2] = ui->label_palette_0_2;
    m_palette_label[3] = ui->label_palette_0_3;

    m_palette_label[4] = ui->label_palette_1_0;
    m_palette_label[5] = ui->label_palette_1_1;
    m_palette_label[6] = ui->label_palette_1_2;
    m_palette_label[7] = ui->label_palette_1_3;

    m_palette_label[8] = ui->label_palette_2_0;
    m_palette_label[9] = ui->label_palette_2_1;
    m_palette_label[10] = ui->label_palette_2_2;
    m_palette_label[11] = ui->label_palette_2_3;

    m_palette_label[12] = ui->label_palette_3_0;
    m_palette_label[13] = ui->label_palette_3_1;
    m_palette_label[14] = ui->label_palette_3_2;
    m_palette_label[15] = ui->label_palette_3_3;

    m_pick_pallete_index = 0;
    for (int i = 0; i < 16; ++i)
        m_palette_label[i]->installEventFilter(this);

    ui->label_global_bg_color->installEventFilter(this);

    m_bg_color_index = 0x25;

    ui->comboBox_palette_mode->blockSignals(true);
    ui->comboBox_palette_mode->addItem("2C02 (Famicom)", QVariant((int)Palette_2C02));
    ui->comboBox_palette_mode->addItem("2C03", QVariant((int)Palette_2C03));
    ui->comboBox_palette_mode->addItem("2C04", QVariant((int)Palette_2C04));
    ui->comboBox_palette_mode->addItem("fceux", QVariant((int)Palette_fceux));
    ui->comboBox_palette_mode->blockSignals(false);

    ui->comboBox_sprite_mode->blockSignals(true);
    ui->comboBox_sprite_mode->addItem("8x8", QVariant((int)SpriteMode_8x8));
    ui->comboBox_sprite_mode->addItem("8x16", QVariant((int)SpriteMode_8x16));
    ui->comboBox_sprite_mode->blockSignals(false);

    ui->widget_blink_palette->setVisible(ui->checkBox_palette_blink->isChecked());
    ui->label_palette_blink->installEventFilter(this);

    on_comboBox_palette_mode_currentIndexChanged(0);


    //-------------------------------------------------------------------------------
    m_palette_tab_zoom = 1;
    s_palette_tab_render = new QLabel();
    s_palette_tab_render->move(0, 0);
    ui->scrollArea_palette_tab->setWidget(s_palette_tab_render);
    s_palette_tab_render->installEventFilter(this);
    s_palette_tab_render->setMouseTracking(true);
    ui->scrollArea_palette_tab->installEventFilter(this);

    //-------------------------------------------------------------------------------
    m_slice_tab_zoom = 1;
    s_slice_tab_render = new QLabel();
    s_slice_tab_render->move(0, 0);
    ui->scrollArea_slice->setWidget(s_slice_tab_render);
    s_slice_tab_render->installEventFilter(this);
    s_slice_tab_render->setMouseTracking(true);
    ui->scrollArea_slice->installEventFilter(this);

    m_slice_start_point_active = false;
    m_slice_selected = -1;

    //-------------------------------------------------------------------------------
    m_oam_tab_zoom = 8;
    s_oam_tab_render = new QLabel();
    s_oam_tab_render->move(0, 0);
    ui->scrollArea_OAM->setWidget(s_oam_tab_render);
    s_oam_tab_render->installEventFilter(this);
    s_oam_tab_render->setMouseTracking(true);
    ui->scrollArea_OAM->installEventFilter(this);
    m_oam_selected = -1;

    AnimationTab_Init();

    connect(&m_frame_timer, SIGNAL(timeout()), this, SLOT(TimerFrame()));
    m_frame_timer.start(1000 / 100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_comboBox_palette_mode_currentIndexChanged(int index)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(index).toInt());
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

    for (int i = 0; i < 16; ++i)
    {
        QImage image(m_palette_label[i]->width(), m_palette_label[i]->height(), QImage::Format_ARGB32);
        image.fill(palette[m_palette_set[i/4].c[i & 3]]);
        m_palette_label[i]->setPixmap(QPixmap::fromImage(image));
    }

    m_bg_color = palette[m_bg_color_index];
    {
        QImage image(ui->label_global_bg_color->width(), ui->label_global_bg_color->height(), QImage::Format_ARGB32);
        image.fill(m_bg_color);
        ui->label_global_bg_color->setPixmap(QPixmap::fromImage(image));
    }

    if (ui->checkBox_palette_blink->isChecked())
        UpdateBlinkPalette();

    RedrawPaletteTab();
}

void MainWindow::UpdateBlinkPalette()
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    QImage image_sprites(s_blink_palette_block_size*16+1, s_blink_palette_block_size*4+1, QImage::Format_ARGB32);
    image_sprites.fill(0);
    {
        QPainter painter_sp(&image_sprites);
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 16; ++x)
            {
                int a = x / 4;
                int b = x % 4;
                m_blink_palette.color[y*16 + x] = ColorAvg(palette[m_palette_set[y].c[a]], palette[m_palette_set[y].c[b]]);
                painter_sp.setPen(QColor((uint32_t)0));
                painter_sp.setBrush(QColor(m_blink_palette.color[y*16 + x]));
                painter_sp.drawRect(x*s_blink_palette_block_size, y*s_blink_palette_block_size, s_blink_palette_block_size, s_blink_palette_block_size);
                if (m_blink_palette.enable[y*16+x] == 0)
                {
                    painter_sp.setPen(Qt::red);
                    painter_sp.drawLine(x*s_blink_palette_block_size+1, y*s_blink_palette_block_size+1, x*s_blink_palette_block_size+s_blink_palette_block_size-1, y*s_blink_palette_block_size+s_blink_palette_block_size-1);
                    painter_sp.drawLine(x*s_blink_palette_block_size+s_blink_palette_block_size-1, y*s_blink_palette_block_size+1, x*s_blink_palette_block_size+1, y*s_blink_palette_block_size+s_blink_palette_block_size-1);
                }
            }
        }
    }
    ui->label_palette_blink->setPixmap(QPixmap::fromImage(image_sprites));
    ui->label_palette_blink->setMinimumSize(image_sprites.size());
    ui->label_palette_blink->setMaximumSize(image_sprites.size());
}

void MainWindow::Image2Index(const QImage &image, std::vector<uint8_t>& index)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    index.resize(image.width()*image.height());
    uint32_t bg_color = ((const uint32_t*)image.constBits())[0];

    for (int y = 0; y < image.height(); ++y)
    {
        const uchar* line_ptr = image.constScanLine(y);
        uint8_t* dest_ptr = index.data() + y*image.width();
        for (int x = 0; x < image.width(); ++x)
        {
            uint32_t color = ((const uint32_t*)line_ptr)[x];
            if (color == bg_color)
            {
                dest_ptr[x] = 0;
                continue;
            }
            auto itt = m_palette_cvt_rule.find(color);
            if (itt != m_palette_cvt_rule.end())
            {
                dest_ptr[x] = itt->second;
                continue;
            }

            int r = color & 0xFF;
            int g = (color >> 8) & 0xFF;
            int b = (color >> 16) & 0xFF;
            int a = (color >> 24) & 0xFF;
            int best_index = 0;
            if (a > 128)
            {
                uint64_t best_diff = UINT64_MAX;
                for (int c = 0; c < 16; ++c)
                {
                    if ( (int)(c & 3) == 0)
                        continue;
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
            }
            dest_ptr[x] = best_index;
        }
    }
}

void MainWindow::Index2Image(const std::vector<uint8_t>& index, QImage &image, int width, int height)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    if (image.width() != width || image.height() != height)
        image = QImage(width, height, QImage::Format_ARGB32);
    if (ui->checkBox_palette_blink->isChecked())
    {
        for (int y = 0; y < height; ++y)
        {
            uchar* line_ptr = image.scanLine(y);
            const uint8_t* index_ptr = index.data() + y*width;
            for (int x = 0; x < width; ++x)
            {
                int c = index_ptr[x];
                if (c == 0)
                    ((uint32_t*)line_ptr)[x] = m_bg_color;
                else
                    ((uint32_t*)line_ptr)[x] = m_blink_palette.color[c];
            }
        }
    } else
    {
        for (int y = 0; y < height; ++y)
        {
            uchar* line_ptr = image.scanLine(y);
            const uint8_t* index_ptr = index.data() + y*width;
            for (int x = 0; x < width; ++x)
            {
                int c = index_ptr[x];
                if (c == 0)
                    ((uint32_t*)line_ptr)[x] = m_bg_color;
                else
                    ((uint32_t*)line_ptr)[x] = palette[m_palette_set[c >> 2].c[c & 3]];
            }
        }
    }
}

void MainWindow::Index2ImageAlpha(const std::vector<uint8_t>& index, QImage &image, int width, int height)
{
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    if (image.width() != width || image.height() != height)
        image = QImage(width, height, QImage::Format_ARGB32);
    if (ui->checkBox_palette_blink->isChecked())
    {
        for (int y = 0; y < height; ++y)
        {
            uchar* line_ptr = image.scanLine(y);
            const uint8_t* index_ptr = index.data() + y*width;
            for (int x = 0; x < width; ++x)
            {
                int c = index_ptr[x];
                if (c == 0)
                    ((uint32_t*)line_ptr)[x] = 0x00000000;
                else
                    ((uint32_t*)line_ptr)[x] = m_blink_palette.color[c];
            }
        }
    } else
    {
        for (int y = 0; y < height; ++y)
        {
            uchar* line_ptr = image.scanLine(y);
            const uint8_t* index_ptr = index.data() + y*width;
            for (int x = 0; x < width; ++x)
            {
                int c = index_ptr[x];
                if (c == 0)
                    ((uint32_t*)line_ptr)[x] = 0x00000000;
                else
                    ((uint32_t*)line_ptr)[x] = palette[m_palette_set[c >> 2].c[c & 3]];
            }
        }
    }
}

void MainWindow::RedrawPaletteTab()
{
    if (m_spriteset_original.isNull())
        return;

    Image2Index(m_spriteset_original, m_spriteset_index);
    Index2Image(m_spriteset_index, m_spriteset_indexed, m_spriteset_original.width(), m_spriteset_original.height());
    Index2ImageAlpha(m_spriteset_index, m_spriteset_indexed_alpha, m_spriteset_original.width(), m_spriteset_original.height());


    QImage image;
    if (ui->checkBox_palette_draw_cvt->isChecked())
    {
        image = m_spriteset_indexed.scaled(m_spriteset_indexed.width()*m_palette_tab_zoom, m_spriteset_indexed.height()*m_palette_tab_zoom);
    } else
        image = m_spriteset_original.scaled(m_spriteset_original.width()*m_palette_tab_zoom, m_spriteset_original.height()*m_palette_tab_zoom);

    s_palette_tab_render->setPixmap(QPixmap::fromImage(image));
    s_palette_tab_render->resize(image.width(), image.height());
    s_palette_tab_render->setMaximumSize(image.width(), image.height());
    s_palette_tab_render->setMinimumSize(image.width(), image.height());

    uint32_t bg_color = ((const uint32_t*)image.constBits())[0];
    QImage img(ui->label_spriteset_bg->width(), ui->label_spriteset_bg->height(), QImage::Format_ARGB32);
    img.fill(bg_color);
    ui->label_spriteset_bg->setPixmap(QPixmap::fromImage(img));
}

bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        for (int i = 0; i < 16; ++i)
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
        }

        if (object == ui->label_global_bg_color)
        {
            const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
            m_pick_color_dialog.SetPalette(palette);
            m_pick_color_dialog.SetPaletteIndex(m_bg_color_index);
            m_pick_color_dialog.UpdatePalette();
            m_pick_color_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
            connect(&m_pick_color_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteWindow_Slot_BgPaletteSelect(int)), Qt::UniqueConnection);
            m_pick_color_dialog.exec();
        }

        if (object == ui->label_palette_blink)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x()/(s_blink_palette_block_size);
            int y = mouse_event->y()/(s_blink_palette_block_size);
            if (x < 16 && y < 4)
            {
                m_blink_palette.enable[x + y*16] ^= 1;
                UpdateBlinkPalette();
            }
        }
    }
    if (object == s_palette_tab_render)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            if ( (int)(mouse_event->buttons() & Qt::RightButton) != 0 )
            {
                int x = mouse_event->x()/(m_palette_tab_zoom);
                int y = mouse_event->y()/(m_palette_tab_zoom);
                if (!m_spriteset_original.isNull()
                        && x < m_spriteset_original.width()
                        && y < m_spriteset_original.height())
                {
                    m_pick_palette_cvt_color = ((const uint32_t*)m_spriteset_original.constScanLine(y))[x];
                    auto itt = m_palette_cvt_rule.find(m_pick_palette_cvt_color);
                    int index = -1;
                    if (itt != m_palette_cvt_rule.end())
                        index = itt->second;
                    m_pick_fami_palette_dialog.SetOriginalColor(m_pick_palette_cvt_color);
                    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
                    m_pick_fami_palette_dialog.SetPalette(palette, m_palette_set, ui->checkBox_palette_blink->isChecked(), m_blink_palette);
                    m_pick_fami_palette_dialog.SetPaletteIndex(index);
                    m_pick_fami_palette_dialog.UpdatePalette();
                    m_pick_fami_palette_dialog.disconnect(SIGNAL(SignalPaletteSelect(int)));
                    connect(&m_pick_fami_palette_dialog, SIGNAL(SignalPaletteSelect(int)), SLOT(PaletteFamiWindow_Slot_PaletteSelect(int)), Qt::UniqueConnection);
                    m_pick_fami_palette_dialog.exec();
                }
            }
        }
    }
    if (object == ui->scrollArea_palette_tab)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                m_palette_tab_zoom ++;
                if (m_palette_tab_zoom > 4)
                    m_palette_tab_zoom = 4;
                else
                    RedrawPaletteTab();
            }
            if(key->key() == Qt::Key_Minus)
            {
                m_palette_tab_zoom --;
                if (m_palette_tab_zoom < 1)
                    m_palette_tab_zoom = 1;
                else
                    RedrawPaletteTab();
            }
        }
    }
    if (object == ui->scrollArea_slice)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                m_slice_tab_zoom ++;
                if (m_slice_tab_zoom > 4)
                    m_slice_tab_zoom = 4;
                else
                    RedrawSliceTab();
            }
            if(key->key() == Qt::Key_Minus)
            {
                m_slice_tab_zoom --;
                if (m_slice_tab_zoom < 1)
                    m_slice_tab_zoom = 1;
                else
                    RedrawSliceTab();
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
            if (key->key() == Qt::Key_Delete)
            {
                if (m_oam_selected >= 0)
                {
                    size_t index = ui->comboBox_oam_slice->currentIndex();
                    if (index >= m_slice_vector.size())
                        return QWidget::eventFilter( object, event );
                    m_slice_vector[index].oam.erase(m_slice_vector[index].oam.begin() + m_oam_selected);
                    m_oam_selected = -1;
                    RedrawOamTab();
                }
            }
        }
    }
    if (object == s_slice_tab_render)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            if ( (int)(mouse_event->buttons() & Qt::RightButton) != 0 )
            {
                int x = mouse_event->x()/(m_slice_tab_zoom);
                int y = mouse_event->y()/(m_slice_tab_zoom);
                m_slice_start_point.setX(x);
                m_slice_start_point.setY(y);
                m_slice_start_point_active = true;
            } else if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
            {
                int x = mouse_event->x()/(m_slice_tab_zoom);
                int y = mouse_event->y()/(m_slice_tab_zoom);
                m_slice_selected = -1;
                m_slice_start_point.setX(x);
                m_slice_start_point.setY(y);
                for (size_t n = 0; n < m_slice_vector.size(); ++n)
                {
                    if (x >= m_slice_vector[n].x && x < m_slice_vector[n].x + m_slice_vector[n].width &&
                        y >= m_slice_vector[n].y && y < m_slice_vector[n].y + m_slice_vector[n].height)
                    {
                        m_slice_selected = n;
                        if (x >= m_slice_vector[n].x && x < m_slice_vector[n].x + s_corner_size &&
                            y >= m_slice_vector[n].y && y < m_slice_vector[n].y + s_corner_size)
                            m_slice_selected_corner = 0;
                        else if (x >= m_slice_vector[n].x+m_slice_vector[n].width-s_corner_size && x < m_slice_vector[n].x+m_slice_vector[n].width &&
                                 y >= m_slice_vector[n].y && y < m_slice_vector[n].y + s_corner_size)
                            m_slice_selected_corner = 1;
                        else if (x >= m_slice_vector[n].x && x < m_slice_vector[n].x + s_corner_size &&
                                 y >= m_slice_vector[n].y+m_slice_vector[n].height-s_corner_size && y < m_slice_vector[n].y + m_slice_vector[n].height)
                            m_slice_selected_corner = 2;
                        else if (x >= m_slice_vector[n].x+m_slice_vector[n].width-s_corner_size  && x < m_slice_vector[n].x + m_slice_vector[n].width &&
                                 y >= m_slice_vector[n].y+m_slice_vector[n].height-s_corner_size && y < m_slice_vector[n].y + m_slice_vector[n].height)
                            m_slice_selected_corner = 3;
                        else
                            m_slice_selected_corner = -1;
                        break;
                    }
                }
                if (m_slice_selected != -1)
                {
                    ui->comboBox_slice_list->blockSignals(true);
                    for (int i = 0; i < ui->comboBox_slice_list->count(); ++i)
                    {
                        if ( m_slice_vector[m_slice_selected].id == ui->comboBox_slice_list->itemData(i).toLongLong() )
                        {
                            ui->comboBox_slice_list->setCurrentIndex(i);
                            break;
                        }
                    }
                    ui->comboBox_slice_list->blockSignals(false);
                    ui->lineEdit_slice_name->setText(m_slice_vector[m_slice_selected].caption);
                    ui->lineEdit_slice_name->setEnabled(true);
                    RedrawSliceTab();
                } else
                {
                    ui->lineEdit_slice_name->setText("");
                    ui->lineEdit_slice_name->setEnabled(false);
                    RedrawSliceTab();
                }
            }
        }

        if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x()/(m_slice_tab_zoom);
            int y = mouse_event->y()/(m_slice_tab_zoom);
            if (m_slice_start_point_active)
            {
                if ( (int)(mouse_event->buttons() & Qt::RightButton) == 0 )
                {
                    m_slice_start_point_active = false;
                    RedrawSliceTab();
                } else
                {
                    m_slice_end_point.setX(x);
                    m_slice_end_point.setY(y);
                    RedrawSliceTab();
                }
            } else if (m_slice_selected >= 0 && (int)(mouse_event->buttons() & Qt::LeftButton) != 0)
            {
                int dx = x - m_slice_start_point.x();
                int dy = y - m_slice_start_point.y();
                if (dx != 0 || dy != 0)
                {
                    if (m_slice_selected_corner == -1)
                    {
                        m_slice_vector[m_slice_selected].x += dx;
                        if (m_slice_vector[m_slice_selected].x < 0)
                            m_slice_vector[m_slice_selected].x = 0;
                        if (m_slice_vector[m_slice_selected].x + m_slice_vector[m_slice_selected].width > m_spriteset_original.width())
                            m_slice_vector[m_slice_selected].x = m_spriteset_original.width() - m_slice_vector[m_slice_selected].width;

                        m_slice_vector[m_slice_selected].y += dy;
                        if (m_slice_vector[m_slice_selected].y < 0)
                            m_slice_vector[m_slice_selected].y = 0;
                        if (m_slice_vector[m_slice_selected].y + m_slice_vector[m_slice_selected].height > m_spriteset_original.height())
                            m_slice_vector[m_slice_selected].y = m_spriteset_original.height() - m_slice_vector[m_slice_selected].height;
                    } else
                    {
                         if (m_slice_selected_corner == 0 || m_slice_selected_corner == 1)
                         {
                             m_slice_vector[m_slice_selected].y += dy;
                             if (m_slice_vector[m_slice_selected].y < 0)
                                 m_slice_vector[m_slice_selected].y = 0;
                             if (m_slice_vector[m_slice_selected].y >= m_spriteset_original.height())
                                 m_slice_vector[m_slice_selected].y = m_spriteset_original.height()-1;
                             m_slice_vector[m_slice_selected].height -= dy;
                             if (m_slice_vector[m_slice_selected].height < 1)
                                 m_slice_vector[m_slice_selected].height = 1;
                         } else
                         {
                            m_slice_vector[m_slice_selected].height += dy;
                            if (m_slice_vector[m_slice_selected].height < 1)
                                m_slice_vector[m_slice_selected].height = 1;
                            if (m_slice_vector[m_slice_selected].y + m_slice_vector[m_slice_selected].height > m_spriteset_original.height())
                                m_slice_vector[m_slice_selected].height = m_spriteset_original.height() - m_slice_vector[m_slice_selected].y;
                         }
                         if (m_slice_selected_corner == 0 || m_slice_selected_corner == 2)
                         {
                             m_slice_vector[m_slice_selected].x += dx;
                             if (m_slice_vector[m_slice_selected].x < 0)
                                 m_slice_vector[m_slice_selected].x = 0;
                             if (m_slice_vector[m_slice_selected].x >= m_spriteset_original.width())
                                 m_slice_vector[m_slice_selected].x = m_spriteset_original.width()-1;
                             m_slice_vector[m_slice_selected].width -= dx;
                             if (m_slice_vector[m_slice_selected].width < 1)
                                 m_slice_vector[m_slice_selected].width = 1;
                         } else
                         {
                             m_slice_vector[m_slice_selected].width += dx;
                             if (m_slice_vector[m_slice_selected].width < 1)
                                 m_slice_vector[m_slice_selected].width = 1;
                             if (m_slice_vector[m_slice_selected].x + m_slice_vector[m_slice_selected].width > m_spriteset_original.width())
                                 m_slice_vector[m_slice_selected].width = m_spriteset_original.width() - m_slice_vector[m_slice_selected].x;
                         }
                    }

                    m_slice_start_point.setX(x);
                    m_slice_start_point.setY(y);
                    RedrawSliceTab();
                }
            }
        }
        if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x()/(m_slice_tab_zoom);
            int y = mouse_event->y()/(m_slice_tab_zoom);
            //if ( (int)(mouse_event->buttons() & Qt::RightButton) != 0 )
            {
                if (m_slice_start_point_active)
                {
                    m_slice_end_point.setX(x);
                    m_slice_end_point.setY(y);
                    m_slice_start_point_active = false;
                    SliceArea area;
                    area.id = QDateTime::currentMSecsSinceEpoch();
                    area.x = m_slice_start_point.x() < m_slice_end_point.x() ? m_slice_start_point.x() : m_slice_end_point.x();
                    area.y = m_slice_start_point.y() < m_slice_end_point.y() ? m_slice_start_point.y() : m_slice_end_point.y();
                    area.width = abs(m_slice_end_point.x() - m_slice_start_point.x());
                    area.height = abs(m_slice_end_point.y() - m_slice_start_point.y());
                    for (int i = 0; i < 1000; ++i)
                    {
                        area.caption = QString("area %1").arg(i);
                        bool dup = false;
                        for (size_t n = 0; n < m_slice_vector.size(); ++n)
                        {
                            if (m_slice_vector[n].caption == area.caption)
                            {
                                dup = true;
                                break;
                            }
                        }
                        if (!dup)
                            break;
                    }

                    ui->comboBox_slice_list->addItem(area.caption, QVariant(area.id));
                    m_slice_vector.push_back(area);
                    m_slice_selected = m_slice_vector.size()-1;
                    RedrawSliceTab();
                } else if (m_slice_selected != -1)
                {
                    //m_slice_selected = -1;
                    //RedrawSliceTab();
                }
            }
        }

        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Delete)
                on_pushButton_slice_delete_clicked();
        }
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
                size_t index = ui->comboBox_oam_slice->currentIndex();
                if (index >= m_slice_vector.size())
                    return QWidget::eventFilter( object, event );
                OAM oam;
                oam.x = x % m_slice_vector[index].width;
                if (oam.x+8 > m_slice_vector[index].width)
                    oam.x = m_slice_vector[index].width - 8;
                oam.y = y;
                ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                if (oam.x+sprite_height > m_slice_vector[index].width)
                    oam.x = m_slice_vector[index].width - sprite_height;
                oam.palette = 0;
                if (ui->radioButton_oam_pal1->isChecked())
                    oam.palette = 1;
                else if (ui->radioButton_oam_pal2->isChecked())
                    oam.palette = 2;
                else if (ui->radioButton_oam_pal3->isChecked())
                    oam.palette = 3;
                oam.mode = ui->checkBox_oam_fill_any_color->isChecked() ? 1 : 0;
                m_slice_vector[index].oam.push_back(oam);
                RedrawOamTab();
            }
            if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
            {
                m_oam_start_point.setX(x);
                m_oam_start_point.setY(y);
                size_t index = ui->comboBox_oam_slice->currentIndex();
                if (index >= m_slice_vector.size())
                    return QWidget::eventFilter( object, event );
                ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                m_oam_selected = -1;
                int sx = x % m_slice_vector[index].width;
                int px = x / m_slice_vector[index].width;
                for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
                {
                    if (x >= m_slice_vector[index].oam[n].x && x < m_slice_vector[index].oam[n].x+8 &&
                        y >= m_slice_vector[index].oam[n].y && y < m_slice_vector[index].oam[n].y+sprite_height)
                    {
                        m_oam_selected = n;
                        break;
                    }
                    if (px == m_slice_vector[index].oam[n].palette+1 &&
                        sx >= m_slice_vector[index].oam[n].x && sx < m_slice_vector[index].oam[n].x+8 &&
                        y >= m_slice_vector[index].oam[n].y && y < m_slice_vector[index].oam[n].y+sprite_height)
                    {
                        m_oam_selected = n;
                        break;
                    }
                }
                if (m_oam_selected >= 0)
                {
                    ui->checkBox_oam_fill_any_color->setChecked(m_slice_vector[index].oam[m_oam_selected].mode);
                    ui->radioButton_oam_pal0->setChecked(m_slice_vector[index].oam[m_oam_selected].palette == 0);
                    ui->radioButton_oam_pal1->setChecked(m_slice_vector[index].oam[m_oam_selected].palette == 1);
                    ui->radioButton_oam_pal2->setChecked(m_slice_vector[index].oam[m_oam_selected].palette == 2);
                    ui->radioButton_oam_pal3->setChecked(m_slice_vector[index].oam[m_oam_selected].palette == 3);
                }
                RedrawOamTab();
            }
        }
        if (event->type() == QEvent::MouseMove)
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
                size_t index = ui->comboBox_oam_slice->currentIndex();
                if (index >= m_slice_vector.size())
                    return QWidget::eventFilter( object, event );
                ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
                int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;
                m_slice_vector[index].oam[m_oam_selected].x += dx;
                if (m_slice_vector[index].oam[m_oam_selected].x < -7)
                    m_slice_vector[index].oam[m_oam_selected].x = -7;
                if (m_slice_vector[index].oam[m_oam_selected].x+1 > m_slice_vector[index].width)
                    m_slice_vector[index].oam[m_oam_selected].x = m_slice_vector[index].width-1;

                m_slice_vector[index].oam[m_oam_selected].y += dy;
                if (m_slice_vector[index].oam[m_oam_selected].y < -sprite_height+1)
                    m_slice_vector[index].oam[m_oam_selected].y = -sprite_height+1;
                if (m_slice_vector[index].oam[m_oam_selected].y+1 > m_slice_vector[index].height)
                    m_slice_vector[index].oam[m_oam_selected].y = m_slice_vector[index].height-1;
                RedrawOamTab();
            }
        }
    }
    if (ui->tabWidget->currentWidget() == ui->tab_Animation)
        AnimationTab_EventFilter(object, event);
    return QWidget::eventFilter( object, event );
}

void MainWindow::TimerFrame()
{
    if (ui->tabWidget->currentWidget() == ui->tab_Animation)
        AnimationTab_FrameTick();
}

void MainWindow::PaletteWindow_Slot_PaletteSelect(int index)
{
    m_palette_set[m_pick_pallete_index/4].c[m_pick_pallete_index & 3] = index;
    QImage image(m_palette_label[m_pick_pallete_index]->width(), m_palette_label[m_pick_pallete_index]->height(), QImage::Format_ARGB32);
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    image.fill(palette[m_palette_set[m_pick_pallete_index/4].c[m_pick_pallete_index & 3]]);
    m_palette_label[m_pick_pallete_index]->setPixmap(QPixmap::fromImage(image));

    if (ui->checkBox_palette_blink->isChecked())
        UpdateBlinkPalette();
}

void MainWindow::PaletteWindow_Slot_BgPaletteSelect(int index)
{
    m_bg_color_index = index;
    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    m_bg_color = palette[m_bg_color_index];
    {
        QImage image(ui->label_global_bg_color->width(), ui->label_global_bg_color->height(), QImage::Format_ARGB32);
        image.fill(m_bg_color);
        ui->label_global_bg_color->setPixmap(QPixmap::fromImage(image));
    }
    RedrawPaletteTab();
}

void MainWindow::PaletteFamiWindow_Slot_PaletteSelect(int index)
{
    //qDebug() << "Pick" << index;
    auto itt = m_palette_cvt_rule.find(m_pick_palette_cvt_color);
    if (itt != m_palette_cvt_rule.end())
        itt->second = index;
    else
        m_palette_cvt_rule.insert(std::make_pair(m_pick_palette_cvt_color, index));
    RedrawPaletteTab();
}

void MainWindow::on_pushButton_clear_colormapping_clicked()
{
    m_palette_cvt_rule.clear();
    RedrawPaletteTab();
}

void MainWindow::on_pushButton_sprite_browse_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Select Image"), QDir::currentPath(), tr("*.jpg *.png"));
     if (file_name.isEmpty())
         return;

     if (!m_project_file_name.isEmpty())
     {
         QFileInfo info(m_project_file_name);
         QDir project_dir(info.dir());
         m_spriteset_file_name = project_dir.relativeFilePath(m_spriteset_file_name);
     }

     ui->lineEdit_sprite_image->setText(m_spriteset_file_name);
     m_spriteset_file_name = file_name;

     QString img_name = m_spriteset_file_name;
     if (!m_project_file_name.isEmpty())
     {
         QFileInfo info(m_project_file_name);
         QDir project_dir(info.dir());
         img_name = project_dir.absoluteFilePath(img_name);
     }

     m_spriteset_original.load(img_name);
     RedrawPaletteTab();
}


void MainWindow::on_checkBox_palette_draw_cvt_stateChanged(int)
{
    RedrawPaletteTab();
}


void MainWindow::on_actionExit_triggered()
{
    this->close();
}


void MainWindow::on_actionOpen_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open project file"), QDir::currentPath(), "*.famiani");
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
    QString file_name = QFileDialog::getSaveFileName(this, "Save project file", QDir::currentPath(), "*.famiani");
    if (file_name.isEmpty())
        return;
    if (file_name.right(8) != ".famiani")
        file_name += ".famiani";
    m_project_file_name = file_name;
    SaveProject(m_project_file_name);
}

void MainWindow::SaveProject(const QString &file_name)
{
    picojson::value json;
    json.set<picojson::object>(picojson::object());
    json.get<picojson::object>()["palette_mode"] = picojson::value( (double)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt() );
    json.get<picojson::object>()["palette_0_0"] = picojson::value( (double)m_palette_set[0].c[0] );
    json.get<picojson::object>()["palette_0_1"] = picojson::value( (double)m_palette_set[0].c[1] );
    json.get<picojson::object>()["palette_0_2"] = picojson::value( (double)m_palette_set[0].c[2] );
    json.get<picojson::object>()["palette_0_3"] = picojson::value( (double)m_palette_set[0].c[3] );

    json.get<picojson::object>()["palette_1_0"] = picojson::value( (double)m_palette_set[1].c[0] );
    json.get<picojson::object>()["palette_1_1"] = picojson::value( (double)m_palette_set[1].c[1] );
    json.get<picojson::object>()["palette_1_2"] = picojson::value( (double)m_palette_set[1].c[2] );
    json.get<picojson::object>()["palette_1_3"] = picojson::value( (double)m_palette_set[1].c[3] );

    json.get<picojson::object>()["palette_2_0"] = picojson::value( (double)m_palette_set[2].c[0] );
    json.get<picojson::object>()["palette_2_1"] = picojson::value( (double)m_palette_set[2].c[1] );
    json.get<picojson::object>()["palette_2_2"] = picojson::value( (double)m_palette_set[2].c[2] );
    json.get<picojson::object>()["palette_2_3"] = picojson::value( (double)m_palette_set[2].c[3] );

    json.get<picojson::object>()["palette_3_0"] = picojson::value( (double)m_palette_set[3].c[0] );
    json.get<picojson::object>()["palette_3_1"] = picojson::value( (double)m_palette_set[3].c[1] );
    json.get<picojson::object>()["palette_3_2"] = picojson::value( (double)m_palette_set[3].c[2] );
    json.get<picojson::object>()["palette_3_3"] = picojson::value( (double)m_palette_set[3].c[3] );

    json.get<picojson::object>()["background_color"] = picojson::value( (double)m_bg_color_index );

    json.get<picojson::object>()["sprite_mode"] = picojson::value( (double)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt() );

    json.get<picojson::object>()["sprite_set"] = picojson::value( m_spriteset_file_name.toUtf8().toBase64().data() );

    json.get<picojson::object>()["palette_blink"] = picojson::value( ui->checkBox_palette_blink->isChecked() ? 1.0 : 0.0 );

    picojson::array items = picojson::array();
    for (auto itt = m_palette_cvt_rule.begin(); itt != m_palette_cvt_rule.end(); ++itt)
    {
            picojson::object item_obj;
            item_obj["r"] = picojson::value( (double)(itt->first & 0xFF) );
            item_obj["g"] = picojson::value( (double)((itt->first >> 8) & 0xFF) );
            item_obj["b"] = picojson::value( (double)((itt->first >> 16) & 0xFF) );
            item_obj["a"] = picojson::value( (double)((itt->first >> 24) & 0xFF) );
            item_obj["i"] = picojson::value( (double)itt->second );
            items.push_back(picojson::value(item_obj));
    }
    json.get<picojson::object>()["color_map"] = picojson::value(items);

    {
        picojson::array items = picojson::array();
        for (size_t n = 0; n < m_slice_vector.size(); ++n)
        {
            picojson::object item_obj;
            item_obj["x"] = picojson::value( (double)m_slice_vector[n].x );
            item_obj["y"] = picojson::value( (double)m_slice_vector[n].y );
            item_obj["w"] = picojson::value( (double)m_slice_vector[n].width );
            item_obj["h"] = picojson::value( (double)m_slice_vector[n].height );
            item_obj["cap"] = picojson::value( m_slice_vector[n].caption.toUtf8().toBase64().data() );
            if (!m_slice_vector[n].oam.empty())
            {
                picojson::array oams = picojson::array();
                for (size_t i = 0; i < m_slice_vector[n].oam.size(); ++i)
                {
                    picojson::object oam_obj;
                    oam_obj["x"] = picojson::value( (double)m_slice_vector[n].oam[i].x );
                    oam_obj["y"] = picojson::value( (double)m_slice_vector[n].oam[i].y );
                    oam_obj["p"] = picojson::value( (double)m_slice_vector[n].oam[i].palette );
                    oam_obj["m"] = picojson::value( (double)m_slice_vector[n].oam[i].mode);
                    oams.push_back(picojson::value(oam_obj));
                }
                item_obj["oam"] = picojson::value(oams);
            }
            items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["slice_area"] = picojson::value(items);
    }


    {
        picojson::array items = picojson::array();
        for (size_t n = 0; n < m_animation.size(); ++n)
        {
            picojson::object item_obj;
            item_obj["name"] = picojson::value( m_animation[n].name.toUtf8().toBase64().data() );
            item_obj["loop"] = picojson::value( m_animation[n].loop );
            if (!m_animation[n].frames.empty())
            {
                picojson::array frames = picojson::array();
                for (size_t i = 0; i < m_animation[n].frames.size(); ++i)
                {
                    picojson::object frame_obj;
                    frame_obj["name"] = picojson::value( m_animation[n].frames[i].name.toUtf8().toBase64().data() );
                    frame_obj["x"] = picojson::value( (double)m_animation[n].frames[i].x );
                    frame_obj["y"] = picojson::value( (double)m_animation[n].frames[i].y );

                    frame_obj["duration_ms"] = picojson::value( (double)m_animation[n].frames[i].duration_ms );
                    frame_obj["bound_box_x"] = picojson::value( (double)m_animation[n].frames[i].bound_box_x );
                    frame_obj["bound_box_y"] = picojson::value( (double)m_animation[n].frames[i].bound_box_y );
                    frame_obj["bound_box_width"] = picojson::value( (double)m_animation[n].frames[i].bound_box_width );
                    frame_obj["bound_box_height"] = picojson::value( (double)m_animation[n].frames[i].bound_box_height );

                    frame_obj["lock_movement"] = picojson::value( m_animation[n].frames[i].lock_movement );
                    frame_obj["lock_direction"] = picojson::value( m_animation[n].frames[i].lock_direction );
                    frame_obj["lock_attack"] = picojson::value( m_animation[n].frames[i].lock_attack );
                    frame_obj["damage_box"] = picojson::value( m_animation[n].frames[i].damage_box );

                    frame_obj["damage_box_x"] = picojson::value( (double)m_animation[n].frames[i].damage_box_x );
                    frame_obj["damage_box_y"] = picojson::value( (double)m_animation[n].frames[i].damage_box_y );
                    frame_obj["damage_box_width"] = picojson::value( (double)m_animation[n].frames[i].damage_box_width );
                    frame_obj["damage_box_height"] = picojson::value( (double)m_animation[n].frames[i].damage_box_height );

                    frames.push_back(picojson::value(frame_obj));
                }
                item_obj["frames"] = picojson::value(frames);
            }
            items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["animation"] = picojson::value(items);
    }

    if (ui->checkBox_palette_blink->isChecked())
    {
        for (size_t n = 0; n < 16*4; ++n)
        {
            QString name = QString("blink_pal_%1").arg(n);
            if (m_blink_palette.enable[n])
                json.get<picojson::object>()[name.toStdString()] = picojson::value( 1.0 );
        }
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

    for (int n = 0; n < 16; ++n)
    {
        QString name = QString("palette_%1_%2").arg(n >> 2).arg(n & 3);
        if (json.contains(name.toStdString()))
            m_palette_set[n >> 2].c[n & 3] = (int)json.get<picojson::object>()[name.toStdString()].get<double>();
    }

    int sprite_mode = SpriteMode_8x8;
    if (json.contains("sprite_mode"))
        sprite_mode = json.get<picojson::object>()["sprite_mode"].get<double>();
    ui->comboBox_sprite_mode->blockSignals(true);
    for (int i = 0; i < ui->comboBox_palette_mode->count(); ++i)
    {
        if (ui->comboBox_sprite_mode->itemData(i).toInt() == sprite_mode)
        {
            ui->comboBox_sprite_mode->setCurrentIndex(i);
            break;
        }
    }
    ui->comboBox_sprite_mode->blockSignals(false);

    if (json.contains("background_color"))
        m_bg_color_index = json.get<picojson::object>()["background_color"].get<double>();

    if (json.contains("palette_blink"))
         ui->checkBox_palette_blink->setChecked(json.get<picojson::object>()["background_color"].get<double>());
    ui->widget_blink_palette->setVisible(ui->checkBox_palette_blink->isChecked());

    if (ui->checkBox_palette_blink->isChecked())
    {
        for (size_t n = 0; n < 16*4; ++n)
        {
            QString name = QString("blink_pal_%1").arg(n);
            if (json.contains(name.toStdString()))
                m_blink_palette.enable[n] = (int)json.get<picojson::object>()[name.toStdString()].get<double>();
            else
                m_blink_palette.enable[n] = 0;
        }
    }

    m_spriteset_file_name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["sprite_set"].get<std::string>().c_str()));
    if (!m_project_file_name.isEmpty())
    {
        QFileInfo info(m_project_file_name);
        QDir project_dir(info.dir());
        m_spriteset_file_name = project_dir.relativeFilePath(m_spriteset_file_name);
    }

    if (!m_spriteset_file_name.isEmpty())
    {
        QString img_name = m_spriteset_file_name;
        if (!m_project_file_name.isEmpty())
        {
            QFileInfo info(m_project_file_name);
            QDir project_dir(info.dir());
            img_name = project_dir.absoluteFilePath(img_name);
        }
        if (!m_spriteset_original.load(img_name))
        {
            QMessageBox::critical(0, "Error", "Can't load image: " + img_name + ", " + file.errorString());
            m_spriteset_file_name.clear();
        } else
        {
            ui->lineEdit_sprite_image->setText(m_spriteset_file_name);
        }
    }

    m_palette_cvt_rule.clear();
    if (json.contains("color_map"))
    {
        picojson::array items = json.get<picojson::object>()["color_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            uint32_t r = (uint32_t)(itt->get<picojson::object>()["r"].get<double>());
            uint32_t g = (uint32_t)(itt->get<picojson::object>()["g"].get<double>());
            uint32_t b = (uint32_t)(itt->get<picojson::object>()["b"].get<double>());
            uint32_t a = (uint32_t)(itt->get<picojson::object>()["a"].get<double>());
            int i = (int)(itt->get<picojson::object>()["i"].get<double>());
            m_palette_cvt_rule.insert(std::make_pair(r | (g<<8) | (b<<16) | (a<<24), i));
        }
    }

    m_slice_vector.clear();
    if (json.contains("slice_area"))
    {
        picojson::array items = json.get<picojson::object>()["slice_area"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            SliceArea area;
            area.id = m_slice_vector.size();
            area.x = (int)(itt->get<picojson::object>()["x"].get<double>());
            area.y = (int)(itt->get<picojson::object>()["y"].get<double>());
            area.width = (int)(itt->get<picojson::object>()["w"].get<double>());
            area.height = (int)(itt->get<picojson::object>()["h"].get<double>());
            area.caption = QString::fromUtf8( QByteArray::fromBase64(itt->get<picojson::object>()["cap"].get<std::string>().c_str()));

            if (itt->contains("oam"))
            {
                picojson::array oams = itt->get<picojson::object>()["oam"].get<picojson::array>();
                for (auto oam_itt = oams.begin(); oam_itt != oams.end(); ++oam_itt)
                {
                    OAM oam;
                    oam.x = (int)(oam_itt->get<picojson::object>()["x"].get<double>());
                    oam.y = (int)(oam_itt->get<picojson::object>()["y"].get<double>());
                    oam.palette = (int)(oam_itt->get<picojson::object>()["p"].get<double>());
                    oam.mode = (int)(oam_itt->get<picojson::object>()["m"].get<double>());
                    area.oam.push_back(oam);
                }
            }
            m_slice_vector.push_back(area);
        }

        ui->comboBox_slice_list->blockSignals(true);
        ui->comboBox_slice_list->clear();
        for (size_t i = 0; i < m_slice_vector.size(); ++i)
            ui->comboBox_slice_list->addItem(m_slice_vector[i].caption, QVariant(m_slice_vector[i].id));
        ui->comboBox_slice_list->blockSignals(false);
    }

    m_animation.clear();
    if (json.contains("animation"))
    {
        picojson::array items = json.get<picojson::object>()["animation"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            Animation animation;
            animation.name = QString::fromUtf8( QByteArray::fromBase64(itt->get<picojson::object>()["name"].get<std::string>().c_str()));
            animation.loop = itt->get<picojson::object>()["loop"].get<bool>();

            if (itt->contains("frames"))
            {
                picojson::array frames = itt->get<picojson::object>()["frames"].get<picojson::array>();
                for (auto frame_itt = frames.begin(); frame_itt != frames.end(); ++frame_itt)
                {
                    AnimationFrame frame;
                    frame.name = QString::fromUtf8( QByteArray::fromBase64(frame_itt->get<picojson::object>()["name"].get<std::string>().c_str()));
                    frame.x = (int)(frame_itt->get<picojson::object>()["x"].get<double>());
                    frame.y = (int)(frame_itt->get<picojson::object>()["y"].get<double>());

                    frame.duration_ms = (int)(frame_itt->get<picojson::object>()["duration_ms"].get<double>());
                    frame.bound_box_x = (int)(frame_itt->get<picojson::object>()["bound_box_x"].get<double>());
                    frame.bound_box_y = (int)(frame_itt->get<picojson::object>()["bound_box_y"].get<double>());
                    frame.bound_box_width = (int)(frame_itt->get<picojson::object>()["bound_box_width"].get<double>());
                    frame.bound_box_height = (int)(frame_itt->get<picojson::object>()["bound_box_height"].get<double>());

                    frame.lock_movement = frame_itt->get<picojson::object>()["lock_movement"].get<bool>();
                    frame.lock_direction = frame_itt->get<picojson::object>()["lock_direction"].get<bool>();
                    frame.lock_attack = frame_itt->get<picojson::object>()["lock_attack"].get<bool>();
                    frame.damage_box = frame_itt->get<picojson::object>()["damage_box"].get<bool>();

                    frame.damage_box_x = (int)(frame_itt->get<picojson::object>()["damage_box_x"].get<double>());
                    frame.damage_box_y = (int)(frame_itt->get<picojson::object>()["damage_box_y"].get<double>());
                    frame.damage_box_width = (int)(frame_itt->get<picojson::object>()["damage_box_width"].get<double>());
                    frame.damage_box_height = (int)(frame_itt->get<picojson::object>()["damage_box_height"].get<double>());

                    animation.frames.push_back(frame);
                }
            }
            m_animation.push_back(animation);
        }
    }


    m_slice_start_point_active = false;
    m_slice_selected = -1;
    on_comboBox_palette_mode_currentIndexChanged(palette_mode);
    RedrawSliceTab();
    FullUpdateOamTab();
    AnimationTab_Reload();
}

void MainWindow::RedrawSliceTab()
{
    if (m_spriteset_original.isNull())
        return;

    QImage image = m_spriteset_original.scaled(m_spriteset_original.width()*m_slice_tab_zoom, m_spriteset_original.height()*m_slice_tab_zoom);

    for (int y = 0; y < image.height(); ++y)
    {
        uint8_t* data = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x)
        {
            if (data[x*4 + 3] < 128)
                *((uint32_t*)(data + x*4)) = m_bg_color;
        }
    }

    {
        QPainter painter(&image);
        if (m_slice_start_point_active)
        {
            painter.setBrush(Qt::NoBrush);
            //painter.setBrush(QColor(0xFFFFFFFF));
            painter.setPen(QColor(0xFFFFFFFF));
            painter.drawRect(m_slice_start_point.x()*m_slice_tab_zoom, m_slice_start_point.y()*m_slice_tab_zoom,
                             (m_slice_end_point.x() - m_slice_start_point.x())*m_slice_tab_zoom,
                             (m_slice_end_point.y() - m_slice_start_point.y())*m_slice_tab_zoom
                             );
        }
        static const int fs = 12;
        painter.setFont(QFont("Arial", fs));
        for (size_t n = 0; n < m_slice_vector.size(); ++n)
        {
            painter.setBrush(Qt::NoBrush);
            if (n == m_slice_selected)
            {
                painter.setPen(QColor(0xFFFFFFFF));
                if (m_slice_selected_corner == 0)
                {
                    painter.drawRect(m_slice_vector[n].x*m_slice_tab_zoom, m_slice_vector[n].y*m_slice_tab_zoom,
                                     s_corner_size*m_slice_tab_zoom, s_corner_size*m_slice_tab_zoom);
                } else if (m_slice_selected_corner == 1)
                {
                    painter.drawRect((m_slice_vector[n].x + m_slice_vector[n].width - s_corner_size)*m_slice_tab_zoom, m_slice_vector[n].y*m_slice_tab_zoom,
                                     s_corner_size*m_slice_tab_zoom, s_corner_size*m_slice_tab_zoom);
                } else if (m_slice_selected_corner == 2)
                {
                    painter.drawRect(m_slice_vector[n].x*m_slice_tab_zoom, (m_slice_vector[n].y + m_slice_vector[n].height - s_corner_size)*m_slice_tab_zoom,
                                     s_corner_size*m_slice_tab_zoom, s_corner_size*m_slice_tab_zoom);
                } else if (m_slice_selected_corner == 3)
                {
                    painter.drawRect((m_slice_vector[n].x + m_slice_vector[n].width - s_corner_size)*m_slice_tab_zoom,
                                     (m_slice_vector[n].y + m_slice_vector[n].height - s_corner_size)*m_slice_tab_zoom,
                                     s_corner_size*m_slice_tab_zoom, s_corner_size*m_slice_tab_zoom);
                }


            } else
                painter.setPen(QColor(0xFFFFFF00));
            painter.drawRect(m_slice_vector[n].x*m_slice_tab_zoom, m_slice_vector[n].y*m_slice_tab_zoom,
                             m_slice_vector[n].width*m_slice_tab_zoom,
                             m_slice_vector[n].height*m_slice_tab_zoom);

            painter.setPen(QColor(0xFF000000));
            painter.drawText(m_slice_vector[n].x*m_slice_tab_zoom+2+1, m_slice_vector[n].y*m_slice_tab_zoom+fs, m_slice_vector[n].caption);
            painter.drawText(m_slice_vector[n].x*m_slice_tab_zoom+2-1, m_slice_vector[n].y*m_slice_tab_zoom+fs, m_slice_vector[n].caption);
            painter.drawText(m_slice_vector[n].x*m_slice_tab_zoom+2, m_slice_vector[n].y*m_slice_tab_zoom+fs+1, m_slice_vector[n].caption);
            painter.drawText(m_slice_vector[n].x*m_slice_tab_zoom+2, m_slice_vector[n].y*m_slice_tab_zoom+fs-1, m_slice_vector[n].caption);
            painter.setPen(QColor(0xFFFFFFFF));
            painter.drawText(m_slice_vector[n].x*m_slice_tab_zoom+2, m_slice_vector[n].y*m_slice_tab_zoom+fs, m_slice_vector[n].caption);
        }
    }

    s_slice_tab_render->setPixmap(QPixmap::fromImage(image));
    s_slice_tab_render->resize(image.width(), image.height());
    s_slice_tab_render->setMaximumSize(image.width(), image.height());
    s_slice_tab_render->setMinimumSize(image.width(), image.height());
}

void MainWindow::on_tabWidget_currentChanged(int)
{
    if (ui->tabWidget->currentWidget() == ui->tab_slice)
    {
        RedrawSliceTab();
    }
    if (ui->tabWidget->currentWidget() == ui->tab_OAM)
    {
        FullUpdateOamTab();
    }
    if (ui->tabWidget->currentWidget() == ui->tab_Animation)
    {
        AnimationTab_Reload();
    }
}


void MainWindow::on_lineEdit_slice_name_editingFinished()
{
    QString text = ui->lineEdit_slice_name->text();
    if ((size_t)m_slice_selected >= m_slice_vector.size())
        return;

    if (m_slice_vector[m_slice_selected].caption == text)
        return;

    bool eq = false;
    for (size_t n = 0; n < m_slice_vector.size(); ++n)
    {
        if (n == m_slice_selected)
            continue;
        if (m_slice_vector[n].caption == text)
        {
            eq = true;
            break;
        }
    }
    if (eq)
    {
        ui->lineEdit_slice_name->text() = m_slice_vector[m_slice_selected].caption;
    } else
    {
        m_slice_vector[m_slice_selected].caption = text;
    }
}

void MainWindow::FullUpdateOamTab()
{
    m_oam_selected = -1;
    ui->comboBox_oam_slice->clear();
    int select_item = 0;
    ui->comboBox_oam_slice->blockSignals(true);
    for (size_t n = 0; n < m_slice_vector.size(); ++n)
    {
        if (m_slice_vector[n].caption == m_oam_slice_last)
            select_item = 0;
        ui->comboBox_oam_slice->addItem(m_slice_vector[n].caption);
    }
    if (m_slice_vector.size())
        ui->comboBox_oam_slice->setCurrentIndex(select_item);
    ui->comboBox_oam_slice->blockSignals(false);
    RedrawOamTab();
}

void MainWindow::RedrawOamTab()
{
    if (ui->comboBox_oam_slice->count() == 0)
        return;
    if (m_spriteset_indexed.isNull())
        return;
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;

    ui->label_oam_couner->setText(QString("%1").arg(m_slice_vector[index].oam.size()));

    ESpriteMode sprite_mode = (ESpriteMode)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt();
    int sprite_height = sprite_mode == SpriteMode_8x8 ? 8 : 16;

    bool blink = ui->checkBox_palette_blink->isChecked();

    QImage cut = m_spriteset_indexed.copy(m_slice_vector[index].x, m_slice_vector[index].y, m_slice_vector[index].width, m_slice_vector[index].height);
    QImage render(cut.width()*6, cut.height(), QImage::Format_ARGB32);
    render.fill(m_bg_color);
    {
        QPainter painter(&render);
        painter.drawImage(0, 0, cut);
    }

    const uint32_t* palette = GetPalette((EPalette)ui->comboBox_palette_mode->itemData(ui->comboBox_palette_mode->currentIndex()).toInt());
    for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
    {
        int pindex = m_slice_vector[index].oam[n].palette;
        Palette pal = m_palette_set[pindex];
        for (int y = 0; y < sprite_height; ++y)
        {
            int yline = y + m_slice_vector[index].oam[n].y;
            if (yline < 0 || yline > cut.height())
                continue;
            const uint32_t* scan_line = (const uint32_t*)cut.scanLine(yline);
            uint32_t* render_line = (uint32_t*)render.scanLine(yline);
            for (int x = 0; x < 8; ++x)
            {
                int xp = x + m_slice_vector[index].oam[n].x;
                if (xp < 0 || xp > cut.width())
                    continue;
                if (scan_line[xp] == m_bg_color)
                    continue;
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
                        render_line[xp + cut.width()*5] = m_blink_palette.color[pindex*16+found];
                        render_line[xp + cut.width()*(pindex+1)] = m_blink_palette.color[pindex*16+found];
                    }
                } else
                {
                    if (color == palette[pal.c[1]])
                    {
                        render_line[xp + cut.width()*5] = color;
                        render_line[xp + cut.width()*(pindex+1)] = color;
                    } else if (color == palette[pal.c[2]])
                    {
                        render_line[xp + cut.width()*5] = color;
                        render_line[xp + cut.width()*(pindex+1)] = color;
                    } else if (color == palette[pal.c[3]])
                    {
                        render_line[xp + cut.width()*5] = color;
                        render_line[xp + cut.width()*(pindex+1)] = color;
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
                        render_line[xp + cut.width()*5] = palette[pal.c[best_c]];
                        render_line[xp + cut.width()*(pindex+1)] = palette[pal.c[best_c]];
                    }
                }
            }
        }
    }

    QImage image = render.scaled(render.width()*m_oam_tab_zoom, render.height()*m_oam_tab_zoom);
    {
        QPainter painter(&image);
        painter.setBrush(Qt::NoBrush);
        for (size_t n = 0; n < m_slice_vector[index].oam.size(); ++n)
        {
            if (m_slice_vector[index].oam[n].palette == 0)
                painter.setPen(QColor(0xFF00FF00));
            else if (m_slice_vector[index].oam[n].palette == 1)
                painter.setPen(QColor(0xFFFF0000));
            else if (m_slice_vector[index].oam[n].palette == 2)
                painter.setPen(QColor(0xFF0000FF));
            else if (m_slice_vector[index].oam[n].palette == 3)
                painter.setPen(QColor(0xFFFFFF00));
            if (n == m_oam_selected)
                painter.setPen(QColor(0xFFFFFFFF));
            int dx = (m_slice_vector[index].oam[n].palette+1) * cut.width();
            painter.drawRect(m_slice_vector[index].oam[n].x*m_oam_tab_zoom, m_slice_vector[index].oam[n].y*m_oam_tab_zoom, 8*m_oam_tab_zoom, sprite_height*m_oam_tab_zoom);
            painter.drawRect((m_slice_vector[index].oam[n].x + dx)*m_oam_tab_zoom, m_slice_vector[index].oam[n].y*m_oam_tab_zoom, 8*m_oam_tab_zoom, sprite_height*m_oam_tab_zoom);
        }
    }


    s_oam_tab_render->setPixmap(QPixmap::fromImage(image));
    s_oam_tab_render->setMinimumSize(image.size());
    s_oam_tab_render->setMaximumSize(image.size());
}

void MainWindow::on_comboBox_oam_slice_currentIndexChanged(int index)
{
    m_oam_slice_last = ui->comboBox_oam_slice->itemText(index);
    m_oam_selected =-1;
    RedrawOamTab();
}

void MainWindow::on_radioButton_oam_pal0_clicked()
{
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;
    if (m_oam_selected < 0)
        return;
    m_slice_vector[index].oam[m_oam_selected].palette = 0;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal1_clicked()
{
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;
    if (m_oam_selected < 0)
        return;
    m_slice_vector[index].oam[m_oam_selected].palette = 1;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal2_clicked()
{
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;
    if (m_oam_selected < 0)
        return;
    m_slice_vector[index].oam[m_oam_selected].palette = 2;
    RedrawOamTab();
}


void MainWindow::on_radioButton_oam_pal3_clicked()
{
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;
    if (m_oam_selected < 0)
        return;
    m_slice_vector[index].oam[m_oam_selected].palette = 3;
    RedrawOamTab();
}


void MainWindow::on_checkBox_oam_fill_any_color_clicked()
{
    size_t index = ui->comboBox_oam_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;
    if (m_oam_selected < 0)
        return;
    m_slice_vector[index].oam[m_oam_selected].mode = ui->checkBox_oam_fill_any_color->isChecked();
    RedrawOamTab();
}


void MainWindow::on_pushButton_slice_add_clicked()
{
}

void MainWindow::on_pushButton_slice_delete_clicked()
{
    if (m_slice_selected < 0)
        return;

    m_slice_vector.erase(m_slice_vector.begin() + m_slice_selected);
    ui->comboBox_slice_list->blockSignals(true);
    ui->comboBox_slice_list->removeItem(m_slice_selected);
    ui->comboBox_slice_list->blockSignals(false);

    ui->lineEdit_slice_name->setText("");
    ui->lineEdit_slice_name->setEnabled(false);

    m_slice_selected = -1;
    RedrawSliceTab();
}

void MainWindow::on_comboBox_slice_list_currentIndexChanged(int index)
{
    m_slice_selected = index;
    ui->lineEdit_slice_name->setText(m_slice_vector[m_slice_selected].caption);
    ui->lineEdit_slice_name->setEnabled(true);
    RedrawSliceTab();
}



void MainWindow::on_checkBox_palette_blink_clicked()
{
    ui->widget_blink_palette->setVisible(ui->checkBox_palette_blink->isChecked());
    if (ui->checkBox_palette_blink->isChecked())
        UpdateBlinkPalette();
}

