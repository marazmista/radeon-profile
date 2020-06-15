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

void radeon_profile::addFanStep(const unsigned int temperature, const unsigned int fanSpeed) {
    const QString temperatureString = QString::number(temperature);
    QString speedString = QString::number(fanSpeed);
    const QList<QTreeWidgetItem*> existing = ui->list_fanSteps->findItems(temperatureString, Qt::MatchExactly);

    if (existing.isEmpty()) {

        // find right place in list to insert new item
        int i = 0;
        while (i < ui->list_fanSteps->topLevelItemCount() && ui->list_fanSteps->topLevelItem(i)->text(0).toUInt() < temperature)
            ++i;

        // check previous and next items values before insering new
        if (ui->list_fanSteps->topLevelItem(i - 1) != nullptr && ui->list_fanSteps->topLevelItem(i - 1)->text(1).toUInt() > fanSpeed)
            speedString = ui->list_fanSteps->topLevelItem(i - 1)->text(1);

        if (ui->list_fanSteps->topLevelItem(i) != nullptr && ui->list_fanSteps->topLevelItem(i)->text(1).toUInt() < fanSpeed)
            speedString = ui->list_fanSteps->topLevelItem(i)->text(1);

        ui->list_fanSteps->insertTopLevelItem(i, new QTreeWidgetItem(QStringList() << temperatureString << speedString));
    } else
        existing.first()->setText(1, speedString);

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
    createFanProfilesMenu(true);
    setCurrentFanProfile("default");
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

void radeon_profile::on_btn_saveAsFanProfile_clicked()
{
    QString name =  QInputDialog::getText(this, "", tr("Fan profile name:"));

    if (name.isEmpty())
        return;

    if (fanProfiles.contains(name)) {
        QMessageBox::information(this, "", tr("Cannot add another profile with the same name that already exists."),QMessageBox::Ok);
        return;
    }

    markFanProfileUnsaved(false);

    fanProfiles.insert(name, stepsListToMap());
    ui->combo_fanProfiles->addItem(name);
    ui->combo_fanProfiles->setCurrentText(name);
    createFanProfilesMenu(true);
    ui->btn_fanControl->menu()->actions()[findCurrentMenuIndex(ui->btn_fanControl->menu(), ui->l_currentFanProfile->text())]->setChecked(true);
    saveConfig();
}

void radeon_profile::on_btn_applyFanProfile_clicked()
{
    if (ui->l_fanProfileUnsavedIndicator->isVisible()) {
        if (!askConfirmation("", tr("Cannot apply unsaved profile. Do you want to save it?")))
            return;

        ui->btn_saveFanProfile->click();
    }

    setCurrentFanProfile(ui->combo_fanProfiles->currentText());
}

void radeon_profile::on_btn_pwmFixedApply_clicked()
{
    device.setPwmValue(ui->slider_fanSpeed->value());
    ui->btn_fanControl->menu()->actions()[1]->setText(tr("Fixed ") + ui->spin_fanFixedSpeed->text());
    ui->btn_fanControl->setText(ui->btn_fanControl->menu()->actions()[1]->text());
}

void radeon_profile::on_btn_pwmFixed_clicked()
{
    ui->btn_pwmFixed->setChecked(true);
    ui->btn_fanControl->menu()->actions()[1]->setChecked(true);
    ui->btn_fanControl->setText(ui->btn_fanControl->menu()->actions()[1]->text());
    ui->stack_fanModes->setCurrentIndex(1);

    device.setPwmManualControl(true);
    device.setPwmValue(ui->slider_fanSpeed->value());
}

void radeon_profile::on_btn_pwmAuto_clicked()
{
    ui->btn_pwmAuto->setChecked(true);
    ui->btn_fanControl->menu()->actions()[0]->setChecked(true);
    ui->btn_fanControl->setText(ui->btn_fanControl->menu()->actions()[0]->text());
    device.setPwmManualControl(false);
    ui->stack_fanModes->setCurrentIndex(0);
}

void radeon_profile::on_btn_pwmProfile_clicked()
{
    ui->btn_pwmProfile->setChecked(true);
    ui->stack_fanModes->setCurrentIndex(2);

    device.setPwmManualControl(true);
    setCurrentFanProfile(ui->l_currentFanProfile->text());
}

void radeon_profile::setCurrentFanProfile(const QString &profileName) {
    const auto profile =  fanProfiles.value(profileName);

    ui->l_currentFanProfile->setText(profileName);
    ui->btn_fanControl->setText(profileName);
    ui->btn_fanControl->menu()->actions()[findCurrentMenuIndex(ui->btn_fanControl->menu(), profileName)]->setChecked(true);

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

    // these two already connected to on_btn_pwmAuto_clicked and on_btn_pwmFixed_clicked
    if (a == ui->btn_fanControl->menu()->actions()[0] || a == ui->btn_fanControl->menu()->actions()[1])
        return;

    if (!ui->btn_pwmProfile->isChecked()) {
        ui->btn_pwmProfile->click();
        ui->btn_pwmProfile->setChecked(true);
    }

    setCurrentFanProfile(a->text());
}

void radeon_profile::on_btn_fanInfo_clicked()
{
    QMessageBox::information(this,tr("Fan control information"), tr("Don't overheat your card! Be careful! Don't use this if you don't know what you're doing! \n\nHovewer, it looks like your card won't apply too low values due its internal protection. \n\nClosing application will restore fan control to Auto. If application crashes, last fan value will remain, so you have been warned!"));
}

void radeon_profile::on_btn_addFanStep_clicked()
{
    auto d = new Dialog_sliders(tr("Add fan step"), this);

    d->addSlider(tr("Temperature"), QString::fromUtf8("\u00B0C"), minFanStepTemperature, maxFanStepTemperature, 0);
    d->addSlider(tr("Fan speed"), "%", minFanStepSpeed, maxFanStepSpeed, 0);

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    addFanStep(d->getValue(0), d->getValue(1));

    markFanProfileUnsaved(true);
    makeFanProfilePlot();

    delete d;
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

    auto d = new Dialog_sliders(tr("Adjust fan step"), this);

    d->addSlider(tr("Temperature"),
                 QString::fromUtf8("\u00B0C"),
                 (index > 0) ? ui->list_fanSteps->topLevelItem(index - 1)->text(0).toInt() + 1 : minFanStepTemperature,
                 (index < ui->list_fanSteps->topLevelItemCount() - 1) ? ui->list_fanSteps->topLevelItem(index + 1)->text(0).toUInt() - 1 : maxFanStepTemperature,
                 item->text(0).toUInt());

    d->addSlider(tr("Fan speed"), "%",
                 (index > 0) ? ui->list_fanSteps->topLevelItem(index - 1)->text(1).toInt() : minFanStepSpeed,
                 (index < ui->list_fanSteps->topLevelItemCount() - 1) ? ui->list_fanSteps->topLevelItem(index + 1)->text(1).toUInt() : maxFanStepSpeed,
                 item->text(1).toUInt());


    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    item->setText(0, QString::number(d->getValue(0)));
    item->setText(1, QString::number(d->getValue(1)));

    markFanProfileUnsaved(true);
    makeFanProfilePlot();

    delete d;
}
