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
#include <QShortcut>

//static QLabel* s_attribute_tab_render;
//static QLabel* s_oam_tab_render;
//static const int s_corner_size = 8;
//static const int s_blink_palette_block_size = 24;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    State state;
    state.m_width_screens = 1;
    state.m_height_screens = 1;
    state.m_palette = Palette_2C02;
    state.m_palette_map_index = 0;
    state.m_chr_map_index = 0;
    m_state.push_back(state);

    GeneralWnd_Init();
    PaletteWnd_Init();
    ChrWnd_Init();

    //-------------------------------------------------------------------------------
    //m_attribute_tab_zoom = 1;
    //s_attribute_tab_render = new QLabel();
    //s_attribute_tab_render->move(0, 0);
    //ui->scrollArea_attribute->setWidget(s_attribute_tab_render);
    //s_attribute_tab_render->installEventFilter(this);
    //s_attribute_tab_render->setMouseTracking(true);
    //ui->scrollArea_attribute->installEventFilter(this);

    //-------------------------------------------------------------------------------
    //m_oam_tab_zoom = 8;
    //s_oam_tab_render = new QLabel();
    //s_oam_tab_render->move(0, 0);
    //ui->scrollArea_OAM->setWidget(s_oam_tab_render);
    //s_oam_tab_render->installEventFilter(this);
    //s_oam_tab_render->setMouseTracking(true);
    //ui->scrollArea_OAM->installEventFilter(this);
    //m_oam_selected = -1;

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


    ui->tabWidget->setCurrentIndex(0);
    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());

    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
    QObject::connect(shortcut, SIGNAL(activated()), this, SLOT(slot_shortcut_ctrl_z()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::StatePush(const State &state)
{
    m_state.push_back(state);
    if (m_state.size() > 64)
        m_state.pop_front();
}

void MainWindow::slot_shortcut_ctrl_z()
{
    if (m_state.size() <= 1)
        return;
    m_state.pop_back();
    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
}

bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
    if (ui->tabWidget->currentWidget() == ui->tab_palette)
        PaletteWnd_EventFilter(object, event);

    if (event->type() == QEvent::MouseButtonPress)
    {
    }

    //if (object == ui->scrollArea_attribute)
    //{
    //    if (event->type() == QEvent::KeyPress)
    //    {
    //        QKeyEvent* key = static_cast<QKeyEvent*>(event);
    //        if(key->key() == Qt::Key_Plus)
    //        {
    //            m_attribute_tab_zoom ++;
    //            if (m_attribute_tab_zoom > 4)
    //                m_attribute_tab_zoom = 4;
    //            else
    //                RedrawAttributeTab();
    //        }
    //        if(key->key() == Qt::Key_Minus)
    //        {
    //            m_attribute_tab_zoom --;
    //            if (m_attribute_tab_zoom < 1)
    //                m_attribute_tab_zoom = 1;
    //            else
    //                RedrawAttributeTab();
    //        }
    //    }
    //}

    return QWidget::eventFilter( object, event );
}

void MainWindow::PaletteFamiWindow_Slot_PaletteSpriteSelect(int index)
{
}


void MainWindow::on_actionExit_triggered()
{
    this->close();
}


void MainWindow::on_actionOpen_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open project file"), QDir::currentPath(), "*.famiscroll");
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
    QString file_name = QFileDialog::getSaveFileName(this, "Save project file", QDir::currentPath(), "*.famiscroll");
    if (file_name.isEmpty())
        return;
    m_project_file_name = file_name;
    SaveProject(m_project_file_name);
}

void MainWindow::SaveProject(const QString &file_name)
{
    picojson::value json;
    State state = m_state.back();

    json.set<picojson::object>(picojson::object());
    json.get<picojson::object>()["palette_mode"] = picojson::value( (double)state.m_palette );

    //json.get<picojson::object>()["name"] = picojson::value( ui->lineEdit_name->text().toUtf8().toBase64().data() );
    //json.get<picojson::object>()["compression"] = picojson::value( (double)ui->comboBox_compression->itemData(ui->comboBox_compression->currentIndex()).toInt() );
    //json.get<picojson::object>()["chr_align"] = picojson::value( (double)ui->comboBox_chr_align->itemData(ui->comboBox_chr_align->currentIndex()).toInt() );


    json.get<picojson::object>()["width_screens"] = picojson::value( (double)state.m_width_screens );
    json.get<picojson::object>()["height_screens"] = picojson::value( (double)state.m_height_screens );


    {
        json.get<picojson::object>()["palette_map_index"] = picojson::value( (double)state.m_palette_map_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_palette_map.begin(); itt != state.m_palette_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                for (int i = 0; i < 16; ++i)
                {
                    QString name = QString("c%1").arg(i);
                    item_obj[name.toStdString()] = picojson::value( (double)(itt->second.color[i]) );
                }
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["palette_map"] = picojson::value(items);
    }

    {
        json.get<picojson::object>()["chr_map_index"] = picojson::value( (double)state.m_chr_map_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                item_obj["file_name"] = picojson::value( itt->second.file_name.toUtf8().toBase64().data() );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["chr_map"] = picojson::value(items);
    }

    //json.get<picojson::object>()["sprite_mode"] = picojson::value( (double)ui->comboBox_sprite_mode->itemData(ui->comboBox_sprite_mode->currentIndex()).toInt() );
    //json.get<picojson::object>()["sprite_set"] = picojson::value( m_spriteset_file_name.toUtf8().toBase64().data() );
    //json.get<picojson::object>()["indexed_apply"] = picojson::value( (bool)ui->checkBox_make_indexed->isChecked() );
    //json.get<picojson::object>()["indexed_method"] = picojson::value( ui->comboBox_palette_method->currentText().toStdString() );
    //json.get<picojson::object>()["indexed_diether"] = picojson::value( (bool)ui->checkBox_palette_deither->isChecked() );
    //json.get<picojson::object>()["indexed_colors"] = picojson::value( (double)ui->lineEdit_palette_color_count->text().toInt() );
    //json.get<picojson::object>()["two_frames_blinking"] = picojson::value( (bool)ui->checkBox_two_frames_blinking->isChecked() );


    //picojson::array items = picojson::array();
    //for (auto itt = m_palette_cvt_rule.begin(); itt != m_palette_cvt_rule.end(); ++itt)
    //{
    //        picojson::object item_obj;
    //        item_obj["r"] = picojson::value( (double)(itt->first & 0xFF) );
    //        item_obj["g"] = picojson::value( (double)((itt->first >> 8) & 0xFF) );
    //        item_obj["b"] = picojson::value( (double)((itt->first >> 16) & 0xFF) );
    //        //item_obj["a"] = picojson::value( (double)((itt->first >> 24) & 0xFF) );
    //        item_obj["i"] = picojson::value( (double)itt->second );
    //        items.push_back(picojson::value(item_obj));
    //}
    //json.get<picojson::object>()["color_map"] = picojson::value(items);


    //int irq0 = ui->edit_irq_0->text().toInt();
    //if (irq0 != 0)
    //    json.get<picojson::object>()["irq0"] = picojson::value( (double)irq0 );


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


    State state;

    picojson::value json;
    picojson::parse(json, json_str.toStdString());
    state.m_palette = Palette_2C02;
    if (json.contains("palette_mode"))
        state.m_palette = (EPalette)json.get<picojson::object>()["palette_mode"].get<double>();

    if (json.contains("width_screens"))
        state.m_width_screens = json.get<picojson::object>()["width_screens"].get<double>();
    else
        state.m_width_screens = 1;

    if (json.contains("height_screens"))
        state.m_height_screens = json.get<picojson::object>()["height_screens"].get<double>();
    else
        state.m_height_screens = 1;



    if (json.contains("palette_map_index"))
        state.m_palette_map_index = json.get<picojson::object>()["palette_map_index"].get<double>();
    else
        state.m_palette_map_index = 0;
    if (json.contains("palette_map"))
    {
        picojson::array items = json.get<picojson::object>()["palette_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            Palette palette;
            int id = (int)(itt->get<picojson::object>()["id"].get<double>());
            palette.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            for (int i = 0; i < 16; ++i)
            {
                QString name = QString("c%1").arg(i);
                palette.color[i] = (int)itt->get<picojson::object>()[name.toStdString()].get<double>();
            }
            state.m_palette_map.insert(std::make_pair(id, palette));
        }
    }

    if (json.contains("chr_map_index"))
        state.m_chr_map_index = json.get<picojson::object>()["chr_map_index"].get<double>();
    else
        state.m_chr_map_index = 0;
    if (json.contains("chr_map"))
    {
        picojson::array items = json.get<picojson::object>()["chr_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            ChrSet chr_set;
            int id = (int)(itt->get<picojson::object>()["id"].get<double>());
            chr_set.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            chr_set.file_name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["file_name"].get<std::string>() .c_str()));
            state.m_chr_map.insert(std::make_pair(id, chr_set));
        }
    }


    //QString name;
    //if (json.contains("name"))
        //name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["name"].get<std::string>().c_str()));
    //ui->lineEdit_name->setText(name);

    //m_palette_cvt_rule.clear();
    //if (json.contains("color_map"))
    //{
    //    picojson::array items = json.get<picojson::object>()["color_map"].get<picojson::array>();
    //    for (auto itt = items.begin(); itt != items.end(); ++itt)
    //    {
    //        uint32_t r = (uint32_t)(itt->get<picojson::object>()["r"].get<double>());
    //        uint32_t g = (uint32_t)(itt->get<picojson::object>()["g"].get<double>());
    //        uint32_t b = (uint32_t)(itt->get<picojson::object>()["b"].get<double>());
    //        uint32_t a = 0x00;//(uint32_t)(itt->get<picojson::object>()["a"].get<double>());
    //        int i = (int)(itt->get<picojson::object>()["i"].get<double>());
    //        m_palette_cvt_rule.insert(std::make_pair(r | (g<<8) | (b<<16) | (a<<24), i));
    //    }
    //}

    //if (json.contains("indexed_apply"))
    //    ui->checkBox_make_indexed->setChecked(json.get<picojson::object>()["indexed_apply"].get<bool>());
    //if (json.contains("indexed_diether"))
    //    ui->checkBox_palette_deither->setChecked(json.get<picojson::object>()["indexed_diether"].get<bool>());
    //if (json.contains("indexed_colors"))
    //    ui->lineEdit_palette_color_count->setText(QString("%1").arg((int)json.get<picojson::object>()["indexed_colors"].get<double>()));
    //if (json.contains("two_frames_blinking"))
        //ui->checkBox_two_frames_blinking->setChecked(json.get<picojson::object>()["two_frames_blinking"].get<bool>());

    //if (json.contains("irq0"))
    //    ui->edit_irq_0->setText( QString("%1").arg( (int)(json.get<picojson::object>()["irq0"].get<double>()) ));
    //else
    //    ui->edit_irq_0->setText("0");

    m_state.clear();
    m_state.push_back(state);
    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
}


void MainWindow::on_tabWidget_currentChanged(int)
{
    if (ui->tabWidget->currentWidget() == ui->tab_general)
        GeneralWnd_FullRedraw();
    if (ui->tabWidget->currentWidget() == ui->tab_palette)
        PaletteWnd_FullRedraw();
    if (ui->tabWidget->currentWidget() == ui->tab_chr)
        ChrWnd_FullRedraw();
}





