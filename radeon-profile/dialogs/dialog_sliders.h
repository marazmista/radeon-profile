#ifndef DIALOG_SLIDERS_H
#define DIALOG_SLIDERS_H

#include <QDialog>

namespace Ui {
class Dialog_sliders;
}

struct DialogSlidersConfig {
    QString label1, label2, suffix1, suffix2;
    unsigned min1, max1, min2, max2, value1, value2;
};

class Dialog_sliders : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_sliders(QWidget *parent = nullptr);
    explicit Dialog_sliders(const DialogSlidersConfig &config, const QString &tile, QWidget *parent = nullptr);
    ~Dialog_sliders();

    unsigned getValue1() const;
    unsigned getValue2() const;

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::Dialog_sliders *ui;
};

#endif // DIALOG_SLIDERS_H
