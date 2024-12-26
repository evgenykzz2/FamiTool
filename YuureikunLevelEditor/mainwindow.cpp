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
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

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
    state.m_length = 16;
    state.m_palette = Palette_2C02;
    state.m_palette_map_index = 0;
    state.m_chr_map_index = 0;
    state.m_block_map_index = 0;
    state.m_transform_index = 0;
    //state.m_block_chr1 = 0;
    //state.m_block_chr0 = 0;
    state.m_tile_set_index = 0;
    m_state.push_back(state);

    GeneralWnd_Init();
    PaletteWnd_Init();
    TilesetWnd_Init();
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
    //ui->comboBox_compression->addItem("Rle4", QVariant((int)Compression_Rle4));
    ui->comboBox_compression->blockSignals(false);

    ui->comboBox_chr_align->blockSignals(true);
    ui->comboBox_chr_align->addItem("None", QVariant((int)ChrAlign_None));
    ui->comboBox_chr_align->addItem("Align 1KB", QVariant((int)ChrAlign_1K));
    ui->comboBox_chr_align->addItem("Align 2KB", QVariant((int)ChrAlign_2K));
    ui->comboBox_chr_align->addItem("Align 4KB", QVariant((int)ChrAlign_4K));
    ui->comboBox_chr_align->addItem("Align 8KB", QVariant((int)ChrAlign_8K));
    ui->comboBox_chr_align->blockSignals(false);

    {
        const QStringList app_location = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
        QDir().mkdir(app_location.constFirst());
        QSettings settings(app_location.constFirst() + "/settings.ini", QSettings::IniFormat);
        QString path = settings.value("last_folder").toString();
        if (!path.isEmpty())
            QDir::setCurrent(path);
    }

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
    if (ui->tabWidget->currentWidget() == ui->tab_tileset)
        TilesetWnd_EventFilter(object, event);

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
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open project file"), "", "*.familevel");
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
    QString file_name = QFileDialog::getSaveFileName(this, "Save project file", "", "*.familevel");
    if (file_name.isEmpty())
        return;
    m_project_file_name = file_name;
    SaveProject(m_project_file_name);
}

QString MainWindow::RelativeToAbsolute(const QString &file_name)
{
    if (!m_project_file_name.isEmpty())
    {
        QFileInfo info(file_name);
        if (!info.isAbsolute())
        {
            QFileInfo project_file_info(m_project_file_name);
            QDir project_dir(project_file_info.dir());
            return project_dir.absoluteFilePath(file_name);
        }
    }
    return file_name;
}

static bool IsUtfString(const QString &str)
{
    for (int i = 0; i < str.length(); ++i)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
        {
            //it is OK
        } else if (str[i] >= 'A' && str[i] <= 'Z')
        {
            //it is OK
        } else if (str[i] >= '0' && str[i] <= '9')
        {
            //it is OK
        } else if (str[i] == '!' || str[i] == '?' || str[i] == '^' || str[i] == '(' || str[i] == ')' ||
                   str[i] == '-' || str[i] == '_' || str[i] == '+' || str[i] == '%' || str[i] == '<' ||
                   str[i] == '>' || str[i] == '.' || str[i] == ',' || str[i] == ':' || str[i] == ';')
        {
            //it is OK
        } else
            return true;
    }
    return false;
}

void MainWindow::SaveProject(const QString &file_name)
{
    picojson::value json;
    State state = m_state.back();

    json.set<picojson::object>(picojson::object());
    json.get<picojson::object>()["palette_mode"] = picojson::value( (double)state.m_palette );

    if (IsUtfString(state.m_name))
        json.get<picojson::object>()["name"] = picojson::value( state.m_name.toUtf8().toBase64().data() );
    else
        json.get<picojson::object>()["name_cstr"] = picojson::value( state.m_name.toStdString().c_str() );
    //json.get<picojson::object>()["compression"] = picojson::value( (double)ui->comboBox_compression->itemData(ui->comboBox_compression->currentIndex()).toInt() );
    //json.get<picojson::object>()["chr_align"] = picojson::value( (double)ui->comboBox_chr_align->itemData(ui->comboBox_chr_align->currentIndex()).toInt() );


    json.get<picojson::object>()["level_type"] = picojson::value( (double)state.m_level_type );
    json.get<picojson::object>()["length"] = picojson::value( (double)state.m_length );


    {
        json.get<picojson::object>()["palette_map_index"] = picojson::value( (double)state.m_palette_map_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_palette_map.begin(); itt != state.m_palette_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                if (IsUtfString(itt->second.name))
                    item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                else
                    item_obj["name_cstr"] = picojson::value( itt->second.name.toStdString().c_str() );
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
        json.get<picojson::object>()["tileset_index"] = picojson::value( (double)state.m_tile_set_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_tileset_map.begin(); itt != state.m_tileset_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)itt->first );
                item_obj["file"] = picojson::value( itt->second.file_name.toUtf8().toBase64().data() );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["tileset_map"] = picojson::value(items);
    }

    {
        json.get<picojson::object>()["chr_map_index"] = picojson::value( (double)state.m_chr_map_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_chr_map.begin(); itt != state.m_chr_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                if (IsUtfString(itt->second.name))
                    item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                else
                    item_obj["name_cstr"] = picojson::value( itt->second.name.toStdString().c_str() );
                item_obj["chr_size"] = picojson::value( (double)(itt->second.chr_size) );
                //item_obj["file_name"] = picojson::value( itt->second.file_name.toUtf8().toBase64().data() );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["chr_map"] = picojson::value(items);
    }

    {
        json.get<picojson::object>()["block_map_index"] = picojson::value( (double)state.m_block_map_index );
        json.get<picojson::object>()["block_transform_index"] = picojson::value( (double)state.m_transform_index );
        picojson::array items = picojson::array();
        for (auto itt = state.m_block_map.begin(); itt != state.m_block_map.end(); ++itt)
        {
                picojson::object item_obj;
                item_obj["id"] = picojson::value( (double)(itt->first) );
                if (IsUtfString(itt->second.name))
                    item_obj["name"] = picojson::value( itt->second.name.toUtf8().toBase64().data() );
                else
                    item_obj["name_cstr"] = picojson::value( itt->second.name.toStdString().c_str() );
                item_obj["chrbank"] = picojson::value( (double)itt->second.chrbank );
                item_obj["tileset"] = picojson::value( (double)itt->second.tileset );
                item_obj["tile_x"] = picojson::value( (double)(itt->second.tile_x) );
                item_obj["tile_y"] = picojson::value( (double)(itt->second.tile_y) );

                if (itt->second.transform_index >= 0)
                    item_obj["transform"] = picojson::value( (double)(itt->second.transform_index) );

                item_obj["pal"] = picojson::value( (double)(itt->second.palette) );
                if (!itt->second.overlay.isEmpty())
                    item_obj["overlay"] = picojson::value( itt->second.overlay.toUtf8().toBase64().data() );
                items.push_back(picojson::value(item_obj));
        }
        json.get<picojson::object>()["block_map"] = picojson::value(items);
    }

    {
        for (size_t n = 0; n < state.m_screen_tiles.size(); ++n)
        {
            picojson::object item_obj;
            QString name = QString("st%1").arg(n, 3, 10, QLatin1Char('0'));
            std::stringstream stream;
            for (int x = 0; x < state.m_screen_tiles[n].size(); ++x)
                stream << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (((uint32_t)state.m_screen_tiles[n][x])&255);
            json.get<picojson::object>()[name.toLocal8Bit().data()] = picojson::value(stream.str());
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

    QFileInfo project_file_info(file_name);
    QDir project_dir(project_file_info.dir());

    {
        const QStringList app_location = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
        QDir().mkdir(app_location.constFirst());
        QSettings settings(app_location.constFirst() + "/settings.ini", QSettings::IniFormat);
        settings.setValue("last_folder", project_file_info.dir().absolutePath());
    }

    State state;

    picojson::value json;
    picojson::parse(json, json_str.toStdString());
    state.m_palette = Palette_2C02;
    if (json.contains("palette_mode"))
        state.m_palette = (EPalette)json.get<picojson::object>()["palette_mode"].get<double>();


    if (json.contains("length"))
        state.m_length = json.get<picojson::object>()["length"].get<double>();
    else
        state.m_length = 16;

    if (json.contains("level_type"))
        state.m_level_type = (ELevelType)json.get<picojson::object>()["level_type"].get<double>();
    else
        state.m_level_type = LevelType_Horizontal;

    if (json.contains("name"))
        state.m_name = QString::fromUtf8( QByteArray::fromBase64(json.get<picojson::object>()["name"].get<std::string>().c_str()));
    if (json.contains("name_cstr"))
        state.m_name = QString::fromStdString( json.get<picojson::object>()["name_cstr"].get<std::string>().c_str() );

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
            if (itt->contains("name"))
                palette.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            if (itt->contains("name_cstr"))
                palette.name = QString::fromStdString( itt->get<picojson::object>()["name_cstr"].get<std::string>());
            for (int i = 0; i < 16; ++i)
            {
                QString name = QString("c%1").arg(i);
                palette.color[i] = (int)itt->get<picojson::object>()[name.toStdString()].get<double>();
            }
            state.m_palette_map.insert(std::make_pair(id, palette));
        }
    }

    if (json.contains("tileset_index"))
        state.m_tile_set_index = json.get<picojson::object>()["tileset_index"].get<double>();
    else
        state.m_tile_set_index = 0;

    if (json.contains("tileset_map"))
    {
        picojson::array items = json.get<picojson::object>()["tileset_map"].get<picojson::array>();
        for (auto itt = items.begin(); itt != items.end(); ++itt)
        {
            TileSet tileset;
            tileset.file_name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["file"].get<std::string>() .c_str()));
            QFileInfo info(tileset.file_name);
            if (info.isAbsolute())
                tileset.file_name = project_dir.relativeFilePath(tileset.file_name);
            int id = 0;
            if (itt->contains("id"))
                id = (int)(itt->get<picojson::object>()["id"].get<double>());
            state.m_tileset_map.insert(std::make_pair(id, tileset));
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
            if (itt->contains("name"))
                chr_set.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            if (itt->contains("name_cstr"))
                chr_set.name = QString::fromStdString( itt->get<picojson::object>()["name_cstr"].get<std::string>());
            chr_set.chr_size = itt->get<picojson::object>()["chr_size"].get<double>();
            state.m_chr_map.insert(std::make_pair(id, chr_set));
        }
    }

    /*if (json.contains("block_chr0"))
        state.m_block_chr0 = json.get<picojson::object>()["block_chr0"].get<double>();
    else
        state.m_block_chr0 = 0;

    if (json.contains("block_chr1"))
        state.m_block_chr1 = json.get<picojson::object>()["block_chr1"].get<double>();
    else
        state.m_block_chr1 = 0;*/

    if (json.contains("block_transform_index"))
        state.m_transform_index = json.get<picojson::object>()["block_transform_index"].get<double>();
    else
        state.m_transform_index = 0;

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
            if (itt->contains("name"))
                block.name = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["name"].get<std::string>() .c_str()));
            if (itt->contains("name_cstr"))
                block.name = QString::fromStdString( itt->get<picojson::object>()["name_cstr"].get<std::string>());
            if (itt->contains("chrbank"))
                block.chrbank = (int)(itt->get<picojson::object>()["chrbank"].get<double>());
            else
                block.chrbank = 0;
            if (itt->contains("tileset"))
                block.tileset = (int)(itt->get<picojson::object>()["tileset"].get<double>());
            else
                block.tileset = 0;
            if (itt->contains("tile_x"))
                block.tile_x = itt->get<picojson::object>()["tile_x"].get<double>();
            else
                block.tile_x = 0;
            if (itt->contains("tile_y"))
                block.tile_y = itt->get<picojson::object>()["tile_y"].get<double>();
            else
                block.tile_y = 0;

            if (itt->contains("transform"))
                block.transform_index = itt->get<picojson::object>()["transform"].get<double>();
            else
                block.transform_index = -1;

            block.palette = (int)itt->get<picojson::object>()["pal"].get<double>();
            if (itt->contains("overlay"))
            {
                block.overlay = QString::fromUtf8( QByteArray::fromBase64( itt->get<picojson::object>()["overlay"].get<std::string>() .c_str()));
                QFileInfo info(block.overlay);
                if (info.isAbsolute())
                    block.overlay = project_dir.relativeFilePath(block.overlay);
            }
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

    for (size_t n = 0; n < state.m_length; ++n)
    {
        QString name = QString("st%1").arg(n, 3, 10, QLatin1Char('0'));
        std::vector<int> tile_vector;
        if (state.m_level_type == LevelType_Horizontal)
            tile_vector.resize(YUUREIKUN_HEIGHT, 0x00);
        else
            tile_vector.resize(YUUREIKUN_WIDTH, 0x00);

        if (json.contains(name.toLocal8Bit().data()))
        {
            std::string line = json.get<picojson::object>()[name.toLocal8Bit().data()].get<std::string>();
            for (int x = 0; x < tile_vector.size(); ++x)
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
                tile_vector[x] = v;
            }
        }
        state.m_screen_tiles.push_back(tile_vector);
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
    if (ui->tabWidget->currentWidget() == ui->tab_tileset)
        TilesetWnd_FullRedraw();
}


































