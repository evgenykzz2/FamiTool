#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>

static QLabel* s_tileset_tab_render;
static int s_tileset_tab_zoom = 1;

void MainWindow::TilesetWnd_Init()
{
    s_tileset_tab_render = new QLabel();
    s_tileset_tab_render->move(0, 0);
    ui->scrollArea_tileset->setWidget(s_tileset_tab_render);
    s_tileset_tab_render->installEventFilter(this);
    s_tileset_tab_render->setMouseTracking(true);
    ui->scrollArea_tileset->installEventFilter(this);
}

void MainWindow::TilesetWnd_FullRedraw()
{
    State state = m_state.back();
    ui->comboBox_tileset->blockSignals(true);
    ui->comboBox_tileset->clear();
    for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt)
        ui->comboBox_tileset->addItem(itt->second.file_name, QVariant(itt->first));
    ui->comboBox_tileset->setCurrentIndex(state.m_tile_set_index);
    ui->comboBox_tileset->blockSignals(false);
    TileSetWnd_RedrawImage();
}

void MainWindow::TilesetWnd_EventFilter(QObject* object, QEvent* event)
{
    if (object == ui->scrollArea_tileset)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(key->key() == Qt::Key_Plus)
            {
                s_tileset_tab_zoom ++;
                if (s_tileset_tab_zoom > 4)
                    s_tileset_tab_zoom = 4;
                else
                    TileSetWnd_RedrawImage();
            }
            if(key->key() == Qt::Key_Minus)
            {
                s_tileset_tab_zoom --;
                if (s_tileset_tab_zoom < 1)
                    s_tileset_tab_zoom = 1;
                else
                    TileSetWnd_RedrawImage();
            }
        }
    }
}

void MainWindow::TileSetWnd_RedrawImage()
{
    QString file_name = RelativeToAbsolute(ui->comboBox_tileset->currentText());

    QImage img(file_name);
    img = img.scaled(img.width()*s_tileset_tab_zoom, img.height()*s_tileset_tab_zoom);
    s_tileset_tab_render->setPixmap(QPixmap::fromImage(img));
    s_tileset_tab_render->setMinimumSize(img.size());
    s_tileset_tab_render->setMaximumSize(img.size());
}

void MainWindow::on_comboBox_tileset_currentIndexChanged(int index)
{
    State state = m_state.back();
    state.m_tile_set_index = index;
    StatePush(state);
    TileSetWnd_RedrawImage();
}


void MainWindow::on_btn_tileset_add_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Select png tileset"), "", tr("*.png"));
    if (file_name.isEmpty())
        return;

    State state = m_state.back();
    int id = 0;
    for (int i = 0; i < 999; ++i)
    {
        if (state.m_tileset_map.find(i) == state.m_tileset_map.end())
        {
            id = i;
            break;
        }
    }

    for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt)
    {
        if (itt->second.file_name == file_name)
        {
            QMessageBox::critical(0, "Error", "Tileset already added");
            return;
        }
    }

    TileSet tileset;
    tileset.file_name = file_name;

    state.m_tileset_map.insert(std::make_pair(id, tileset));
    ui->comboBox_tileset->blockSignals(true);
    ui->comboBox_tileset->clear();
    for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt)
    {
        ui->comboBox_tileset->addItem(tileset.file_name, QVariant(id));
        if (itt->second.file_name == file_name)
            ui->comboBox_tileset->setCurrentIndex(ui->comboBox_tileset->count()-1);
    }
    ui->comboBox_tileset->blockSignals(false);

    StatePush(state);
    TileSetWnd_RedrawImage();
}

void MainWindow::on_btn_tileset_remove_clicked()
{
    if (ui->comboBox_tileset->count() == 0)
        return;

    State state = m_state.back();
    for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt)
    {
        if (itt->second.file_name == ui->comboBox_tileset->currentText())
        {
            state.m_tileset_map.erase(itt);
            ui->comboBox_tileset->removeItem(ui->comboBox_tileset->currentIndex());
            state.m_tile_set_index = ui->comboBox_tileset->currentIndex();
            StatePush(state);
            TileSetWnd_RedrawImage();
            return;
        }
    }
}
