#ifndef DIALOG_SLIDERS_H
#define DIALOG_SLIDERS_H

#include <QDialog>

namespace Ui {
class Dialog_sliders;
}

class Dialog_sliders : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_sliders(QWidget *parent = nullptr);
    explicit Dialog_sliders(const QString &tile, QWidget *parent = nullptr);

    ~Dialog_sliders();

    unsigned getValue(int sliderId) const;
    void addSlider(const QString &label, const QString &suffix, unsigned min, unsigned max, unsigned value);

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::Dialog_sliders *ui;
};

#endif // DIALOG_SLIDERS_H
