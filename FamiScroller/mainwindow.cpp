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
#include <iomanip>
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
    state.m_block_map_index = 0;
    state.m_block_chr1 = 0;
    state.m_block_chr0 = 0;
    state.m_block_tile_slot = 0;
    m_state.push_back(state);

    GeneralWnd_Init();
    PaletteWnd_Init();
    ChrWnd_Init();
    BlockWnd_Init();
    ScreenWnd_Init();

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
    if (ui->tabWidget->currentWidget() == ui->tab_Block)
        BlockWnd_EventFilter(object, event);
    if (ui->tabWidget->currentWidget() == ui->tab_screen)
        ScreenWnd_EventFilter(object, event);

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

    json.get<picojson::object>()["name"] = picojson::value( state.m_name.toUtf8().toBase64().data() );
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

    {
        json.get<picojson::object>()["block_chr0"] = picojson::value( (double)state.m_block_chr0 );
        json.get<picojson::object>()["block_chr1"] = picojson::value( (double)state.m_block_chr1 );
        json.get<picojson::object>()["block_map_index"] = picojson::value( (double)state.m_block_map_index );
        json.get<picojson::object>()["block_tile_slot"] = picojson::value( (double)state.m_block_tile_slot );
        picojson::array items = picojson::array();
        for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                item_obj["t0"] = picojson::value( (double)(itt->second.tile_id[0]) );
                item_obj["t1"] = picojson::value( (double)(itt->second.tile_id[1]) );
                item_obj["t2"] = picojson::value( (double)(itt->second.tile_id[2]) );
                item_obj["t3"] = picojson::value( (double)(itt->second.tile_id[3]) );
                item_obj["pal"] = picojson::value( (double)(itt->second.palette) );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["block_map"] = picojson::value(items);
    }

    {
        picojson::array items = picojson::array();
        for (auto itt = state.m_world.begin(); itt != state.m_world.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["x"] = picojson::value( (double)(itt->first.first) );
                item_obj["y"] = picojson::value( (double)(itt->first.second) );
                item_obj["chr0"] = picojson::value( (double)(itt->second.chr0_id) );
                item_obj["chr1"] = picojson::value( (double)(itt->second.chr1_id) );
                item_obj["flags"] = picojson::value( (double)(itt->second.flags) );
                item_obj["palette_id"] = picojson::value( (double)(itt->second.palette_id) );
                for (int y = 0; y < 16; ++y)
                {
                    std::stringstream stream;
                    for (int x = 0; x < 16; ++x)
                        stream << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (((uint32_t)itt->second.block[y*16+x])&255);
                    QString name = QString("l%1").arg(y, 2, 10, QChar('0'));
                    item_obj[name.toStdString()] = picojson::value(stream.str());
                }
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["world_map"] = picojson::value(items);
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

#if 0
static const uint8_t s_block_chr[128*4] =
    {
        0x24, 0x24, 0x24, 0x24,   0x27, 0x27, 0x27, 0x27,   0x24, 0x24, 0x24, 0x35,   0x36, 0x25, 0x37, 0x25,   //00
        0x24, 0x38, 0x24, 0x24,   0x24, 0x30, 0x30, 0x26,   0x26, 0x26, 0x34, 0x26,   0x24, 0x31, 0x24, 0x32,   //04
        0x33, 0x26, 0x24, 0x33,   0x34, 0x26, 0x26, 0x26,   0x26, 0x26, 0x26, 0x26,   0x24, 0xC0, 0x24, 0xC0,   //08
        0x24, 0x7F, 0x7F, 0x24,   0xB8, 0xBA, 0xB9, 0xBB,   0xB8, 0xBC, 0xB9, 0xBD,   0xBA, 0xBC, 0xBB, 0xBD,   //0C
        0x60, 0x64, 0x61, 0x65,   0x62, 0x66, 0x63, 0x67,   0x60, 0x64, 0x61, 0x65,   0x62, 0x66, 0x63, 0x67,   //10
        0x68, 0x68, 0x69, 0x69,   0x26, 0x26, 0x6A, 0x6A,   0x4B, 0x4C, 0x4D, 0x4E,   0x4D, 0x4F, 0x4D, 0x4F,   //14
        0x4D, 0x4E, 0x50, 0x51,   0x6B, 0x70, 0x2C, 0x2D,   0x6C, 0x71, 0x6D, 0x72,   0x6E, 0x73, 0x6F, 0x74,   //18
        0x86, 0x8A, 0x87, 0x8B,   0x88, 0x8C, 0x88, 0x8C,   0x89, 0x8D, 0x69, 0x69,   0x8E, 0x91, 0x8F, 0x92,   //1C
        0x26, 0x93, 0x26, 0x93,   0x90, 0x94, 0x69, 0x69,   0xA4, 0xE9, 0xEA, 0xEB,   0x24, 0x24, 0x24, 0x24,   //20
        0x24, 0x2F, 0x24, 0x3D,   0xA2, 0xA2, 0xA3, 0xA3,   0x24, 0x24, 0x24, 0x24,   0xA2, 0xA2, 0xA3, 0xA3,   //24
        0x99, 0x24, 0x99, 0x24,   0x24, 0xA2, 0x3E, 0x3F,   0x5B, 0x5C, 0x24, 0xA3,   0x24, 0x24, 0x24, 0x24,   //28
        0x9D, 0x47, 0x9E, 0x47,   0x47, 0x47, 0x27, 0x27,   0x47, 0x47, 0x47, 0x47,   0x27, 0x27, 0x47, 0x47,   //2C
        0xA9, 0x47, 0xAA, 0x47,   0x9B, 0x27, 0x9C, 0x27,   0x27, 0x27, 0x27, 0x27,   0x52, 0x52, 0x52, 0x52,   //30
        0x80, 0xA0, 0x81, 0xA1,   0xBE, 0xBE, 0xBF, 0xBF,   0x75, 0xBA, 0x76, 0xBB,   0xBA, 0xBA, 0xBB, 0xBB,   //34
        0x45, 0x47, 0x45, 0x47,   0x47, 0x47, 0x47, 0x47,   0x45, 0x47, 0x45, 0x47,   0xB4, 0xB6, 0xB5, 0xB7,   //38
        0x45, 0x47, 0x45, 0x47,   0x45, 0x47, 0x45, 0x47,   0x45, 0x47, 0x45, 0x47,   0x45, 0x47, 0x45, 0x47,   //3C
        0x45, 0x47, 0x45, 0x47,   0x47, 0x47, 0x47, 0x47,   0x47, 0x47, 0x47, 0x47,   0x47, 0x47, 0x47, 0x47,   //40
        0x47, 0x47, 0x47, 0x47,   0x47, 0x47, 0x47, 0x47,   0x24, 0x24, 0x24, 0x24,   0x24, 0x24, 0x24, 0x24,   //44
        0xAB, 0xAC, 0xAD, 0xAE,   0x5D, 0x5E, 0x5D, 0x5E,   0xC1, 0x24, 0xC1, 0x24,   0xC6, 0xC8, 0xC7, 0xC9,   //48
        0xCA, 0xCC, 0xCB, 0xCD,   0x2A, 0x2A, 0x40, 0x40,   0x24, 0x24, 0x24, 0x24,   0x24, 0x47, 0x24, 0x47,   //4C
        0x82, 0x83, 0x84, 0x85,   0x24, 0x47, 0x24, 0x47,   0x86, 0x8A, 0x87, 0x8B,   0x8E, 0x91, 0x8F, 0x92,   //50
        0x24, 0x2F, 0x24, 0x3D,   0x24, 0x24, 0x24, 0x35,   0x36, 0x25, 0x37, 0x25,   0x24, 0x38, 0x24, 0x24,   //54
        0x24, 0x24, 0x39, 0x24,   0x3A, 0x24, 0x3B, 0x24,   0x3C, 0x24, 0x24, 0x24,   0x41, 0x26, 0x41, 0x26,   //58
        0x26, 0x26, 0x26, 0x26,   0xB0, 0xB1, 0xB2, 0xB3,   0x77, 0x79, 0x77, 0x79,   0x53, 0x55, 0x54, 0x56,   //5C
        0x53, 0x55, 0x54, 0x56,   0xA5, 0xA7, 0xA6, 0xA8,   0xC2, 0xC4, 0xC3, 0xC5,   0x57, 0x59, 0x58, 0x5A,   //60
        0x7B, 0x7D, 0x7C, 0x7E,   0x3F, 0x00, 0x20, 0x0F,   0x15, 0x12, 0x25, 0x0F,   0x3A, 0x1A, 0x0F, 0x0F,   //64
        0x30, 0x12, 0x0F, 0x0F,   0x27, 0x12, 0x0F, 0x22,   0x16, 0x27, 0x18, 0x0F,   0x10, 0x30, 0x27, 0x0F,   //68
        0x16, 0x30, 0x27, 0x0F,   0x0F, 0x30, 0x10, 0x00,   0x3F, 0x00, 0x20, 0x0F,   0x29, 0x1A, 0x0F, 0x0F,   //6C
        0x36, 0x17, 0x0F, 0x0F,   0x30, 0x21, 0x0F, 0x0F,   0x27, 0x17, 0x0F, 0x0F,   0x16, 0x27, 0x18, 0x0F,   //70
        0x1A, 0x30, 0x27, 0x0F,   0x16, 0x30, 0x27, 0x0F,   0x0F, 0x36, 0x17, 0x00,   0x3F, 0x00, 0x20, 0x0F,   //74
        0x29, 0x1A, 0x09, 0x0F,   0x3C, 0x1C, 0x0F, 0x0F,   0x30, 0x21, 0x1C, 0x0F,   0x27, 0x17, 0x1C, 0x0F,   //78
        0x16, 0x27, 0x18, 0x0F,   0x1C, 0x36, 0x17, 0x0F,   0x16, 0x30, 0x27, 0x0F,   0x0C, 0x3C, 0x1C, 0x00    //7C
    };

static const uint8_t s_block_pal[128] =
    {
        0, 0, 0, 0, //00
        0, 0, 0, 0, //04
        0, 0, 0, 0, //08
        0, 0, 0, 0, //0C
        0, 0, 0, 0, //10
        0, 0, 0, 0, //14
        0, 0, 0, 0, //18
        0, 0, 0, 0, //1C
        0, 0, 0, 0, //20
        0, 0, 0, 0, //24
        0, 0, 0, 0, //28
        1, 1, 1, 1, //2C
        1, 1, 0, 1, //30
        0, 0, 0, 0, //34
        1, 1, 1, 1, //38
        1, 1, 1, 1, //3C
        1, 1, 1, 1, //40
        1, 1, 0, 0, //44
        1, 0, 1, 1, //48
        1, 1, 0, 1, //4C
        2, 1, 0, 0, //50
        0, 2, 2, 2, //54
        2, 2, 2, 0, //58
        0, 0, 0, 3, //5C
        3, 3, 3, 1, //60
        0, 0, 0, 0, //64
        0, 0, 0, 0, //68
        0, 0, 0, 0, //6C
        0, 0, 0, 0, //70
        0, 0, 0, 0, //74
        0, 0, 0, 0, //78
        0, 0, 0, 0, //7C
    };
#endif

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

    if (json.contains("name"))
        state.m_name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["name"].get<std::string>().c_str()));

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

            if (!chr_set.file_name.isEmpty())
            {
                chr_set.chr_data = std::make_shared<std::vector<uint8_t>>(2048, 0x00);

                QFile file(chr_set.file_name);
                QByteArray byte_array;
                if (file.open(QFile::ReadOnly))
                {
                    byte_array = file.readAll();
                    file.close();

                    int size = chr_set.chr_data->size();
                    if (byte_array.size() < size)
                        size = byte_array.size();
                    memcpy(chr_set.chr_data->data(), byte_array.data(), size);
                }
            }
            state.m_chr_map.insert(std::make_pair(id, chr_set));
        }
    }

    if (json.contains("block_chr0"))
        state.m_block_chr0 = json.get<picojson::object>()["block_chr0"].get<double>();
    else
        state.m_block_chr0 = 0;

    if (json.contains("block_chr1"))
        state.m_block_chr1 = json.get<picojson::object>()["block_chr1"].get<double>();
    else
        state.m_block_chr1 = 0;

    if (json.contains("block_tile_slot"))
        state.m_block_tile_slot = json.get<picojson::object>()["block_tile_slot"].get<double>();
    else
        state.m_block_tile_slot = 0;

    if (json.contains("block_map_index"))
        state.m_block_map_index = json.get<picojson::object>()["block_map_index"].get<double>();
    else
        state.m_block_map_index = 0;
    if (json.contains("block_map"))
    {
        picojson::array items = json.get<picojson::object>()["block_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            Block block;
            int id = (int)(itt->get<picojson::object>()["id"].get<double>());
            block.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            block.tile_id[0] = (int)itt->get<picojson::object>()["t0"].get<double>();
            block.tile_id[1] = (int)itt->get<picojson::object>()["t1"].get<double>();
            block.tile_id[2] = (int)itt->get<picojson::object>()["t2"].get<double>();
            block.tile_id[3] = (int)itt->get<picojson::object>()["t3"].get<double>();
            block.palette = (int)itt->get<picojson::object>()["pal"].get<double>();
            state.m_block_map.insert(std::make_pair(id, block));
        }
    }

#if 0
    for (int i = 0; i < 102; ++i)
    {
        Block block;
        block.name = QString("block-%1").arg(i);
        block.palette = s_block_pal[i];
        block.tile_id[0] = s_block_chr[i*4+0];
        block.tile_id[1] = s_block_chr[i*4+2];
        block.tile_id[2] = s_block_chr[i*4+1];
        block.tile_id[3] = s_block_chr[i*4+3];
        state.m_block_map.insert(std::make_pair(i, block));
    }
#endif

    if (json.contains("world_map"))
    {
        picojson::array items = json.get<picojson::object>()["world_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            Screen screen;
            int x = (int)(itt->get<picojson::object>()["x"].get<double>());
            int y = (int)(itt->get<picojson::object>()["y"].get<double>());
            screen.chr0_id = (int)(itt->get<picojson::object>()["chr0"].get<double>());
            screen.chr1_id = (int)(itt->get<picojson::object>()["chr1"].get<double>());
            screen.flags = (int)(itt->get<picojson::object>()["flags"].get<double>());
            screen.palette_id = (int)(itt->get<picojson::object>()["palette_id"].get<double>());
            for (int y = 0; y < 16; ++y)
            {
                QString name = QString("l%1").arg(y, 2, 10, QChar('0'));
                std::string line = itt->get<picojson::object>()[name.toStdString()].get<std::string>();
                for (int x = 0; x < 16; ++x)
                {
                    uint8_t v = 0;
                    if (line[x*2+0] >= '0' && line[x*2+0] <= '9')
                        v = (line[x*2+0]-'0') << 4;
                    else if (line[x*2+0] >= 'A' && line[x*2+0] <= 'F')
                        v = ((line[x*2+0]-'A') + 0xA) << 4;

                    if (line[x*2+1] >= '0' && line[x*2+1] <= '9')
                        v |= (line[x*2+1]-'0');
                    else if (line[x*2+1] >= 'A' && line[x*2+1] <= 'F')
                        v |= ((line[x*2+1]-'A') + 0xA);
                    screen.block[x+y*16] = v;
                }
            }
            state.m_world.insert(std::make_pair(std::make_pair(x, y), screen));
        }
    }

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
    if (ui->tabWidget->currentWidget() == ui->tab_Block)
        BlockWnd_FullRedraw();
    if (ui->tabWidget->currentWidget() == ui->tab_screen)
        ScreenWnd_FullRedraw();
}









