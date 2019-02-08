#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include "dialogs/dialog_sliders.h"

#include <QMessageBox>
#include <QMenu>

void radeon_profile::createDefaultFanProfile() {
    FanProfileSteps p;

    p.insert(40, 35);
    p.insert(65, maxFanStepSpeed);
    fanProfiles.insert("default", p);
}

void radeon_profile::createFanProfileListaAndGraph(const QString &profileName) {
    auto profile = fanProfiles.value(profileName);
    auto series = static_cast<QLineSeries*>(chartView_fan->chart()->series()[0]);

    series->clear();
    ui->list_fanSteps->clear();

    series->append(0, profile.first());

    for (int temperature : profile.keys()) {
        series->append(temperature, profile.value(temperature));
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << QString::number(temperature) << QString::number(profile.value(temperature))));
    }

    series->append(100, profile.last());
}

void radeon_profile::makeFanProfilePlot() {
    auto series = static_cast<QLineSeries*>(chartView_fan->chart()->series()[0]);
    series->clear();

    series->append(0, ui->list_fanSteps->topLevelItem(0)->text(1).toInt());

    for (int i = 0; i < ui->list_fanSteps->topLevelItemCount(); ++i)
        series->append(ui->list_fanSteps->topLevelItem(i)->text(0).toInt(), ui->list_fanSteps->topLevelItem(i)->text(1).toInt());


    series->append(100, ui->list_fanSteps->topLevelItem(ui->list_fanSteps->topLevelItemCount() - 1)->text(1).toInt());
}

void radeon_profile::addFanStep(const int temperature, const int fanSpeed) {
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
    const auto fanProfile = stepsListToMap();
    fanProfiles.insert(ui->combo_fanProfiles->currentText(), fanProfile);
    saveConfig();

    if (ui->combo_fanProfiles->currentText() == ui->l_currentFanProfile->text())
        currentFanProfile = fanProfile;
}

int radeon_profile::findCurrentFanProfileMenuIndex() {
    auto menu_fanProfile = ui->btn_fanControl->menu();

    for (int i = 0; i < menu_fanProfile->actions().count(); ++i) {
        if (menu_fanProfile->actions()[i]->text() == ui->l_currentFanProfile->text())
            return i;
    }

    return 0;
}

void radeon_profile::on_btn_saveAsFanProfile_clicked()
{
    QString name =  QInputDialog::getText(this, "", tr("Fan profile name:"));

    if (fanProfiles.contains(name)) {
        QMessageBox::information(this, "", tr("Cannot add another profile with the same name that already exists."),QMessageBox::Ok);
        return;
    }

    markFanProfileUnsaved(false);

    fanProfiles.insert(name, stepsListToMap());
    ui->combo_fanProfiles->addItem(name);
    ui->combo_fanProfiles->setCurrentText(name);
    setupFanProfilesMenu(true);
    ui->btn_fanControl->menu()->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);
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
    ui->btn_fanControl->menu()->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);
}

void radeon_profile::setCurrentFanProfile(const QString &profileName, const FanProfileSteps &profile) {
    ui->l_currentFanProfile->setText(profileName);
    ui->btn_fanControl->setText(profileName);
    ui->btn_fanControl->menu()->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);

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

    if (a == ui->btn_fanControl->menu()->actions()[0] || a == ui->btn_fanControl->menu()->actions()[1])
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
    const int temperature = askNumber(0, minFanStepTemperature, maxFanStepTemperature, tr("Temperature"));
    if (temperature == -1) // User clicked Cancel
        return;

    if (currentFanProfile.contains(temperature)) // A step with this temperature already exists
        QMessageBox::warning(this, tr("Error"), tr("This step already exists. Double click on it, to change its value"));
    else { // This step does not exist, proceed
        const int fanSpeed = askNumber(0, minFanStepSpeed, maxFanStepSpeed, tr("Speed [%]"));
        if (fanSpeed == -1) // User clicked Cancel
            return;

        addFanStep(temperature,fanSpeed);
    }

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}

void radeon_profile::on_btn_removeFanStep_clicked()
{
    // at least one element must stay
    if (ui->list_fanSteps->topLevelItemCount() == 1)
        return;

    QTreeWidgetItem *current = ui->list_fanSteps->takeTopLevelItem(ui->list_fanSteps->currentIndex().row());

    // The selected item can be removed, remove it
    currentFanProfile.remove(current->text(0).toInt());
    adjustFanSpeed();

    // Remove the step from the list and from the graph
    delete current;

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}


void radeon_profile::on_list_fanSteps_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    auto index = ui->list_fanSteps->currentIndex().row();

    DialogSlidersConfig dialogConfig;
    dialogConfig.label1 = tr("Temperature");
    dialogConfig.label2 = tr("Fan speed");

    dialogConfig.suffix1 = QString::fromUtf8("\u00B0C");
    dialogConfig.suffix2 = "%";

    dialogConfig.min1 = (index > 0) ? ui->list_fanSteps->topLevelItem(index - 1)->text(0).toInt() + 1 : minFanStepTemperature;
    dialogConfig.max1 = (index < ui->list_fanSteps->topLevelItemCount() - 1) ? ui->list_fanSteps->topLevelItem(index + 1)->text(0).toInt() - 1 : maxFanStepTemperature;

    dialogConfig.min2 = (index > 0) ? ui->list_fanSteps->topLevelItem(index - 1)->text(1).toInt() + 1 : minFanStepSpeed;
    dialogConfig.max2 = (index < ui->list_fanSteps->topLevelItemCount() - 1) ? ui->list_fanSteps->topLevelItem(index + 1)->text(1).toInt() - 1 : maxFanStepSpeed;

    dialogConfig.value1 = item->text(0).toUInt();
    dialogConfig.value2 = item->text(1).toUInt();

    auto d = new Dialog_sliders(dialogConfig, tr("Adjust fan step"), this);

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    item->setText(0, QString::number(d->getValue1()));
    item->setText(1, QString::number(d->getValue2()));

    markFanProfileUnsaved(true);
    makeFanProfilePlot();

    delete d;
}

void radeon_profile::on_slider_fanSpeed_valueChanged(int value)
{
    ui->labelFixedSpeed->setText(QString::number(value)+"%");
}
