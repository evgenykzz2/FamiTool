#ifndef DIALOGPICKFAMIPALETTE_H
#define DIALOGPICKFAMIPALETTE_H

#include <QDialog>
#include <map>
#include <stdint.h>
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
    void SetPalette(const uint32_t* palette, const Palette* palette_set, bool enable_blink_palette, const BlinkPalette &blink_palette);
    void SetOriginalColor(uint32_t color);
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
    const Palette* m_palette_set;
    BlinkPalette m_blink_palette;
    bool m_enable_blink_palette;
    uint32_t m_original_color;
};

#endif // DIALOGPICKFAMIPALETTE_H
