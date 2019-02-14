#include "dialog_sliders.h"
#include "ui_dialog_sliders.h"
#include <components/slider.h>

Dialog_sliders::Dialog_sliders(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_sliders)
{
    ui->setupUi(this);
}

Dialog_sliders::Dialog_sliders(const QString &title, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_sliders)
{
    ui->setupUi(this);
    setWindowTitle(title);
}

Dialog_sliders::~Dialog_sliders()
{
    delete ui;
}

void Dialog_sliders::addSlider(const QString &label, const QString &suffix, unsigned int min, unsigned int max, unsigned int value) {
    ui->verticalLayout->addWidget(new Slider(label, suffix, min, max, value, this));
}

void Dialog_sliders::on_btn_ok_clicked()
{
    accept();
}

void Dialog_sliders::on_btn_cancel_clicked()
{
    reject();
}

unsigned Dialog_sliders::getValue(int sliderId) const {
    if (sliderId > ui->verticalLayout->count() - 1)
        return 0;

    return static_cast<Slider*>(ui->verticalLayout->itemAt(sliderId)->widget())->getValue();
}
