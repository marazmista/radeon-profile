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
    fanProfilesMenu->actions()[1]->setText(tr("Fixed ") + ui->labelFixedSpeed->text()+"%");
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
    int index = ui->combo_pProfile->currentIndex();

    globalStuff::powerProfiles newPP = (globalStuff::powerProfiles)index;

    if (device.features.pm == globalStuff::DPM)
        device.setPowerProfile(newPP);
    else {
        index += 3; // frist three in enum is dpm so we need to increase
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
        return;
    }

    //Check if a process is still running
    for(execBin * process : *execsRunning) {
        if(process->getExecState() == QProcess::Running
                && ! askConfirmation(tr("Quit"), process->name + tr(" is still running, exit anyway?"))) {
            e->ignore();
            return;
        }
    }

    timer->stop();
    saveConfig();

    if (device.features.pwmAvailable) {
        device.setPwmManualControl(false);
        saveFanProfiles();
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
    QStringList profiles;
    profiles << profile_auto << profile_default << profile_high << profile_mid << profile_low;

    QString selection = QInputDialog::getItem(this, tr("Select new power profile"), tr("Profile selection"), profiles,0,false,&ok);

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
    configureDaemonAutoRefresh(ui->cb_daemonAutoRefresh->isChecked(), ui->spin_timerInterval->value());
}

void radeon_profile::on_tabs_execOutputs_tabCloseRequested(int index)
{
    if (execsRunning->at(index)->getExecState() == QProcess::Running) {
        if (QMessageBox::question(this,"", tr("Process is still running. Close tab?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
            return;
    }

    ui->tabs_execOutputs->removeTab(index);
    execsRunning->removeAt(index);

    if (ui->tabs_execOutputs->count() == 0)
        btnBackToProfilesClicked();
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
    ui->plotFanProfile->graph(0)->removeData(temperature);
    ui->plotFanProfile->replot();
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

    const int oldTemp = item->text(0).toInt(), oldSpeed = item->text(1).toInt();
    int newTemp, newSpeed;

    if (column == 0) { // The user wants to change the temperature
        newTemp = askNumber(oldTemp, minFanStepsTemp, maxFanStepsTemp, tr("Temperature"));
        if(newTemp != -1){
            newSpeed = oldSpeed;
            currentFanProfile.remove(oldTemp);
            delete item;
            ui->plotFanProfile->graph(0)->removeData(oldTemp);
            addFanStep(newTemp,newSpeed);
        }
    } else { // The user wants to change the speed
        newTemp = oldTemp;
        newSpeed = askNumber(oldSpeed, minFanStepsSpeed, maxFanStepsSpeed, tr("Speed [%] (10-100)"));
        // addFanStep() will check the validity of newSpeed and overwrite the current step
        addFanStep(newTemp,newSpeed);
    }
}

int radeon_profile::askNumber(const int value, const int min, const int max, const QString label) {
    bool ok;
    int number = QInputDialog::getInt(this,"",label,value,min,max,1, &ok);

    if (!ok)
        return -1;

    return number;
}

void radeon_profile::on_fanSpeedSlider_valueChanged(int value)
{
    ui->labelFixedSpeed->setText(QString().setNum(((float)value / device.features.pwmMaxSpeed) * 100,'f',0));
}

//========

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

    fanProfiles.remove(ui->combo_fanProfiles->currentText());
    ui->combo_fanProfiles->removeItem(ui->combo_fanProfiles->currentIndex());
    setupFanProfilesMenu(true);
    setCurrentFanProfile("default", fanProfiles.value("default"));
}

void radeon_profile::on_btn_saveFanProfile_clicked()
{
    fanProfiles.insert(ui->combo_fanProfiles->currentText(), stepsListToMap());
    setCurrentFanProfile(ui->combo_fanProfiles->currentText(),stepsListToMap());
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

    ui->combo_fanProfiles->addItem(name);
    fanProfiles.insert(name, stepsListToMap());
    setupFanProfilesMenu(true);
}

void radeon_profile::on_btn_activateFanProfile_clicked()
{
    setCurrentFanProfile(ui->combo_fanProfiles->currentText(), fanProfiles.value(ui->combo_fanProfiles->currentText()));
    fanProfilesMenu->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);
}

void radeon_profile::setCurrentFanProfile(const QString &profileName, const fanProfileSteps &profile) {
    ui->l_currentFanProfile->setText(profileName);
    fanProfilesMenu->actions()[findCurrentFanProfileMenuIndex()]->setChecked(true);

    currentFanProfile = profile;
    adjustFanSpeed();
}


radeon_profile::fanProfileSteps radeon_profile::stepsListToMap() {
    fanProfileSteps steps;
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

void radeon_profile::on_cb_zeroPercentFanSpeed_clicked(bool checked)
{
    if (checked && !askConfirmation(tr("Question"), tr("This option may cause overheat of your card and it is your responsibility if this happens. Do you want to enable it?"))) {
        ui->cb_zeroPercentFanSpeed->setChecked(false);
        return;
    }

    setupMinFanSpeedSetting((checked) ? 0 : 10);
}

void radeon_profile::setupMinFanSpeedSetting(unsigned int speed) {
    minFanStepsSpeed = speed;
    ui->l_minFanSpeed->setText(QString::number(minFanStepsSpeed)+"%");
    ui->fanSpeedSlider->setMinimum((minFanStepsSpeed * 255) / 100);
}
