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

static QLabel* s_animation_tab_render;
static int s_animation_tab_zoom = 4;
static qint64 s_animation_start_time = 0;
static qint64 s_animation_time = 0;

void MainWindow::AnimationTab_Init()
{
    m_oam_tab_zoom = 8;
    s_animation_tab_render = new QLabel();
    s_animation_tab_render->move(0, 0);
    ui->scrollArea_Animation->setWidget(s_animation_tab_render);
    s_animation_tab_render->installEventFilter(this);
    s_animation_tab_render->setMouseTracking(true);
    ui->scrollArea_Animation->installEventFilter(this);
}

void MainWindow::AnimationTab_Reload()
{
    int index = ui->combo_animation->currentIndex();
    ui->combo_animation->blockSignals(true);
    ui->combo_animation->clear();
    for (size_t n = 0; n < m_animation.size(); ++n)
        ui->combo_animation->addItem(m_animation[n].name);
    ui->combo_animation->blockSignals(false);

    ui->combo_animation_slice->blockSignals(true);
    ui->combo_animation_slice->clear();
    for (size_t n = 0; n < m_slice_vector.size(); ++n)
        ui->combo_animation_slice->addItem(m_slice_vector[n].caption);
    ui->combo_animation_slice->blockSignals(false);

    if (index >= ui->combo_animation->count())
        index = 0;
    on_combo_animation_currentIndexChanged(index);
    s_animation_start_time = m_elapsed_timer.elapsed();
}

void MainWindow::on_combo_animation_currentIndexChanged(int index)
{
    if (index >= m_animation.size())
        return;
    ui->edit_animation_name->setText(m_animation[index].name);
    ui->edit_animation_frames->setText(QString("%1").arg(m_animation[index].frames.size()));
    ui->check_animation_loop->setChecked(m_animation[index].loop);
    ui->slider_animation_frame->setRange(0, m_animation[index].frames.size() - 1);
    ui->label_animation_frame->setText(QString("%1").arg(ui->slider_animation_frame->value()));
    AnimationTab_UpdateFrame();
    AnimationTab_Redraw();
}

void MainWindow::on_btn_animation_add_clicked()
{
    QString avaliable_name;
    ui->combo_animation->blockSignals(true);
    for (int i = 0; i < 999; ++i)
    {
        QString name = QString("animation-%1").arg(i);
        bool avaliable = true;
        for (size_t n = 0; n < m_animation.size(); ++n)
        {
            if (m_animation[n].name == name)
            {
                avaliable = false;
                break;
            }
        }
        if (avaliable)
        {
            avaliable_name = name;
            break;
        }
    }
    ui->combo_animation->blockSignals(false);

    if (!avaliable_name.isEmpty())
    {
        ui->combo_animation->addItem(avaliable_name);
        Animation anim;
        anim.name = avaliable_name;
        anim.loop = false;
        m_animation.push_back(anim);
        ui->combo_animation->blockSignals(true);
        ui->combo_animation->setCurrentIndex(m_animation.size()-1);
        ui->combo_animation->blockSignals(false);
        on_combo_animation_currentIndexChanged(m_animation.size()-1);
    }
}

void MainWindow::on_btn_animation_delete_clicked()
{
    if (m_animation.size() == 0)
        return;
}

void MainWindow::on_edit_animation_name_editingFinished()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    QString name = ui->edit_animation_name->text();
    for (size_t n = 0; n < m_animation.size(); ++n)
    {
        if (n == index)
            continue;
        if (m_animation[n].name == name)
        {
            ui->edit_animation_name->setText(m_animation[index].name);
            return;
        }
    }
    ui->combo_animation->setItemText(index, name);
    m_animation[index].name = name;
}

void MainWindow::on_edit_animation_frames_editingFinished()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frames = ui->edit_animation_frames->text().toInt();
    ui->slider_animation_frame->setRange(0,  frames-1);
    ui->label_animation_frame->setText(QString("%1").arg(ui->slider_animation_frame->value()));
    m_animation[index].frames.resize(frames);
    AnimationTab_UpdateFrame();
}

void MainWindow::on_check_animation_loop_clicked()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    m_animation[index].loop = ui->check_animation_loop->isChecked();
}

void MainWindow::AnimationTab_UpdateFrame()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;

    for (int i = 0; i < ui->combo_animation_slice->count(); ++i)
    {
        if (ui->combo_animation_slice->itemText(i) == m_animation[index].frames[frame].name)
        {
            ui->combo_animation_slice->blockSignals(true);
            ui->combo_animation_slice->setCurrentIndex(i);
            ui->combo_animation_slice->blockSignals(false);
            break;
        }
    }

    ui->edit_frame_x->setText(QString("%1").arg(m_animation[index].frames[frame].x));
    ui->edit_frame_y->setText(QString("%1").arg(m_animation[index].frames[frame].y));
    ui->edit_frame_duration->setText(QString("%1").arg(m_animation[index].frames[frame].duration_ms));
    ui->check_frame_lock_movement->setChecked(m_animation[index].frames[frame].lock_movement);
    ui->check_frame_lock_direction->setChecked(m_animation[index].frames[frame].lock_direction);
    ui->check_frame_lock_attack->setChecked(m_animation[index].frames[frame].lock_attack);
    ui->check_frame_damage_box->setChecked(m_animation[index].frames[frame].damage_box);
}

void MainWindow::on_slider_animation_frame_valueChanged(int value)
{
    ui->label_animation_frame->setText(QString("%1").arg(ui->slider_animation_frame->value()));
    AnimationTab_UpdateFrame();
    AnimationTab_Redraw();
}

void MainWindow::on_combo_animation_slice_currentIndexChanged(int combo_index)
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].name = ui->combo_animation_slice->itemText(combo_index);
    AnimationTab_Redraw();
}


void MainWindow::on_edit_frame_x_editingFinished()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].x = ui->edit_frame_x->text().toInt();
    AnimationTab_Redraw();
}


void MainWindow::on_edit_frame_y_editingFinished()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].y = ui->edit_frame_y->text().toInt();
    AnimationTab_Redraw();
}


void MainWindow::on_edit_frame_duration_editingFinished()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].duration_ms = ui->edit_frame_duration->text().toInt();
}


void MainWindow::on_check_frame_lock_movement_clicked()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].lock_movement = ui->check_frame_lock_movement->isChecked();
}


void MainWindow::on_check_frame_lock_direction_clicked()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].lock_direction = ui->check_frame_lock_direction->isChecked();
}


void MainWindow::on_check_frame_lock_attack_clicked()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].lock_attack = ui->check_frame_lock_attack->isChecked();
}

void MainWindow::on_check_frame_damage_box_clicked()
{
    if (m_animation.size() == 0)
        return;
    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;
    m_animation[index].frames[frame].damage_box = ui->check_frame_damage_box->isChecked();
    AnimationTab_Redraw();
}

void MainWindow::on_btn_animation_play_clicked()
{
    if (ui->btn_animation_play->isChecked())
    {
        ui->btn_animation_play->setIcon(QIcon(":/res/resource/pause.png"));
        s_animation_start_time = m_elapsed_timer.elapsed();
    } else
        ui->btn_animation_play->setIcon(QIcon(":/res/resource/play.png"));
}

void MainWindow::AnimationTab_FrameTick()
{
    if (!ui->btn_animation_play->isChecked())
        return;

    int index = ui->combo_animation->currentIndex();
    int frame = ui->slider_animation_frame->value();
    if (frame >= m_animation[index].frames.size())
        return;

    qint64 t = m_elapsed_timer.elapsed();
    s_animation_time += t - s_animation_start_time;
    s_animation_start_time = t;

    if (s_animation_time >= m_animation[index].frames[frame].duration_ms)
    {
        s_animation_time -= m_animation[index].frames[frame].duration_ms;
        frame = (frame + 1) % m_animation[index].frames.size();
        ui->slider_animation_frame->setValue(frame);
    }
}

void MainWindow::AnimationTab_Redraw()
{
    QImage image(128, 128, QImage::Format_ARGB32);
    {
        QPainter painter(&image);
        painter.setPen(QPen(QColor(196, 255, 196, 255)));
        painter.setBrush(QBrush(QColor(196, 255, 196, 255)));
        painter.drawRect(0, 0, image.width()/2, image.height()/2);

        painter.setPen(QPen(QColor(196, 196, 255, 255)));
        painter.setBrush(QBrush(QColor(196, 196, 255, 255)));
        painter.drawRect(image.width()/2, 0, image.width()/2, image.height()/2);

        painter.setPen(QPen(QColor(64, 0, 0, 255)));
        painter.setBrush(QBrush(QColor(96, 0, 0, 255)));
        painter.drawRect(0, image.height()/2, image.width(), image.height()/2);

        int index = ui->combo_animation->currentIndex();
        int frame = ui->slider_animation_frame->value();
        size_t slice_index = 0;
        if ( !m_spriteset_indexed_alpha.isNull()
             && index < m_animation.size() && frame < m_animation[index].frames.size())
        {
            for (size_t i = 0; i < m_slice_vector.size(); ++i)
            {
                if (m_slice_vector[i].caption == m_animation[index].frames[frame].name)
                {
                    slice_index = i;
                    break;
                }
            }
            painter.drawImage(64 + m_animation[index].frames[frame].x, 64 + m_animation[index].frames[frame].y,
                              m_spriteset_indexed_alpha,
                              m_slice_vector[slice_index].x, m_slice_vector[slice_index].y,
                              m_slice_vector[slice_index].width, m_slice_vector[slice_index].height);
        }
    }

    QImage scale = image.scaled(image.width()*s_animation_tab_zoom, image.height()*s_animation_tab_zoom);
    s_animation_tab_render->setPixmap(QPixmap::fromImage(scale));
    s_animation_tab_render->setMinimumSize(scale.size());
    s_animation_tab_render->setMaximumSize(scale.size());
}
