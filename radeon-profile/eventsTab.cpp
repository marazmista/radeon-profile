
#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include "dialog_rpevent.h"
#include "globalStuff.h"

#include <QMessageBox>

void radeon_profile::on_btn_addEvent_clicked()
{
    Dialog_RPEvent *d = new Dialog_RPEvent(this);
    d->setFeatures(device.getDriverFeatures(), fanProfiles.keys());

    if (d->exec() == QDialog::Accepted) {
        RPEvent rpe = d->getCreatedEvent();
        events.insert(rpe.name, rpe);

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setCheckState(0, (rpe.enabled) ? Qt::Checked : Qt::Unchecked);
        item->setText(1, rpe.name);

        ui->list_events->addTopLevelItem(item);
    }

    delete d;
}

void radeon_profile::checkEvents() {
    checkInfoStruct data;
    data.checkTemperature = device.gpuTemeperatureData.current;

    if (savedState != nullptr)  {
        RPEvent e = events.value(ui->l_currentActiveEvent->text());

        // one degree handicap to avid constant activation when on the fence
        data.checkTemperature += 1;

        if (!e.isActivationConditonFulfilled(data))
            revokeEvent();

        return;
    }

    for (RPEvent e : events) {
        if (!e.enabled)
            continue;

        if (e.isActivationConditonFulfilled(data)) {
            activateEvent(e);
            return;
        }
    }
}

void radeon_profile::activateEvent(const RPEvent &rpe) {
    if (!device.getDriverFeatures().canChangeProfile)
        return;

    qDebug() << "Activating event: " + rpe.name;

    savedState = new currentStateInfo();
    savedState->profile = static_cast<globalStuff::powerProfiles>(ui->combo_pProfile->currentIndex());
    savedState->powerLevel = static_cast<globalStuff::forcePowerLevels>(ui->combo_pLevel->currentIndex());

    if (device.getDriverFeatures().pwmAvailable) {
        switch (ui->fanModesTabs->currentIndex()) {
            case 0:
            case 1:
             savedState->fanIndex = ui->fanModesTabs->currentIndex();
                break;
            case 2:
                savedState->fanProfileName = ui->l_currentFanProfile->text();
                savedState->fanIndex = ui->fanModesTabs->currentIndex();
                break;
        }
    }

    hideEventControls(false);
    ui->l_currentActiveEvent->setText(rpe.name);


    if (rpe.dpmProfileChange > -1)
        device.setPowerProfile(static_cast<globalStuff::powerProfiles>(rpe.dpmProfileChange));

    if (rpe.powerLevelChange > -1)
        device.setForcePowerLevel(static_cast<globalStuff::forcePowerLevels>(rpe.powerLevelChange));

    if (rpe.fanComboIndex > 0 && device.getDriverFeatures().pwmAvailable) {
        switch (rpe.fanComboIndex) {
            case 1:
                ui->btn_pwmAuto->click();
                break;
            case 2:
                device.setPwmManualControl(true);
                device.setPwmValue(rpe.fixedFanSpeedChange);
                break;
            default:
                if (!fanProfiles.contains(rpe.fanProfileNameChange))
                    break;

                ui->l_currentFanProfile->setText(rpe.fanProfileNameChange);
                ui->btn_pwmProfile->click();
                break;
        }
    }
}

void radeon_profile::revokeEvent() {
    qDebug() << "Deactivating event: " + ui->l_currentActiveEvent->text();

    device.setPowerProfile(static_cast<globalStuff::powerProfiles>(savedState->profile));
    device.setForcePowerLevel(static_cast<globalStuff::forcePowerLevels>(savedState->powerLevel));

    if (device.getDriverFeatures().pwmAvailable) {
        switch (savedState->fanIndex) {
            case 0:
                ui->btn_pwmAuto->click();
                break;
            case 1:
                ui->btn_pwmFixed->click();
                break;
            default:
                if (!fanProfiles.contains(savedState->fanProfileName))
                    break;

                ui->l_currentFanProfile->setText(savedState->fanProfileName);
                ui->btn_pwmProfile->click();
        }

    }

    ui->l_currentActiveEvent->clear();
    hideEventControls(true);

    delete savedState;
    savedState = nullptr;
}

void radeon_profile::hideEventControls(bool hide) {
    ui->l_currentActiveEvent->setVisible(!hide);
    ui->label_18->setVisible(!hide);
    ui->btn_revokeEvent->setVisible(!hide);
}

void radeon_profile::on_list_events_itemChanged(QTreeWidgetItem *item, int column)
{
    RPEvent e = events.value(item->text(1));
    e.enabled = (item->checkState(column) == Qt::Checked);

    events.insert(e.name, e);
}

void radeon_profile::on_btn_eventsInfo_clicked()
{
    QMessageBox::information(this, tr("Events info"),
                             tr("Here you can define events. Each event has a defined condition, and when this condition is fulfilled, event is activated. \n\n"
                                "After activation, defined power profile, power level and fan profile are applied. When one of events is activated, tracking is suspended.\n\n"
                                "When active event condition is no longer true, event is revoked and power profile, power level and fan profile are restored to state before event was activated."),
                             QMessageBox::Ok);
}

void radeon_profile::on_btn_modifyEvent_clicked()
{
    if (!ui->list_events->currentItem())
        return;

    Dialog_RPEvent *d = new Dialog_RPEvent(this);
    d->setFeatures(device.getDriverFeatures(), fanProfiles.keys());
    d->setEditedEvent(events.value(ui->list_events->currentItem()->text(1)));

    if (d->exec() == QDialog::Accepted) {
        RPEvent rpe = d->getCreatedEvent();
        events.insert(rpe.name, rpe);

        if (rpe.name == ui->list_events->currentItem()->text(1)) {
            ui->list_events->currentItem()->setCheckState(0, (rpe.enabled) ? Qt::Checked : Qt::Unchecked);
            ui->list_events->currentItem()->setText(1, rpe.name);
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setCheckState(0, (rpe.enabled) ? Qt::Checked : Qt::Unchecked);
            item->setText(1, rpe.name);
            ui->list_events->addTopLevelItem(item);
        }
    }

    delete d;
}

void radeon_profile::on_btn_removeEvent_clicked()
{
    if (!ui->list_events->currentItem())
        return;

    if (ui->list_events->currentItem()->text(1) == ui->l_currentActiveEvent->text()) {
        QMessageBox::information(this, "", tr("Cannot remove event that is currently active."));
        return;
    }

    if (!askConfirmation("", tr("Do you want to remove event: ")+ui->list_events->currentItem()->text(1)+"?"))
        return;

    events.remove(ui->list_events->currentItem()->text(1));
    delete ui->list_events->currentItem();
}

void radeon_profile::on_btn_revokeEvent_clicked()
{
    revokeEvent();
}

void radeon_profile::on_list_events_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(item)
    Q_UNUSED(column);

    ui->btn_modifyEvent->click();
}
