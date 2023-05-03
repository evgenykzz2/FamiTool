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

    void RedrawPaletteTab();
    void Image2Index(const QImage &image, std::vector<uint8_t>& index);
    void Index2Image(const std::vector<uint8_t>& index, QImage &image, int width, int height);
    void SaveProject(const QString &file_name);
    void LoadProject(const QString &file_name);

    void RedrawSliceTab();
    void RedrawOamTab();
    void FullUpdateOamTab();
    void Export_SpriteConvert();
    void Export_Buffer(std::stringstream &stream, const void* data, size_t size);
protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
private slots:
    void on_comboBox_palette_mode_currentIndexChanged(int index);
    void PaletteWindow_Slot_PaletteSelect(int index);
    void PaletteWindow_Slot_BgPaletteSelect(int index);
    void PaletteFamiWindow_Slot_PaletteSelect(int index);
    void on_pushButton_sprite_browse_clicked();
    void on_checkBox_palette_draw_cvt_stateChanged(int arg1);
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_pushButton_clear_colormapping_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_lineEdit_slice_name_editingFinished();
    void on_comboBox_oam_slice_currentIndexChanged(int index);
    void on_radioButton_oam_pal0_clicked();
    void on_radioButton_oam_pal1_clicked();
    void on_radioButton_oam_pal2_clicked();
    void on_radioButton_oam_pal3_clicked();
    void on_checkBox_oam_fill_any_color_clicked();
    void on_pushButton_export_test_clicked();

    void on_pushButton_slice_add_clicked();

    void on_comboBox_slice_list_currentIndexChanged(int index);

    void on_pushButton_slice_delete_clicked();

private:
    Ui::MainWindow *ui;
    QString m_project_file_name;
    EPalette m_palette;
    Palette  m_palette_set[4];
    QLabel*  m_palette_label[16];
    int      m_bg_color_index;
    uint32_t m_bg_color;

    int      m_pick_pallete_index;
    DialogPickPalette m_pick_color_dialog;
    DialogPickFamiPalette m_pick_fami_palette_dialog;
    uint32_t m_pick_palette_cvt_color;
    std::map<uint32_t, int> m_palette_cvt_rule;
    QString m_spriteset_file_name;
    QImage m_spriteset_original;
    QImage m_spriteset_indexed;
    std::vector<uint8_t> m_spriteset_index;
    int    m_palette_tab_zoom;

    int    m_slice_tab_zoom;
    int    m_slice_selected;
    int    m_slice_selected_corner;
    std::vector<SliceArea> m_slice_vector;
    QPoint m_slice_start_point;
    QPoint m_slice_end_point;
    bool m_slice_start_point_active;

    int    m_oam_tab_zoom;
    QString m_oam_slice_last;
    int m_oam_selected;
    QPoint m_oam_start_point;
};
#endif // MAINWINDOW_H
