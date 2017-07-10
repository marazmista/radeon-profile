// copyright marazmista @ 29.12.2013

// this file contains functions for handle gui events, clicks and others

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "dialogs/dialog_topbarcfg.h"

#include <QTimer>
#include <QColorDialog>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QFileDialog>

bool closeFromTrayMenu;

//===================================
// === GUI events === //
// == menu forcePowerLevel
void radeon_profile::forceAuto() {
    ui->combo_pLevel->setCurrentText(dpm_auto);

    // device.setForcePowerLevel(globalStuff::F_AUTO);
}

void radeon_profile::forceLow() {
    ui->combo_pLevel->setCurrentText(dpm_low);

    //  device.setForcePowerLevel(globalStuff::F_LOW);
}

void radeon_profile::forceHigh() {
    ui->combo_pLevel->setCurrentText(dpm_high);
    //    device.setForcePowerLevel(globalStuff::F_HIGH);
}

void radeon_profile::setBattery() {
    setPowerLevel(0);
}

void radeon_profile::setBalanced() {
    setPowerLevel(1);
}

void radeon_profile::setPerformance() {
    setPowerLevel(2);
}

// == fan control
void radeon_profile::on_btn_pwmFixedApply_clicked()
{
    device.setPwmValue(ui->fanSpeedSlider->value());
    fanProfilesMenu->actions()[1]->setText(tr("Fixed ") + ui->labelFixedSpeed->text());
    ui->btn_fanControl->setText(fanProfilesMenu->actions()[1]->text());
}

void radeon_profile::on_btn_pwmFixed_clicked()
{
    ui->btn_pwmFixed->setChecked(true);
    fanProfilesMenu->actions()[1]->setChecked(true);
    ui->btn_fanControl->setText(fanProfilesMenu->actions()[1]->text());
    ui->fanModesTabs->setCurrentIndex(1);

    device.setPwmManualControl(true);
    device.setPwmValue(ui->fanSpeedSlider->value());
}

void radeon_profile::on_btn_pwmAuto_clicked()
{
    ui->btn_pwmAuto->setChecked(true);
    fanProfilesMenu->actions()[0]->setChecked(true);
    ui->btn_fanControl->setText(fanProfilesMenu->actions()[0]->text());
    device.setPwmManualControl(false);
    ui->fanModesTabs->setCurrentIndex(0);
}

void radeon_profile::on_btn_pwmProfile_clicked()
{
    ui->btn_pwmProfile->setChecked(true);
    ui->fanModesTabs->setCurrentIndex(2);

    device.setPwmManualControl(true);
    setCurrentFanProfile(ui->l_currentFanProfile->text(), fanProfiles.value(ui->l_currentFanProfile->text()));
}

void radeon_profile::setPowerLevelFromCombo() {
    if (ui->group_freq->isChecked() && (ForcePowerLevels)ui->combo_pLevel->currentIndex() != ForcePowerLevels::F_MANUAL)
        ui->group_freq->setChecked(false);

    device.setForcePowerLevel((ForcePowerLevels)ui->combo_pLevel->currentIndex());
}

void radeon_profile::resetMinMax() {
    device.gpuData[ValueID::TEMPERATURE_MIN].setValue(device.gpuData.value(ValueID::TEMPERATURE_CURRENT).value);
    device.gpuData[ValueID::TEMPERATURE_MAX].setValue(device.gpuData.value(ValueID::TEMPERATURE_CURRENT).value);
}

void radeon_profile::setPowerLevel(int level) {
            device.setPowerProfile(static_cast<PowerProfiles>((device.getDriverFeatures().currentPowerMethod == PowerMethod::DPM) ?
                                                                               level :
                                                                               level + 3));
}

void radeon_profile::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && ui->cb_minimizeTray->isChecked()) {
        if (isMinimized())
            this->hide();

        event->accept();
        return;
    }
}

void radeon_profile::gpuChanged()
{
    timer->stop();
    device.changeGpu(ui->combo_gpus->currentIndex());
    refreshGpuData();
    setupUiEnabledFeatures(device.getDriverFeatures(), device.gpuData);
    timerEvent();
    refreshBtnClicked();
    timer->start();
}

void radeon_profile::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger: {
        if (isHidden()) {
            this->setWindowFlags(Qt::Window);
            showNormal();
        } else hide();
        break;
    }
    default: break;
    }
}

void radeon_profile::closeEvent(QCloseEvent *e) {
    if (ui->cb_closeTray->isChecked() && !closeFromTrayMenu) {
        this->hide();
        e->ignore();
        return;
    }

    //Check if a process is still running
    for (ExecBin * process : execsRunning) {
        if (process->getExecState() == QProcess::Running
                && !askConfirmation(tr("Quit"), process->name + tr(" is still running, exit anyway?"))) {
            e->ignore();
            return;
        }
    }

    // means app was initialized ok
    if (timer != nullptr) {
        timer->stop();
        delete timer;

        device.finalize();

        saveConfig();
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents, 50); // Wait for the daemon to disable pwm
    QApplication::quit();
}

void radeon_profile::closeFromTray() {
    closeFromTrayMenu = true;
    this->close();
}

void radeon_profile::on_spin_timerInterval_valueChanged(double arg1)
{
    timer->setInterval(arg1*1000);
}

void radeon_profile::refreshBtnClicked() {
    ui->list_glxinfo->clear();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));

    fillConnectors();

    fillModInfo();
}

void radeon_profile::on_cb_stats_clicked(bool checked)
{
    ui->tabs_systemInfo->setTabEnabled(3,checked);

    // reset stats data
    statsTickCounter = 0;
    if (!checked)
        resetStats();
}

void radeon_profile::copyGlxInfoToClipboard() {
    QString clip;
    for (int i = 0; i < ui->list_glxinfo->count(); i++)
        clip += ui->list_glxinfo->item(i)->text() + "\n";

    QApplication::clipboard()->setText(clip);
}

void radeon_profile::copyConnectorsToClipboard(){
    QString clip;
    const QList<QTreeWidgetItem *> selectedItems = ui->list_connectors->selectedItems();

    for(int itemIndex=0; itemIndex < selectedItems.size(); itemIndex++){ // For each item
        QTreeWidgetItem * current = selectedItems.at(itemIndex);
        clip += current->text(0).simplified() + '\t';
        clip += current->text(1).simplified() + '\n';
    }

    QApplication::clipboard()->setText(clip);
}

void radeon_profile::resetStats() {
    statsTickCounter = 0;
    pmStats.clear();
    ui->list_stats->clear();
}

void radeon_profile::on_cb_alternateRow_clicked(bool checked) {
    ui->list_currentGPUData->setAlternatingRowColors(checked);
    ui->list_glxinfo->setAlternatingRowColors(checked);
    ui->list_modInfo->setAlternatingRowColors(checked);
    ui->list_connectors->setAlternatingRowColors(checked);
    ui->list_stats->setAlternatingRowColors(checked);
    ui->list_execProfiles->setAlternatingRowColors(checked);
    ui->list_variables->setAlternatingRowColors(checked);
    ui->list_vaules->setAlternatingRowColors(checked);
}

void radeon_profile::on_chProfile_clicked()
{
    bool ok;

    QString selection = QInputDialog::getItem(this, tr("Select new power profile"), tr("Profile selection"), globalStuff::createProfileCombo(), 0, false, &ok);

    if (ok) {
        if (selection == profile_default)
            device.setPowerProfile(PowerProfiles::DEFAULT);
        else if (selection == profile_auto)
            device.setPowerProfile(PowerProfiles::AUTO);
        else if (selection == profile_high)
            device.setPowerProfile(PowerProfiles::HIGH);
        else if (selection == profile_mid)
            device.setPowerProfile(PowerProfiles::MID);
        else if (selection == profile_low)
            device.setPowerProfile(PowerProfiles::LOW);
    }
}

void radeon_profile::on_tabs_execOutputs_tabCloseRequested(int index)
{
    if (execsRunning.at(index)->getExecState() == QProcess::Running) {
        if (QMessageBox::question(this,"", tr("Process is still running. Close tab?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
            return;
    }

    ui->tabs_execOutputs->removeTab(index);
    execsRunning.removeAt(index);

    if (ui->tabs_execOutputs->count() == 0)
        btnBackToProfilesClicked();
}


int radeon_profile::askNumber(const int value, const int min, const int max, const QString label) {
    bool ok;
    int number = QInputDialog::getInt(this,"",label,value,min,max,1, &ok);

    if (!ok)
        return -1;

    return number;
}

void radeon_profile::applyOc()
{
    device.setOverclockValue(device.getDriverFiles().sysFs.pp_sclk_od, ui->slider_ocSclk->value());
    device.setOverclockValue(device.getDriverFiles().sysFs.pp_mclk_od, ui->slider_ocMclk->value());
}

void radeon_profile::on_btn_applyOverclock_clicked() {
    if (ui->group_oc->isChecked())
        applyOc();

    if (ui->group_freq->isChecked()) {

    }
}

void radeon_profile::on_group_oc_toggled(bool arg1)
{
    if (!device.isInitialized())
        return;

    if (arg1)
        applyOc();
    else
        device.resetOverclock();
}

void radeon_profile::on_group_freq_toggled(bool arg1)
{
    if (!device.isInitialized())
        return;

    if (arg1) {
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

        ui->slider_freqSclk->setValue(device.getCurrentPowerPlayTableId(device.getDriverFiles().sysFs.pp_dpm_sclk));
        if (device.getDriverFeatures().freqMemAvailable)
            ui->slider_freqMclk->setValue(device.getCurrentPowerPlayTableId(device.getDriverFiles().sysFs.pp_dpm_mclk));
    } else
        device.setForcePowerLevel(ForcePowerLevels::F_AUTO);
}

void radeon_profile::on_slider_ocSclk_valueChanged(const int value)
{
    ui->l_ocSclk->setText(QString::number(value) + "%");
}

void radeon_profile::on_slider_ocMclk_valueChanged(const int value)
{
    ui->l_ocMclk->setText(QString::number(value) + "%");
}

void radeon_profile::on_slider_freqSclk_valueChanged(int value)
{
    ui->l_freqSclk->setText(device.getDriverFeatures().sclkTable.value(value));
}

void radeon_profile::on_slider_freqMclk_valueChanged(int value)
{
    ui->l_freqMclk->setText(device.getDriverFeatures().mclkTable.value(value));

}

void radeon_profile::on_btn_saveAll_clicked()
{
    saveConfig();
}

void radeon_profile::on_slider_timeRange_valueChanged(int value)
{
    plotManager.setTimeRange(value);
}

void radeon_profile::on_cb_daemonData_clicked(bool checked)
{
    if (!checked)
        return;

    if (device.gpuList[ui->combo_gpus->currentIndex()].module == DriverModule::RADEON)
        return;

    if (checked && !askConfirmation(tr("Question"), tr("Kernel: ") + QSysInfo::kernelVersion()+ "\n" + tr("Module used: ") +
            device.gpuList[ui->combo_gpus->currentIndex()].driverModuleString + "\n\n" +
            tr("Checking this option may cause problems and is not recommended when you use Linux 4.12<= and amdgpu module.\n\nDo you want to check it anyway?"))) {

        ui->cb_daemonData->setChecked(false);
    }
}

void radeon_profile::pauseRefresh(bool checked)
{
    if (checked)
        timer->stop();
    else
        timer->start();
}

void radeon_profile::on_btn_general_clicked()
{
    ui->btn_general->showMenu();
}

void radeon_profile::on_btn_configureTopbar_clicked()
{
    Dialog_topbarCfg *d = new Dialog_topbarCfg(topbarManager.schemas, device.gpuData.keys(), &device.getGpuConstParams(), this);

    if (d->exec() == QDialog::Accepted) {
        topbarManager.schemas = d->getCreatedSchemas();
        topbarManager.createTopbar(ui->topbar_layout);
    }

    delete d;
}
