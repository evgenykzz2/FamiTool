#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "Palette.h"
#include "Sprite.h"
#include "Screen.h"
#include "dialogpickpalette.h"
#include "dialogPickFamiPalette.h"
#include <sstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct State
{
    int m_width_screens;
    int m_height_screens;
    EPalette m_palette;

    std::map<int, Palette> m_palette_map;
    int m_palette_map_index; //combo box item

    std::map<int, ChrSet> m_chr_map;
    int m_chr_map_index; //combo box item

    std::map<std::pair<int, int>, Screen> m_world;
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

    void GeneralWnd_Init();
    void GeneralWnd_FullRedraw();
    void GeneralWnd_RedrawPalette();

    void PaletteWnd_Init();
    void PaletteWnd_FullRedraw();
    void PaletteWnd_RedrawPalette();
    void PaletteWnd_EventFilter(QObject* object, QEvent* event);

    void ChrWnd_Init();
    void ChrWnd_FullRedraw();
    void ChrWnd_RedrawImage();
    void ChrWnd_RedrawFileName();

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
    void on_pushButton_export_clicked();
    void on_edit_width_in_screens_editingFinished();
    void on_edit_height_in_screens_editingFinished();
    void on_btn_palette_set_add_clicked();
    void on_btn_palette_set_remove_clicked();
    void on_comboBox_palette_set_currentIndexChanged(int index);
    void on_comboBox_chr_set_currentIndexChanged(int index);
    void on_btn_chr_set_add_clicked();
    void on_btn_chr_set_remove_clicked();
    void on_btn_chr_set_browse_clicked();

    void on_lineEdit_chr_set_editingFinished();

    void on_lineEdit_palette_set_editingFinished();

private:
    Ui::MainWindow *ui;
    QString m_project_file_name;

    DialogPickPalette m_pick_color_dialog;
    DialogPickFamiPalette m_pick_fami_palette_dialog;

    std::list<State> m_state;
};
#endif // MAINWINDOW_H
