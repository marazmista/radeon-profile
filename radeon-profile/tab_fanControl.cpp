#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMessageBox>
#include <QMenu>
#include <QInputDialog>

void radeon_profile::createDefaultFanProfile() {
    FanProfileSteps p;
    p.insert(0,minFanStepsSpeed);
    p.insert(65,maxFanStepsSpeed);
    p.insert(90,maxFanStepsSpeed);

    fanProfiles.insert("default", p);
    ui->combo_fanProfiles->addItem("default");
    ui->l_currentFanProfile->setText("default");
}

void radeon_profile::makeFanProfileListaAndGraph(const FanProfileSteps &profile) {
    fanSeries->clear();
    ui->list_fanSteps->clear();

    for (int temperature : profile.keys()) {
        fanSeries->append(temperature, profile.value(temperature));
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << QString::number(temperature) << QString::number(profile.value(temperature))));
    }
}

void radeon_profile::makeFanProfilePlot() {
    fanSeries->clear();

    for (int i = 0; i < ui->list_fanSteps->topLevelItemCount(); ++i) {
        fanSeries->append(ui->list_fanSteps->topLevelItem(i)->text(0).toInt(), ui->list_fanSteps->topLevelItem(i)->text(1).toInt());
    }
}

bool radeon_profile::isFanStepValid(const unsigned int temperature, const unsigned int fanSpeed) {
    return temperature <= maxFanStepsTemp &&
            fanSpeed >= minFanStepsSpeed &&
            fanSpeed <= maxFanStepsSpeed;
}

void radeon_profile::addFanStep(const int temperature, const int fanSpeed) {

    if (!isFanStepValid(temperature, fanSpeed)) {
        qWarning() << "Invalid value, can't be inserted into the fan step list:" << temperature << fanSpeed;
        return;
    }

    const QString temperatureString = QString::number(temperature),
            speedString = QString::number(fanSpeed);
    const QList<QTreeWidgetItem*> existing = ui->list_fanSteps->findItems(temperatureString,Qt::MatchExactly);

    if (existing.isEmpty()) { // The element does not exist
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << temperatureString << speedString));
        ui->list_fanSteps->sortItems(0, Qt::AscendingOrder);
    } else // The element exists already, overwrite it
        existing.first()->setText(1,speedString);

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}

void radeon_profile::markFanProfileUnsaved(bool unsaved) {
    ui->l_fanProfileUnsavedIndicator->setVisible(unsaved);
}

void radeon_profile::on_cb_zeroPercentFanSpeed_clicked(bool checked)
{
    if (checked && !askConfirmation(tr("Question"), tr("This option may cause overheat of your card and it is your responsibility if this happens. Do you want to enable it?"))) {
        ui->cb_zeroPercentFanSpeed->setChecked(false);
        return;
    }

    setupMinFanSpeedSetting((checked) ? 0 : 10);
}

void radeon_profile::on_combo_fanProfiles_currentIndexChanged(const QString &arg1)
{
    makeFanProfileListaAndGraph(fanProfiles.value(arg1));
}

void radeon_profile::on_btn_removeFanProfile_clicked()
{
    if (ui->combo_fanProfiles->currentText() == "default") {
        QMessageBox::information(this, "", tr("Cannot remove default profile."), QMessageBox::Ok);
        return;
    }

    if (!askConfirmation("", tr("Remove profile: ")+ui->combo_fanProfiles->currentText()+"?"))
        return;

    fanProfiles.remove(ui->combo_fanProfiles->currentText());
    ui->combo_fanProfiles->removeItem(ui->combo_fanProfiles->currentIndex());
    setupFanProfilesMenu(true);
    setCurrentFanProfile("default", fanProfiles.value("default"));
    saveConfig();
}

void radeon_profile::on_btn_saveFanProfile_clicked()
{
    markFanProfileUnsaved(false);
    fanProfiles.insert(ui->combo_fanProfiles->currentText(), stepsListToMap());
    saveConfig();
}

int radeon_profile::findCurrentFanProfileMenuIndex() {
    for (int i = 0; i < fanProfilesMenu->actions().count(); ++i) {
        if (fanProfilesMenu->actions()[i]->text() == ui->l_currentFanProfile->text())
            return i;
    }

    return 0;
}

void radeon_profile::on_btn_saveAsFanProfile_clicked()
{
    QString name =  QInputDialog::getText(this, "", tr("Fan profile name:"));

    if (name.contains('|')) {
        QMessageBox::information(this, "", tr("Profile name musn't contain '|' character."), QMessageBox::Ok);
        return;
    }

    if (fanProfiles.contains(name)) {
        QMessageBox::information(this, "", tr("Cannot add another profile with the same name that already exists."),QMessageBox::Ok);
        return;
    }

    markFanProfileUnsaved(false);

    fanProfiles.insert(name, stepsListToMap());
    ui->combo_fanProfiles->addItem(name);
    ui->combo_fanProfiles->setCurrentText(name);
    setupFanProfilesMenu(true);
    fanProfilesMenu->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);
    saveConfig();
}

void radeon_profile::on_btn_activateFanProfile_clicked()
{
    if (ui->l_fanProfileUnsavedIndicator->isVisible()) {
        if (!askConfirmation("", tr("Cannot activate unsaved profile. Do you want to save it?")))
            return;

        ui->btn_saveFanProfile->click();
    }

    setCurrentFanProfile(ui->combo_fanProfiles->currentText(), fanProfiles.value(ui->combo_fanProfiles->currentText()));
    fanProfilesMenu->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);
}

void radeon_profile::setCurrentFanProfile(const QString &profileName, const FanProfileSteps &profile) {
    ui->l_currentFanProfile->setText(profileName);
    ui->btn_fanControl->setText(profileName);
    fanProfilesMenu->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);

    currentFanProfile = profile;
    adjustFanSpeed();
}

FanProfileSteps radeon_profile::stepsListToMap() {
    FanProfileSteps steps;
    for (int i = 0; i < ui->list_fanSteps->topLevelItemCount(); ++ i)
        steps.insert(ui->list_fanSteps->topLevelItem(i)->text(0).toInt(),ui->list_fanSteps->topLevelItem(i)->text(1).toInt());

    return steps;
}

void radeon_profile::fanProfileMenuActionClicked(QAction *a) {
    if (a->isSeparator())
        return;

    if (a == fanProfilesMenu->actions()[0] || a == fanProfilesMenu->actions()[1])
        return;

    if (!ui->btn_pwmProfile->isChecked()) {
        ui->btn_pwmProfile->click();
        ui->btn_pwmProfile->setChecked(true);
    }

    setCurrentFanProfile(a->text(),fanProfiles.value(a->text()));
}

void radeon_profile::on_btn_fanInfo_clicked()
{
    QMessageBox::information(this,tr("Fan control information"), tr("Don't overheat your card! Be careful! Don't use this if you don't know what you're doing! \n\nHovewer, looks like card won't apply too low values due its internal protection. \n\nClosing application will restore fan control to Auto. If application crashes, last fan value will remain, so you have been warned!"));
}

void radeon_profile::on_btn_addFanStep_clicked()
{
    const int temperature = askNumber(0, minFanStepsTemp, maxFanStepsTemp, tr("Temperature"));
    if (temperature == -1) // User clicked Cancel
        return;

    if (currentFanProfile.contains(temperature)) // A step with this temperature already exists
        QMessageBox::warning(this, tr("Error"), tr("This step already exists. Double click on it, to change its value"));
    else { // This step does not exist, proceed
        const int fanSpeed = askNumber(0, minFanStepsSpeed, maxFanStepsSpeed, tr("Speed [%]"));
        if (fanSpeed == -1) // User clicked Cancel
            return;

        addFanStep(temperature,fanSpeed);
    }

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}

void radeon_profile::on_btn_removeFanStep_clicked()
{
    QTreeWidgetItem *current = ui->list_fanSteps->currentItem();

    if (ui->list_fanSteps->indexOfTopLevelItem(current) == 0 || ui->list_fanSteps->indexOfTopLevelItem(current) == ui->list_fanSteps->topLevelItemCount()-1) {
        // The selected item is the first or the last, it can't be deleted
        QMessageBox::warning(this, tr("Error"), tr("You can't delete the first and the last item"));
        return;
    }

    // The selected item can be removed, remove it
    int temperature = current->text(0).toInt();

    currentFanProfile.remove(temperature);
    adjustFanSpeed();

    // Remove the step from the list and from the graph
    delete current;

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}


void radeon_profile::on_list_fanSteps_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (ui->list_fanSteps->indexOfTopLevelItem(item) == ui->list_fanSteps->topLevelItemCount()-1) {
        // The selected item is the first or the last, it can't be edited
        QMessageBox::warning(this, tr("Error"), tr("You can't edit the last item"));
        return;
    }

    if (ui->list_fanSteps->indexOfTopLevelItem(item) == 0 && column == 0) {
        QMessageBox::warning(this, tr("Error"), tr("You can't edit temperature of the first item"));
        return;
    }

    switch (column) {
        case 0: {
            int newTemp = askNumber(item->text(column).toInt(), minFanStepsTemp, maxFanStepsTemp, tr("Temperature"));

            if (newTemp == -1)
                return;

            item->setText(column, QString::number(newTemp));
            break;
        }
        case 1: {
            int newSpeed = askNumber(item->text(column).toInt(), minFanStepsSpeed, maxFanStepsSpeed, tr("Speed [%]"));

            if (newSpeed == -1)
                return;

            item->setText(column, QString::number(newSpeed));
            break;
        }
    }

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}

void radeon_profile::on_fanSpeedSlider_valueChanged(int value)
{
    ui->labelFixedSpeed->setText(QString().setNum(value)+"%");
}
