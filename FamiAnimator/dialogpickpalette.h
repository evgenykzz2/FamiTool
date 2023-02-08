#ifndef DIALOGPICKPALETTE_H
#define DIALOGPICKPALETTE_H

#include <QDialog>
#include <map>
#include <stdint.h>

namespace Ui {
class DialogPickPalette;
}

class DialogPickPalette : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogPickPalette(QWidget *parent = 0);
    ~DialogPickPalette();
    void SetPalette(const uint32_t* palette);
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
    Ui::DialogPickPalette *ui;
    const uint32_t* m_palette;
};

#endif // DIALOGPICKPALETTE_H
