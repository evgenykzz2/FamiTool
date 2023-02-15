#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "Palette.h"
#include "Sprite.h"
#include "dialogpickpalette.h"
#include "dialogPickFamiPalette.h"
#include <sstream>
#include "AviReader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void GeneralTab_Init();
    void GeneralTab_UpdatePalette();
    void GeneralTab_UpdateMovieInfo();
    void GeneralTab_ReloadBaseTiles();

    void UpdateIndexedImage();

    void VideoTab_Init();
    void VideoTab_FullUpdate();
    void VideoTab_Redraw();
    void VideoTab_UpdateFrameNumber();
    void VideoTab_EventFilter(QObject *object, QEvent *event);

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
    void PaletteFamiWindow_Slot_PaletteSelect(int index);
    void PaletteFamiWindow_Slot_PaletteSpriteSelect(int index);
    void on_pushButton_movie_browse_clicked();
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
    void on_pushButton_frame_previous_clicked();
    void on_pushButton_frame_next_clicked();
    void on_horizontalSlider_sliderMoved(int position);
    void on_lineEdit_video_frame_number_editingFinished();
    void on_pushButton_base_tiles_browse_clicked();
    void on_comboBox_video_view_mode_currentIndexChanged(int index);

    void on_comboBox_frame_mode_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QString m_project_file_name;
    std::shared_ptr<AviReader> m_avi_reader;
    std::map<int, FrameInfo> m_frame_info_map;
    std::vector<Tile> m_base_tile_vector;
    std::map<Tile, size_t> m_base_tile_map;

    int      m_pick_pallete_index;
    DialogPickPalette m_pick_color_dialog;
    DialogPickFamiPalette m_pick_fami_palette_dialog;
    uint32_t m_pick_palette_cvt_color;
    QString m_movie_file_name;
    QImage m_image_original;
    QImage m_image_indexed;
    QImage m_image_screen;
    std::vector<uint8_t> m_spriteset_index;


    std::vector<uint8_t> m_screen_attribute; //1 byte per 16x16
    std::vector<uint8_t> m_screen_tiles; //16 bytes per 8x8
    int    m_attribute_tab_zoom;

    int    m_oam_tab_zoom;
    QString m_oam_slice_last;
    int m_oam_selected;
    QPoint m_oam_start_point;
    std::vector<OAM> m_oam_vector;
};
#endif // MAINWINDOW_H
