#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "Palette.h"
#include "Sprite.h"
#include "dialogpickpalette.h"
#include "dialogPickFamiPalette.h"
#include <sstream>
#include <set>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct State
{
    QString m_name;
    int m_length;
    ELevelType m_level_type;
    EPalette m_palette;

    std::map<int, Palette> m_palette_map;
    int m_palette_map_index; //combo box item

    std::map<int, ChrSet> m_chr_map;
    int m_chr_map_index; //combo box item

    std::map<int, Block> m_block_map;
    int m_block_map_index; //combo box item
    int m_transform_index; //combo box item

    std::map<int, TileSet> m_tileset_map;
    int m_tile_set_index; //combo box item

    std::vector< std::vector<int> > m_screen_tiles;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void SaveProject(const QString &file_name);
    void LoadProject(const QString &file_name);
    void StatePush(const State &state);
    QString RelativeToAbsolute(const QString &file_name);

    void GeneralWnd_Init();
    void GeneralWnd_FullRedraw();
    void GeneralWnd_RedrawPalette();

    void PaletteWnd_Init();
    void PaletteWnd_FullRedraw();
    void PaletteWnd_RedrawPalette();
    void PaletteWnd_EventFilter(QObject* object, QEvent* event);

    void TilesetWnd_Init();
    void TilesetWnd_FullRedraw();
    void TileSetWnd_RedrawImage();
    void TilesetWnd_EventFilter(QObject* object, QEvent* event);

    void BlockWnd_Init();
    void BlockWnd_FullRedraw();
    void BlockWnd_EventFilter(QObject* object, QEvent* event);
    void BlockWnd_RedrawBlock();
    void BlockWnd_RedrawTileset();
    void BlockWnd_RedrawTransformBlock();

    void ChrWnd_Init();
    void ChrWnd_FullRedraw();
    void ChrWnd_RedrawImage();
    void ChrWnd_RedrawFileName();
    void ChrWnd_Recalculate();
    std::map<int, TChrRender> ChrWnd_GetTileMap();

    void ScreenWnd_Init();
    void ScreenWnd_FullRedraw();
    void ScreenWnd_RedrawScreen();
    void ScreenWnd_EventFilter(QObject* object, QEvent* event);

    void Export_SpriteConvert();
    void Export_Buffer(std::stringstream &stream, const void* data, size_t size);
protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
private slots:
    void slot_shortcut_ctrl_z();
    void on_comboBox_palette_mode_currentIndexChanged(int index);
    void PaletteFamiWindow_Slot_PaletteSelect(int index);
    void PaletteFamiWindow_Slot_PaletteSpriteSelect(int index);
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_tabWidget_currentChanged(int index);
    void on_btn_palette_set_add_clicked();
    void on_btn_palette_set_remove_clicked();
    void on_comboBox_palette_set_currentIndexChanged(int index);
    void on_comboBox_chr_set_currentIndexChanged(int index);
    void on_btn_chr_set_add_clicked();
    void on_btn_chr_set_remove_clicked();
    void on_lineEdit_chr_set_editingFinished();
    void on_lineEdit_palette_set_editingFinished();
    void on_lineEdit_name_editingFinished();
    void on_comboBox_block_set_currentIndexChanged(int index);
    void on_lineEdit_block_set_editingFinished();
    void on_btn_block_set_add_clicked();
    void on_btn_block_set_remove_clicked();
    void on_comboBox_block_set_palette_currentIndexChanged(int index);
    void on_checkBox_screen_draw_grid_clicked();
    void on_scroll_screen_v_valueChanged(int value);
    void on_scroll_screen_h_valueChanged(int value);
    void on_checkBox_screen_show_block_index_clicked();
    void on_checkBox_screen_patter_clicked();
    void on_btn_screen_fill_clicked();
    void on_btn_block_overlay_browse_clicked();
    void on_btn_block_overlay_clear_clicked();
    void on_checkBox_screen_draw_overlay_clicked();
    void on_btn_block_export_all_clicked();
    void on_btn_block_import_all_clicked();
    void on_btn_export_blocks_clicked();
    void on_btn_export_stage_clicked();
    void on_btn_tileset_add_clicked();
    void on_btn_tileset_remove_clicked();
    void on_comboBox_tileset_currentIndexChanged(int index);
    void on_comboBox_block_tileset_currentIndexChanged(int index);
    void on_comboBox_chr_size_currentIndexChanged(int index);
    void on_checkBox_block_mark_tiles_clicked();
    void on_checkBox_block_mark_used_tiles_clicked();
    void on_comboBox_level_type_currentIndexChanged(int index);
    void on_edit_level_length_editingFinished();
    void on_btn_id_move_up_clicked();
    void on_btn_export_chr_clicked();
    void on_comboBox_transform_block_currentIndexChanged(int index);
    void on_comboBox_draw_transformation_currentIndexChanged(int index);
    void on_comboBox_block_logic_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QString m_project_file_name;

    DialogPickPalette m_pick_color_dialog;
    DialogPickFamiPalette m_pick_fami_palette_dialog;

    std::list<State> m_state;
};
#endif // MAINWINDOW_H
