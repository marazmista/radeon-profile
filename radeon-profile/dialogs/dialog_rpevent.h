#ifndef DIALOG_EVENT_H
#define DIALOG_EVENT_H

#include "radeon_profile.h"

#include <QDialog>


namespace Ui {
    class Dialog_RPEvent;
}

class Dialog_RPEvent : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_RPEvent(QWidget *parent = 0);
    ~Dialog_RPEvent();
    void setFeatures(const GPUDataContainer &gpuData, const DriverFeatures &features, const QList<QString> &profiles);
    void setEditedEvent(const RPEvent &rpe);
    RPEvent getCreatedEvent();

private slots:
    void on_btn_cancel_clicked();
    void on_btn_save_clicked();
    void on_combo_fanChange_currentIndexChanged(int index);
    void on_btn_setBinary_clicked();

private:
    void setFixedFanSpeedVisibility(bool visibility);

    Ui::Dialog_RPEvent *ui;
    RPEvent createdEvent;
};

#endif // DIALOG_EVENT_H
