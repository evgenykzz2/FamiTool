#include "dialogpickpalette.h"
#include "ui_dialogpickpalette.h"

#include <set>

#include <QPainter>
#include <QImage>
#include <QMouseEvent>

static const int g_cell_size = 48;
static const int g_cell_width = 16;
static const int g_cell_height = 4;

DialogPickPalette::DialogPickPalette(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPickPalette)
{
    ui->setupUi(this);
    m_index = 0;
    m_palette = 0;
    ui->label_palette->setMinimumSize(g_cell_width*g_cell_size, g_cell_height*g_cell_size);
    ui->label_palette->installEventFilter( this );
}

void DialogPickPalette::SetPalette(const uint32_t* palette)
{
    m_palette = palette;
}

void DialogPickPalette::SetPaletteIndex(int index)
{
    m_index = index;
}

void DialogPickPalette::UpdatePalette()
{
    if (m_palette == 0)
        return;
    QImage image(g_cell_width*g_cell_size, g_cell_height*g_cell_size, QImage::Format_ARGB32);
    {
        QPainter painter(&image);
        for (int y = 0; y < g_cell_height; ++y)
        {
            for (int x = 0; x < g_cell_width; ++x)
            {
                int index = x + y*g_cell_width;
                uint32_t c = m_palette[index];
                painter.setBrush(QBrush(QColor(c)));
                if (index == m_index)
                    painter.setPen(QColor(0xFFFFFFFF));
                else
                    painter.setPen(QColor(0xFF000000));
                painter.drawRect(x*g_cell_size, y*g_cell_size, g_cell_size-1, g_cell_size-1);
            }
        }
    }
    ui->label_palette->setPixmap(QPixmap::fromImage(image));

    QImage img(ui->label_palette_sample->width(),ui->label_palette_sample->height(), QImage::Format_ARGB32);
    uint32_t c = m_palette[m_index];
    img.fill(QColor(c));
    ui->label_palette_sample->setPixmap(QPixmap::fromImage(img));
    ui->label_color_code->setText(QString("0x%1").arg(m_index, 2, 16 ));
}

bool DialogPickPalette::eventFilter( QObject* object, QEvent* event )
{
    if( object == ui->label_palette &&
            (event->type() == QEvent::MouseButtonPress ||
             event->type() == QEvent::MouseMove) )
    {
        QMouseEvent* mouse_event = (QMouseEvent*)event;
        if ( (int)(mouse_event->buttons() & Qt::LeftButton) != 0 )
        {
            int x = mouse_event->x() / g_cell_size;
            int y = mouse_event->y() / g_cell_size;
            if (x >= g_cell_width) x = g_cell_width-1;
            if (y >= g_cell_height) y = g_cell_height-1;
            int index = x + y*g_cell_width;
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

DialogPickPalette::~DialogPickPalette()
{
    delete ui;
}

void DialogPickPalette::on_btn_cancel_clicked()
{
    this->close();
}
