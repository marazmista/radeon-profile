#ifndef SLIDER_H
#define SLIDER_H

#include <QWidget>
#include "ui_slider.h"

namespace Ui {
class Slider;
}

class Slider : public QWidget
{
    Q_OBJECT

public:
    explicit Slider(QWidget *parent = nullptr) : QWidget(parent),
      ui(new Ui::Slider) {
        ui->setupUi(this);
    };

    explicit Slider(const QString &label, const QString &suffix, unsigned int min, unsigned int max, unsigned int value, QWidget *parent = nullptr) : QWidget(parent),
      ui(new Ui::Slider) {
        ui->setupUi(this);

        setLabel(label);
        setRange(min, max);
        setValue(value);
        setSuffix(suffix);
    };

    ~Slider() {
        delete ui;
    };

    void setLabel(const QString &label) {
        ui->label->setText(label);
    };

    void setSuffix(const QString &suffix) {
        ui->spin->setSuffix(suffix);
    }

    void setValue(unsigned value) {
        ui->slider->setValue(value);
    }

    void setRange(unsigned min, unsigned max) {
        ui->slider->setRange(min, max);
        ui->spin->setRange(min, max);

        ui->slider->setTickInterval((max - min) / 5);
    };

    unsigned getValue() {
        return ui->slider->value();
    };

private:
    Ui::Slider *ui;
};

#endif // SLIDER_H
