#include "dialogpickfamipalette.h"
#include "ui_dialogpickfamipalette.h"

#include <set>

#include <QPainter>
#include <QImage>
#include <QMouseEvent>

static const int g_cell_size = 48;
static const int g_cell_width = 4;
static const int g_cell_height = 4;

DialogPickFamiPalette::DialogPickFamiPalette(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPickFamiPalette)
{
    ui->setupUi(this);
    m_index = 0;
    m_palette = 0;
    m_palette_set = 0;
    m_original_color = 0;
    ui->label_palette->setMinimumSize(g_cell_width*g_cell_size, g_cell_height*g_cell_size);
    ui->label_palette->installEventFilter(this);
}

void DialogPickFamiPalette::SetPalette(const uint32_t* palette, const Palette* palette_set, bool enable_blink_palette, const BlinkPalette &blink_palette)
{
    m_palette = palette;
    m_palette_set = palette_set;
    m_enable_blink_palette = enable_blink_palette;
    m_blink_palette = blink_palette;
}

void DialogPickFamiPalette::SetOriginalColor(uint32_t color)
{
    m_original_color = color;
}

void DialogPickFamiPalette::SetPaletteIndex(int index)
{
    m_index = index;
}

void DialogPickFamiPalette::UpdatePalette()
{
    if (m_palette == 0)
        return;
    int cell_count = m_enable_blink_palette ? 16 : 4;
    QImage image(cell_count*g_cell_size, g_cell_height*g_cell_size, QImage::Format_ARGB32);
    if (m_enable_blink_palette)
    {
        QPainter painter(&image);
        for (int y = 0; y < g_cell_height; ++y)
        {
            for (int x = 0; x < 16; ++x)
            {
                uint32_t color = m_blink_palette.color[x + y*16];
                painter.setBrush(QBrush(QColor(color)));
                if (y*cell_count + x == m_index)
                    painter.setPen(QColor(0xFFFFFFFF));
                else
                    painter.setPen(QColor(0xFF000000));
                painter.drawRect(x*g_cell_size, y*g_cell_size, g_cell_size-1, g_cell_size-1);
                if (m_blink_palette.enable[x + y*16] == 0)
                {
                    painter.setPen(Qt::red);
                    painter.drawLine(x*g_cell_size, y*g_cell_size, x*g_cell_size+g_cell_size, y*g_cell_size+g_cell_size);
                    painter.drawLine(x*g_cell_size+g_cell_size, y*g_cell_size, x*g_cell_size, y*g_cell_size+g_cell_size);
                }
            }
        }
    } else
    {
        QPainter painter(&image);
        for (int y = 0; y < g_cell_height; ++y)
        {
            for (int x = 0; x < g_cell_width; ++x)
            {
                int index = m_palette_set[y].c[x];
                uint32_t c = m_palette[index];
                painter.setBrush(QBrush(QColor(c)));
                if (y*g_cell_width + x == m_index)
                    painter.setPen(QColor(0xFFFFFFFF));
                else
                    painter.setPen(QColor(0xFF000000));
                painter.drawRect(x*g_cell_size, y*g_cell_size, g_cell_size-1, g_cell_size-1);
            }
        }
    }
    ui->label_palette->setPixmap(QPixmap::fromImage(image));

    QImage img(ui->label_palette_sample->width(),ui->label_palette_sample->height(), QImage::Format_ARGB32);
    img.fill(QColor(m_original_color));
    ui->label_palette_sample->setPixmap(QPixmap::fromImage(img));
    //ui->label_color_code->setText(QString("0x%1").arg(m_index, 2, 16 ));
}

bool DialogPickFamiPalette::eventFilter( QObject* object, QEvent* event )
{
    if( object == ui->label_palette &&
            (event->type() == QEvent::MouseButtonPress ||
             event->type() == QEvent::MouseMove) )
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int cell_count = m_enable_blink_palette ? 16 : 4;
            int x = mouse_event->x() / g_cell_size;
            int y = mouse_event->y() / g_cell_size;
            if (x >= cell_count) x = cell_count-1;
            if (y >= g_cell_height) y = g_cell_height-1;
            int index = x + y*cell_count;
            if (index != m_index)
            {
                m_index = index;
                UpdatePalette();
            }
        }
    }

    if(object == ui->label_palette && event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if (mouse_event->button() == Qt::LeftButton)
        {
            emit SignalPaletteSelect(m_index);
            close();
        }
    }
    return QWidget::eventFilter( object, event );
}

DialogPickFamiPalette::~DialogPickFamiPalette()
{
    delete ui;
}

void DialogPickFamiPalette::on_btn_cancel_clicked()
{
    this->close();
}
