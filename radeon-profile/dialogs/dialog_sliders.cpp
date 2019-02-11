#include "dialog_sliders.h"
#include "ui_dialog_sliders.h"

Dialog_sliders::Dialog_sliders(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_sliders)
{
    ui->setupUi(this);
}

Dialog_sliders::Dialog_sliders(const DialogSlidersConfig &config, const QString &title, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_sliders)
{
    ui->setupUi(this);
    setWindowTitle(title);

    ui->label_value1->setText(config.labels[DialogSet::FIRST]);
    ui->label_value2->setText(config.labels[DialogSet::SECOND]);

    ui->slider_value1->setRange(config.mins[DialogSet::FIRST], config.maxes[DialogSet::FIRST]);
    ui->slider_value2->setRange(config.mins[DialogSet::SECOND], config.maxes[DialogSet::SECOND]);
    ui->spin_value1->setRange(config.mins[DialogSet::FIRST], config.maxes[DialogSet::FIRST]);
    ui->spin_value2->setRange(config.mins[DialogSet::SECOND], config.maxes[DialogSet::SECOND]);

    ui->spin_value1->setSuffix(config.suffixes[DialogSet::FIRST]);
    ui->spin_value2->setSuffix(config.suffixes[DialogSet::SECOND]);

    ui->slider_value1->setValue(config.values[DialogSet::FIRST]);
    ui->slider_value2->setValue(config.values[DialogSet::SECOND]);
}

Dialog_sliders::~Dialog_sliders()
{
    delete ui;
}

void Dialog_sliders::on_btn_ok_clicked()
{
    accept();
}

void Dialog_sliders::on_btn_cancel_clicked()
{
    reject();
}

unsigned Dialog_sliders::getValue(DialogSet set) const {
    switch (set) {
        case DialogSet::FIRST:
            return ui->slider_value1->value();
        case DialogSet::SECOND:
            return ui->slider_value2->value();
    }

    return 0;
}
