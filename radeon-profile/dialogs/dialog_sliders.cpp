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

    ui->label_value1->setText(config.label1);
    ui->label_value2->setText(config.label2);

    ui->slider_value1->setRange(config.min1, config.max1);
    ui->slider_value2->setRange(config.min2, config.max2);

    ui->slider_value1->setValue(config.value1);
    ui->slider_value2->setValue(config.value2);

    ui->spin_value1->setRange(config.min1, config.max1);
    ui->spin_value2->setRange(config.min2, config.max2);

    ui->spin_value1->setSuffix(config.suffix1);
    ui->spin_value2->setSuffix(config.suffix2);
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

unsigned Dialog_sliders::getValue1() const {
    return ui->slider_value1->value();
}

unsigned Dialog_sliders::getValue2() const {
    return ui->slider_value2->value();
}
