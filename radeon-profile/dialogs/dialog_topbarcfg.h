
// copyright marazamista @ 4.07.2017

#ifndef DIALOG_TOPBARCFG_H
#define DIALOG_TOPBARCFG_H

#include <QDialog>
#include <QGridLayout>
#include "components/topbarcomponents.h"
#include "dialogs/dialog_deinetopbaritem.h"

namespace Ui {
class Dialog_topbarCfg;
}

class Dialog_topbarCfg : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_topbarCfg(QList<TopbarItemDefinitionSchema> s, QList<ValueID> data, const GPUConstParams *params, QWidget *parent = 0);
    ~Dialog_topbarCfg();

    void setSchemas(QList<TopbarItemDefinitionSchema> s);
    QList<TopbarItemDefinitionSchema> getCreatedSchemas();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();
    void on_btn_moveLeft_clicked();
    void on_btn_moveRight_clicked();
    void on_btn_add_clicked();
    void on_btn_modify_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_btn_remove_clicked();

private:
    Ui::Dialog_topbarCfg *ui;
    QList<TopbarItemDefinitionSchema> schemas;
    QList<ValueID> availableGpuData;
    const GPUConstParams *gpuParams;
};

#endif // DIALOG_TOPBARCFG_H
