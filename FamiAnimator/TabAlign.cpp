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

static QLabel* s_align_tab_render = 0;
static int s_align_tab_zoom = 4;
static int s_grab_x = -1;
static int s_grab_y = -1;

void MainWindow::AlignTab_Init()
{
    s_align_tab_render = new QLabel();
    s_align_tab_render->move(0, 0);
    ui->scrollArea_Align->setWidget(s_align_tab_render);
    s_align_tab_render->installEventFilter(this);
    s_align_tab_render->setMouseTracking(true);
    ui->scrollArea_Align->installEventFilter(this);
}

void MainWindow::AlignTab_Reload()
{
    ui->comboBox_align_slice->blockSignals(true);
    ui->comboBox_align_slice->clear();
    for (size_t n = 0; n < m_slice_vector.size(); ++n)
        ui->comboBox_align_slice->addItem(m_slice_vector[n].caption);
    ui->comboBox_align_slice->blockSignals(false);
    AlignTab_Redraw();
}

void MainWindow::AlignTab_Redraw()
{
    int index = ui->comboBox_align_slice->currentIndex();
    if (index >= m_slice_vector.size())
        return;

    int max_width = 0;
    int max_height = 0;
    for (size_t n = 0; n < m_slice_vector.size(); ++n)
    {
        if (m_slice_vector[n].width > max_width)
            max_width = m_slice_vector[n].width;
        if (m_slice_vector[n].height > max_height)
            max_height = m_slice_vector[n].height;
    }

    QImage image(max_width*2, max_height*2, QImage::Format_ARGB32);
    image.fill(m_bg_color);
    {
        QPainter painter(&image);
        painter.drawImage(max_width+m_slice_vector[index].dx, max_height+m_slice_vector[index].dy,
                          m_spriteset_original.copy(m_slice_vector[index].x, m_slice_vector[index].y, m_slice_vector[index].width, m_slice_vector[index].height));
    }

    QImage scale = image.scaled(image.width()*s_align_tab_zoom, image.height()*s_align_tab_zoom);
    {
        QPainter painter(&scale);
        painter.setPen(0xFF00FF00);
        painter.drawLine(0, max_height*s_align_tab_zoom, scale.width()-1, max_height*s_align_tab_zoom);
        painter.drawLine(max_width*s_align_tab_zoom, 0, max_width*s_align_tab_zoom, scale.height()-1);
    }
    s_align_tab_render->setPixmap(QPixmap::fromImage(scale));
    s_align_tab_render->setMinimumSize(scale.size());
    s_align_tab_render->setMaximumSize(scale.size());
}

void MainWindow::AlignTab_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->scrollArea_Align)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                s_align_tab_zoom ++;
                if (s_align_tab_zoom > 16)
                    s_align_tab_zoom = 16;
                else
                    AlignTab_Redraw();
            }
            if(key->key() == Qt::Key_Minus)
            {
                s_align_tab_zoom --;
                if (s_align_tab_zoom < 1)
                    s_align_tab_zoom = 1;
                else
                    AlignTab_Redraw();
            }
        }
    }

    if (object == s_align_tab_render)
    {
        int index = ui->comboBox_align_slice->currentIndex();
        if (index >= m_slice_vector.size() || m_spriteset_indexed_alpha.isNull())
            return;
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouse_event = (QMouseEvent*)event;
            int x = mouse_event->x() / s_align_tab_zoom;
            int y = mouse_event->y() / s_align_tab_zoom;

            if (event->type() == QEvent::MouseButtonPress && mouse_event->button() == Qt::LeftButton)
            {
                s_grab_x = x;
                s_grab_y = y;
            } else if (event->type() == QEvent::MouseButtonRelease)
                s_grab_x = -1;
            else if (s_grab_x >= 0)
            {
                if (s_grab_x != x || s_grab_y != y)
                {
                    m_slice_vector[index].dx -= s_grab_x - x;
                    m_slice_vector[index].dy -= s_grab_y - y;
                    s_grab_x = x;
                    s_grab_y = y;
                    ui->edit_align_dx->setText(QString("%1").arg(m_slice_vector[index].dx));
                    ui->edit_align_dy->setText(QString("%1").arg(m_slice_vector[index].dy));
                    AlignTab_Redraw();
                }
            }
        }
    }
}

void MainWindow::on_comboBox_align_slice_currentIndexChanged(int)
{
    int index = ui->comboBox_align_slice->currentIndex();
    if (index >= m_slice_vector.size() || m_spriteset_indexed_alpha.isNull())
        return;
    ui->edit_align_dx->setText(QString("%1").arg(m_slice_vector[index].dx));
    ui->edit_align_dy->setText(QString("%1").arg(m_slice_vector[index].dy));
    AlignTab_Redraw();
}

void MainWindow::on_edit_align_dx_editingFinished()
{
    int index = ui->comboBox_align_slice->currentIndex();
    if (index >= m_slice_vector.size() || m_spriteset_indexed_alpha.isNull())
        return;
    m_slice_vector[index].dx = ui->edit_align_dx->text().toInt();
    AlignTab_Redraw();
}

void MainWindow::on_edit_align_dy_editingFinished()
{
    int index = ui->comboBox_align_slice->currentIndex();
    if (index >= m_slice_vector.size() || m_spriteset_indexed_alpha.isNull())
        return;
    m_slice_vector[index].dy = ui->edit_align_dy->text().toInt();
    AlignTab_Redraw();
}







