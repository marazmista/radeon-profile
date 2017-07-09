
// copyright marazamista @ 4.07.2017

#include "dialog_topbarcfg.h"
#include "ui_dialog_topbarcfg.h"

Dialog_topbarCfg::Dialog_topbarCfg(QList<TopbarItemDefinitionSchema> s, QList<ValueID> data, const GPUConstParams *params, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_topbarCfg)
{
    ui->setupUi(this);
    schemas = s;
    availableGpuData = data;
    gpuParams = params;

    for (const TopbarItemDefinitionSchema &tis : schemas)
        ui->listWidget->addItem(tis.name);
}

Dialog_topbarCfg::~Dialog_topbarCfg()
{
    delete ui;
}

void Dialog_topbarCfg::setSchemas(QList<TopbarItemDefinitionSchema> s) {
    this->schemas = s;
}

 QList<TopbarItemDefinitionSchema> Dialog_topbarCfg::getCreatedSchemas() {
    return schemas;
}

void Dialog_topbarCfg::on_btn_ok_clicked()
{

    this->setResult(QDialog::Accepted);
    this->accept();
}

void Dialog_topbarCfg::on_btn_cancel_clicked()
{
    this->setResult(QDialog::Rejected);
    this->reject();
}

void Dialog_topbarCfg::on_btn_moveLeft_clicked()
{
    int a = ui->listWidget->currentRow() == ui->listWidget->count() - 1 ? 0 : 1;

    schemas.insert(ui->listWidget->currentRow() - 1, schemas.takeAt(ui->listWidget->currentRow()));
    ui->listWidget->insertItem(ui->listWidget->currentRow() - a, ui->listWidget->takeItem(ui->listWidget->currentRow()));
    ui->listWidget->setCurrentRow(ui->listWidget->currentRow() - (a + 1));
}

void Dialog_topbarCfg::on_btn_moveRight_clicked()
{
    schemas.insert(ui->listWidget->currentRow() + 1, schemas.takeAt(ui->listWidget->currentRow()));
    ui->listWidget->insertItem(ui->listWidget->currentRow() + 1, ui->listWidget->takeItem(ui->listWidget->currentRow()));
    ui->listWidget->setCurrentRow(ui->listWidget->currentRow() + 1);

}

void Dialog_topbarCfg::on_btn_add_clicked()
{
    Dialog_deineTopbarItem *d = new Dialog_deineTopbarItem(&availableGpuData, gpuParams, this);

    if (d->exec() == QDialog::Accepted) {
        schemas.append(d->getCreatedSchema());
        ui->listWidget->addItem(schemas.last().name);
    }

    delete d;
}

void Dialog_topbarCfg::on_btn_modify_clicked()
{
    if (!ui->listWidget->currentItem())
        return;

    Dialog_deineTopbarItem *d = new Dialog_deineTopbarItem(&availableGpuData, gpuParams, this);
    d->setEditedSchema(schemas[ui->listWidget->currentRow()]);

    if (d->exec() == QDialog::Accepted) {
        schemas[ui->listWidget->currentRow()] = d->getCreatedSchema();
        ui->listWidget->currentItem()->setText(schemas[ui->listWidget->currentRow()].name);
    }

    delete d;
}

void Dialog_topbarCfg::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)

    on_btn_modify_clicked();
}

void Dialog_topbarCfg::on_btn_remove_clicked()
{
    if (!ui->listWidget->currentItem())
        return;

    if (QMessageBox::question(this, tr("Question"), tr("Remove selected item?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
        return;

    schemas.removeAt(ui->listWidget->currentRow());
    delete ui->listWidget->currentItem();
}
