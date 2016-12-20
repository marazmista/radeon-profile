
#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include "dialog_rpevent.h"
#include "globalStuff.h"

void radeon_profile::on_btn_addEvent_clicked()
{
    Dialog_RPEvent *d = new Dialog_RPEvent(this);
    d->setFeatures(device.features, fanProfiles.keys());

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

    if (savedState)  {
        RPEvent e = events.value(ui->l_currentActiveEvent->text());

        // one degree handicap to avid constant activation when on the fence
        data.checkTemperature += 1;

        if (!e.isActivationConditonFullfilled(data))
            revokeEvent();

        return;
    }

    for (RPEvent e : events) {
        if (!e.enabled)
            continue;

        if (e.isActivationConditonFullfilled(data)) {
            activateEvent(e);
            return;
        }
    }
}

void radeon_profile::activateEvent(const RPEvent &rpe) {
    if (!globalStuff::globalConfig.rootMode && !device.daemonConnected())
        return;

    qDebug() << "Activating event: " + rpe.name;

    savedState = new currentStateInfo();
    savedState->profile = static_cast<globalStuff::powerProfiles>(ui->combo_pProfile->currentIndex());
    savedState->powerLevel = static_cast<globalStuff::forcePowerLevels>(ui->combo_pLevel->currentIndex());

    if (device.features.pwmAvailable) {
        for (int i = 0; i < fanProfilesMenu->actions().count(); ++i) {
            if (fanProfilesMenu->actions()[i]->isChecked()) {
                savedState->fanIndex = i;
                break;
            }
        }
    }

    hideEventControls(false);
    ui->l_currentActiveEvent->setText(rpe.name);


    if (rpe.dpmProfileChange > -1)
        device.setPowerProfile(static_cast<globalStuff::powerProfiles>(rpe.dpmProfileChange));

    if (rpe.powerLevelChange > -1)
        device.setForcePowerLevel(static_cast<globalStuff::forcePowerLevels>(rpe.powerLevelChange));

    if (rpe.fanComboIndex > 0) {
        switch (rpe.fanComboIndex) {
            case 1:
                ui->btn_pwmAuto->click();
                break;
            case 2:
                device.setPwmManualControl(true);
                device.setPwmValue(rpe.fixedFanSpeedChange);
                break;
            default:
                ui->btn_pwmProfile->click();
                setCurrentFanProfile(rpe.fanProfileNameChange, fanProfiles.value(rpe.fanProfileNameChange));
                break;
        }
    }
}

void radeon_profile::revokeEvent() {
    qDebug() << "Deactivating event: " + ui->l_currentActiveEvent->text();

    device.setPowerProfile(static_cast<globalStuff::powerProfiles>(savedState->profile));
    device.setForcePowerLevel(static_cast<globalStuff::forcePowerLevels>(savedState->powerLevel));

    if (device.features.pwmAvailable)
        fanProfilesMenu->actions()[savedState->fanIndex]->trigger();

    ui->l_currentActiveEvent->clear();
    hideEventControls(true);

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
}

void radeon_profile::on_btn_modifyEvent_clicked()
{
    if (!ui->list_events->currentItem())
        return;

    Dialog_RPEvent *d = new Dialog_RPEvent(this);
    d->setFeatures(device.features, fanProfiles.keys());
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
