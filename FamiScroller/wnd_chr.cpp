#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include "QMouseEvent"
#include <iostream>

void MainWindow::ChrWnd_Init()
{
}

void MainWindow::ChrWnd_FullRedraw()
{
    State state = m_state.back();

    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->clear();
    for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt)
        ui->comboBox_chr_set->addItem(itt->second.name, QVariant(itt->first));
    ui->comboBox_chr_set->setCurrentIndex(state.m_chr_map_index);
    ui->comboBox_chr_set->blockSignals(false);

    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());

    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::ChrWnd_RedrawImage()
{

}

void MainWindow::ChrWnd_RedrawFileName()
{
    State state = m_state.back();
    if (state.m_chr_map.empty())
        ui->widget_chr_set_item->setVisible(false);
    else
        ui->widget_chr_set_item->setVisible(true);

    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt != state.m_chr_map.end())
    {
        ui->lineEdit_chr_set_file_name->setText(itt->second.file_name);
    }
}

void MainWindow::on_comboBox_chr_set_currentIndexChanged(int index)
{
    State state = m_state.back();
    if (state.m_chr_map_index == index)
        return;
    state.m_chr_map_index = index;
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());
    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::on_lineEdit_chr_set_editingFinished()
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
        return;
    if (itt->second.name == ui->lineEdit_chr_set->text())
        return;
    itt->second.name = ui->lineEdit_chr_set->text();
    ui->comboBox_chr_set->setItemText(state.m_chr_map_index,  ui->lineEdit_chr_set->text());
    StatePush(state);
}

void MainWindow::on_btn_chr_set_add_clicked()
{
    State state = m_state.back();
    int found_id = 0;
    for (int id = 0; id < 1024; ++id)
    {
        if (state.m_chr_map.find(id) == state.m_chr_map.end())
        {
            found_id = id;
            break;
        }
    }
    ChrSet chr;
    chr.name = QString("chr-%1").arg(found_id);
    state.m_chr_map.insert(std::make_pair(found_id, chr));

    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->addItem(chr.name, QVariant(found_id));
    ui->comboBox_chr_set->setCurrentIndex(ui->comboBox_chr_set->count()-1);
    state.m_chr_map_index = ui->comboBox_chr_set->count()-1;
    ui->comboBox_chr_set->blockSignals(false);
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());

    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}


void MainWindow::on_btn_chr_set_remove_clicked()
{
    State state = m_state.back();
    int id = ui->comboBox_chr_set->itemData(state.m_chr_map_index).toInt();
    auto itt = state.m_chr_map.find(id);
    if (itt == state.m_chr_map.end())
        return;
    state.m_chr_map.erase(itt);
    ui->comboBox_chr_set->blockSignals(true);
    ui->comboBox_chr_set->removeItem(state.m_chr_map_index);
    state.m_chr_map_index = ui->comboBox_chr_set->currentIndex();
    ui->comboBox_chr_set->blockSignals(false);
    ui->lineEdit_chr_set->setText(ui->comboBox_chr_set->currentText());
    StatePush(state);
    ChrWnd_RedrawFileName();
    ChrWnd_RedrawImage();
}

void MainWindow::on_btn_chr_set_browse_clicked()
{

}

