// copyright marazmista @ 29.12.2013

// this file contains functions for handle gui events, clicks and others

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

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
    device.setPwmValue(ui->fanSpeedSlider->value());
    fanProfilesMenu->actions()[1]->setText(tr("Fixed ") + ui->labelFixedSpeed->text());
}

void radeon_profile::on_btn_pwmFixed_clicked()
{
    ui->btn_pwmFixed->setChecked(true);
    fanProfilesMenu->actions()[1]->setChecked(true);
    ui->fanModesTabs->setCurrentIndex(1);

    device.setPwmManualControl(true);
    device.setPwmValue(ui->fanSpeedSlider->value());
}

void radeon_profile::on_btn_pwmAuto_clicked()
{
    ui->btn_pwmAuto->setChecked(true);
    fanProfilesMenu->actions()[0]->setChecked(true);
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

void radeon_profile::changeProfileFromCombo() {
    device.setPowerProfile(static_cast<PowerProfiles>((device.getDriverFeatures().currentPowerMethod == PowerMethod::DPM) ?
                                                                       ui->combo_pProfile->currentIndex() :
                                                                       ui->combo_pProfile->currentIndex() + 3)); // frist three in enum is dpm so we need to increase
}

void radeon_profile::changePowerLevelFromCombo() {
    device.setForcePowerLevel((ForcePowerLevels)ui->combo_pLevel->currentIndex());
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

void radeon_profile::resetMinMax() { device.gpuData.remove(ValueID::TEMPERATURE_MIN); device.gpuData.remove(ValueID::TEMPERATURE_MAX); }

void radeon_profile::changeTimeRange() {
    rangeX = ui->timeSlider->value();
}

void radeon_profile::on_cb_showFreqGraph_clicked(const bool &checked)
{
    ui->plotClocks->setVisible(checked);
}

void radeon_profile::on_cb_showTempsGraph_clicked(const bool &checked)
{
    ui->plotTemp->setVisible(checked);
}

void radeon_profile::on_cb_showVoltsGraph_clicked(const bool &checked)
{
    ui->plotVolts->setVisible(checked);
}

void radeon_profile::resetGraphs() {
    ui->plotClocks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);
    ui->plotTemp->yAxis->setRange(10,20);
}

void radeon_profile::showLegend(const bool &checked) {
    ui->plotClocks->legend->setVisible(checked);
    ui->plotVolts->legend->setVisible(checked);
    ui->plotClocks->replot();
    ui->plotVolts->replot();
}

void radeon_profile::setGraphOffset(const bool &checked) {
    globalStuff::globalConfig.graphOffset = (checked) ? 20 : 0;
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
    for (execBin * process : execsRunning) {
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

        if (device.gpuData.contains(ValueID::FAN_SPEED_PERCENT))
            device.setPwmManualControl(false);
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents, 50); // Wait for the daemon to disable pwm
    QApplication::quit();
}

void radeon_profile::closeFromTray() {
    closeFromTrayMenu = true;
    this->close();
}

void radeon_profile::on_spin_lineThick_valueChanged(int arg1)
{
    Q_UNUSED(arg1)
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
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("GPU data is disabled")));
    }
}

void radeon_profile::refreshBtnClicked() {
    ui->list_glxinfo->clear();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));

    fillConnectors();

    fillModInfo();
}

void radeon_profile::on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
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

void radeon_profile::on_btn_reconfigureDaemon_clicked()
{
    configureDaemonAutoRefresh(ui->cb_daemonAutoRefresh->isChecked(), ui->spin_timerInterval->value());
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

void radeon_profile::on_cb_enableOverclock_toggled(const bool enable){
    ui->slider_overclock->setEnabled(enable);
    ui->btn_applyOverclock->setEnabled(enable);
    ui->cb_overclockAtLaunch->setEnabled(enable);

    if(enable)
        qDebug() << "Enabling overclock";
    else {
        qDebug() << "Disabling overclock";
        device.resetOverclock();
    }
}

void radeon_profile::on_btn_applyOverclock_clicked(){
    if( ! device.overclock(ui->slider_overclock->value()))
        QMessageBox::warning(this, tr("Error"), tr("An error occurred, overclock failed"));
}

void radeon_profile::on_slider_overclock_valueChanged(const int value){
    ui->label_overclockPercentage->setText(QString::number(value));
}

void radeon_profile::on_btn_export_clicked(){
    QString folder = QFileDialog::getExistingDirectory(this, tr("Export destination directory"));

    if (!folder.isEmpty()) {
        qDebug() << "Exporting graphs into " << folder;

        if (ui->cb_showTempsGraph->isChecked())
            ui->plotTemp->savePng(folder + "/temperature.png");

        if (ui->cb_showFreqGraph->isChecked())
            ui->plotClocks->savePng(folder + "/clocks.png");

        if (ui->cb_showVoltsGraph->isChecked())
            ui->plotVolts->savePng(folder + "/voltages.png");
    }
}

void radeon_profile::on_btn_saveAll_clicked()
{
    saveConfig();
}
