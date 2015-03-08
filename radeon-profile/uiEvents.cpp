// copyright marazmista @ 29.12.2013

// this file contains functions for handle gui events, clicks and others

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QTimer>
#include <QColorDialog>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>

bool closeFromTrayMenu;

//===================================
// === GUI events === //
// == menu forcePowerLevel
void radeon_profile::forceAuto() {
    ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(dpm_auto));

   // device.setForcePowerLevel(globalStuff::F_AUTO);
}

void radeon_profile::forceLow() {
    ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(dpm_low));

  //  device.setForcePowerLevel(globalStuff::F_LOW);
}

void radeon_profile::forceHigh() {
    ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(dpm_high));
//    device.setForcePowerLevel(globalStuff::F_HIGH);
}

// == buttons for forcePowerLevel
void radeon_profile::on_btn_forceAuto_clicked()
{
    forceAuto();
}

void radeon_profile::on_btn_forceHigh_clicked()
{
    forceHigh();
}

void radeon_profile::on_btn_forceLow_clicked()
{
    forceLow();
}

// == fan control
void radeon_profile::on_btn_pwmFixedApply_clicked()
{
    device.setPwmValue(ui->pwmSlider->value());
}

void radeon_profile::on_btn_pwmFixed_clicked()
{
    ui->fanModesTabs->setCurrentIndex(0);
    ui->fanModesTabs->setEnabled(true);

    device.setPwmManualControl(true);
    device.setPwmValue(ui->pwmSlider->value());
}

void radeon_profile::on_btn_pwmAuto_clicked()
{
    ui->fanModesTabs->setEnabled(false);

    device.setPwmManualControl(false);
}

void radeon_profile::on_btn_pwmProfile_clicked()
{
    ui->fanModesTabs->setEnabled(true);
    ui->fanModesTabs->setCurrentIndex(1);
}

void radeon_profile::changeProfileFromCombo() {
    int index = ui->combo_pProfile->currentIndex();

    globalStuff::powerProfiles newPP = (globalStuff::powerProfiles)index;

    if (device.features.pm == globalStuff::DPM)
        device.setPowerProfile(newPP);
    else {
        index = index + 3; // frist three in enum is dpm so we need to increase
        device.setPowerProfile(newPP);
    }
}

void radeon_profile::changePowerLevelFromCombo() {
    device.setForcePowerLevel((globalStuff::forcePowerLevels)ui->combo_pLevel->currentIndex());
}

// == others
void radeon_profile::on_btn_dpmBattery_clicked() {
    ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(dpm_battery));

   // device.setPowerProfile(globalStuff::BATTERY);
}

void radeon_profile::on_btn_dpmBalanced_clicked() {
    ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(dpm_balanced));

//    device.setPowerProfile(globalStuff::BALANCED);
}

void radeon_profile::on_btn_dpmPerformance_clicked() {
    ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(dpm_performance));

//    device.setPowerProfile(globalStuff::PERFORMANCE);
}

void radeon_profile::resetMinMax() { device.gpuTemeperatureData.min = 0; device.gpuTemeperatureData.max = 0; }

void radeon_profile::changeTimeRange() {
    rangeX = ui->timeSlider->value();
}

void radeon_profile::on_cb_showFreqGraph_clicked(bool checked)
{
    ui->plotColcks->setVisible(checked);
}

void radeon_profile::on_cb_showTempsGraph_clicked(bool checked)
{
    ui->plotTemp->setVisible(checked);
}

void radeon_profile::on_cb_showVoltsGraph_clicked(bool checked)
{
    ui->plotVolts->setVisible(checked);
}

void radeon_profile::resetGraphs() {
    ui->plotColcks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);
    ui->plotTemp->yAxis->setRange(10,20);
}

void radeon_profile::showLegend(bool checked) {
    ui->plotColcks->legend->setVisible(checked);
    ui->plotVolts->legend->setVisible(checked);
    ui->plotColcks->replot();
    ui->plotVolts->replot();
}

void radeon_profile::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange && ui->cb_minimizeTray->isChecked()) {
        if(isMinimized())
            this->hide();

        event->ignore();
    }
    if (event->type() == QEvent::Close && ui->cb_closeTray) {
        this->hide();
        event->ignore();
    }
}

void radeon_profile::gpuChanged()
{
    device.changeGpu(ui->combo_gpus->currentIndex());
    setupUiEnabledFeatures(device.features);
    timerEvent();
    refreshBtnClicked();
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
    }

    saveConfig();

    if (device.features.pwmAvailable)
        device.setPwmManualControl(false);
}

void radeon_profile::closeFromTray() {
    closeFromTrayMenu = true;
    this->close();
}

void radeon_profile::on_spin_lineThick_valueChanged(int arg1)
{
    setupGraphsStyle();
}

void radeon_profile::on_spin_timerInterval_valueChanged(double arg1)
{
    timer->setInterval(arg1*1000);
}

void radeon_profile::on_cb_graphs_clicked(bool checked)
{
    ui->mainTabs->setTabEnabled(1,checked);
}

void radeon_profile::on_cb_gpuData_clicked(bool checked)
{
    ui->cb_graphs->setEnabled(checked);
    ui->cb_stats->setEnabled(checked);

    if (ui->cb_stats->isChecked())
        ui->tabs_systemInfo->setTabEnabled(3,checked);

    if (ui->cb_graphs->isChecked())
        ui->mainTabs->setTabEnabled(1,checked);

    if (!checked) {
        ui->list_currentGPUData->clear();
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "GPU data is disabled."));
    }
}

void radeon_profile::refreshBtnClicked() {
    ui->list_glxinfo->clear();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));

    ui->list_connectors->clear();
    ui->list_connectors->addTopLevelItems(device.getCardConnectors());

    ui->list_modInfo->clear();
    ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
}

void radeon_profile::on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QColor c = QColorDialog::getColor(item->backgroundColor(1));
    if (c.isValid()) {
        item->setBackgroundColor(1,c);
        // apply colors
        setupGraphsStyle();
    }
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
    QStringList profiles;
    profiles << profile_auto << profile_default << profile_high << profile_mid << profile_low;

    QString selection = QInputDialog::getItem(this,"Select new power profile", "Profile selection",profiles,0,false,&ok);

    if (ok) {
        if (selection == profile_default)
            device.setPowerProfile(globalStuff::DEFAULT);
        else if (selection == profile_auto)
            device.setPowerProfile(globalStuff::AUTO);
        else if (selection == profile_high)
            device.setPowerProfile(globalStuff::HIGH);
        else if (selection == profile_mid)
            device.setPowerProfile(globalStuff::MID);
        else if (selection == profile_low)
            device.setPowerProfile(globalStuff::LOW);
    }
}

void radeon_profile::on_btn_reconfigureDaemon_clicked()
{
    globalStuff::globalConfig.daemonAutoRefresh = ui->cb_daemonAutoRefresh->isChecked();
    globalStuff::globalConfig.interval = ui->spin_timerInterval->value();
    device.reconfigureDaemon();
}

void radeon_profile::on_tabs_execOutputs_tabCloseRequested(int index)
{
    if (execsRunning->at(index)->getExecState() == QProcess::Running) {
        if (QMessageBox::question(this,"","Process is still running. Close tab?",QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
            return;
    }

    ui->tabs_execOutputs->removeTab(index);
    execsRunning->removeAt(index);

    if (ui->tabs_execOutputs->count() == 0)
        on_btn_backToProfiles_clicked();
}


void radeon_profile::on_btn_fanInfo_clicked()
{
    QMessageBox::information(this,"Fan control information",
    "Don't overheat your card! Be careful! \n\nHovewer, card won't apply too low values due its internal protection. Closing application will restore fan state to Auto. If application crashes, the fan state will remain, so you have been warned.");
}
//========
