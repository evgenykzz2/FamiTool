#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "Palette.h"
#include "Sprite.h"
#include "dialogpickpalette.h"
#include "dialogPickFamiPalette.h"
#include <sstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void UpdateIndexedImage();
    void RedrawPaletteTab();
    void UpdateBlinkPalette();
    void Image2Index(const QImage &image, std::vector<uint8_t>& index);
    void Index2Image(const std::vector<uint8_t>& index, QImage &image, int width, int height);
    void SaveProject(const QString &file_name);
    void LoadProject(const QString &file_name);

    void RedrawAttributeTab();
    void RedrawOamTab();
    void FullUpdateOamTab();
    void Export_SpriteConvert();
    void Export_Buffer(std::stringstream &stream, const void* data, size_t size);
protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
private slots:
    void on_comboBox_palette_mode_currentIndexChanged(int index);
    void PaletteWindow_Slot_PaletteSelect(int index);
    void PaletteWindow_Slot_PaletteSpriteSelect(int index);
    void PaletteFamiWindow_Slot_PaletteSelect(int index);
    void PaletteFamiWindow_Slot_PaletteSpriteSelect(int index);
    void on_pushButton_sprite_browse_clicked();
    void on_checkBox_palette_draw_cvt_stateChanged(int arg1);
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_pushButton_clear_colormapping_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_radioButton_oam_pal0_clicked();
    void on_radioButton_oam_pal1_clicked();
    void on_radioButton_oam_pal2_clicked();
    void on_radioButton_oam_pal3_clicked();
    void on_checkBox_oam_fill_any_color_clicked();
    void on_pushButton_export_clicked();
    void on_checkBox_attribute_grid_clicked();
    void on_lineEdit_palette_color_count_editingFinished();
    void on_checkBox_palette_deither_clicked();
    void on_comboBox_palette_method_currentIndexChanged(int index);
    void on_checkBox_make_indexed_clicked();
    void on_checkBox_draw_grid_clicked();
    void on_radioButton_oam_draw_original_clicked();
    void on_radioButton_oam_draw_result_clicked();
    void on_radioButton_oam_draw_background_clicked();

    void on_checkBox_two_frames_blinking_clicked();

private:
    Ui::MainWindow *ui;
    QString m_project_file_name;
    EPalette m_palette;
    Palette  m_palette_set[4];
    Palette  m_palette_sprite[4];
    QLabel*  m_palette_label[16];
    QLabel*  m_palette_sprite_label[16];

    BlinkPalette m_blink_palette_bg;
    BlinkPalette m_blink_palette_sprite;

    int      m_pick_pallete_index;
    DialogPickPalette m_pick_color_dialog;
    DialogPickFamiPalette m_pick_fami_palette_dialog;
    uint32_t m_pick_palette_cvt_color;
    std::map<uint32_t, int> m_palette_cvt_rule;
    std::map<uint32_t, int> m_palette_sprite_cvt_rule;
    QString m_spriteset_file_name;
    QImage m_image_original;
    QImage m_image_indexed;
    QImage m_image_screen;
    std::vector<uint8_t> m_spriteset_index;
    int    m_palette_tab_zoom;

    std::vector<uint8_t> m_screen_attribute; //1 byte per 16x16
    std::vector<uint8_t> m_screen_tiles;     //16 bytes per 8x8
    std::vector<uint8_t> m_screen_tiles_blink;     //alternate blink tile
    int    m_attribute_tab_zoom;

    int    m_oam_tab_zoom;
    QString m_oam_slice_last;
    int m_oam_selected;
    QPoint m_oam_start_point;
    std::vector<OAM> m_oam_vector;
};
#endif // MAINWINDOW_H
