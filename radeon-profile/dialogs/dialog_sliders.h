#ifndef DIALOG_SLIDERS_H
#define DIALOG_SLIDERS_H

#include <QDialog>

namespace Ui {
class Dialog_sliders;
}

enum DialogSet {
    FIRST = 0,
    SECOND = 1,
};

struct DialogSlidersConfig {
    QString labels[2], suffixes[2];
    unsigned mins[2], maxes[2], values[2];

    void configureSet(DialogSet set, const QString &label, const QString &suffix, unsigned min, unsigned max, unsigned value) {
        labels[set] = label;
        suffixes[set] = suffix;
        mins[set] = min;
        maxes[set] = max;
        values[set] = value;
    }
};

class Dialog_sliders : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_sliders(QWidget *parent = nullptr);
    explicit Dialog_sliders(const DialogSlidersConfig &config, const QString &tile, QWidget *parent = nullptr);
    ~Dialog_sliders();

    unsigned getValue(DialogSet set) const;

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::Dialog_sliders *ui;
};

#endif // DIALOG_SLIDERS_H
