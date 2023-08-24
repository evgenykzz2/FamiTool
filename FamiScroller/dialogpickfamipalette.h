#ifndef DIALOGPICKFAMIPALETTE_H
#define DIALOGPICKFAMIPALETTE_H

#include <QDialog>
#include <map>
#include <stdint.h>
#include <vector>
#include "Palette.h"

namespace Ui {
class DialogPickFamiPalette;
}

class DialogPickFamiPalette : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogPickFamiPalette(QWidget *parent = 0);
    ~DialogPickFamiPalette();
    void SetPalette(const uint32_t* palette, const std::vector<uint8_t> &palette_set);
    void SetPaletteIndex(int index);
    void UpdatePalette();
    int m_index;

protected:
    virtual bool eventFilter(QObject* object, QEvent* event);
private slots:
    void on_btn_cancel_clicked();
signals:
    void SignalPaletteSelect(int index);
private:
    Ui::DialogPickFamiPalette *ui;
    const uint32_t* m_palette;
    std::vector<uint8_t> m_palette_set;
    uint32_t m_original_color;
};

#endif // DIALOGPICKFAMIPALETTE_H
