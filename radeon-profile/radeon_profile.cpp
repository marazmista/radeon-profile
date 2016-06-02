// copyright marazmista 3.2013

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

//==================
// the logic is simple. radeon-profile create object based on gpu class.
// gpu class decides what driver is used (device.initialize()). Later, based
// on structure driverFeatures, program activate some ui features.

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QTimer>
#include <QTextStream>
#include <QMenu>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QTime>

#define logTime qDebug() << QTime::currentTime().toString("ss.zzz")


radeon_profile::radeon_profile(QStringList a,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);

    // checks if running as root
    if (globalStuff::grabSystemInfo("whoami")[0] == "root") {
         globalStuff::globalConfig.rootMode = true;
        ui->label_rootWarrning->setVisible(true);
    } else {
        globalStuff::globalConfig.rootMode = false;
        ui->label_rootWarrning->setVisible(false);
    }

    logTime << "Setting up ui elements";
    ui->mainTabs->setCurrentWidget(ui->infoTab);
    ui->tabs_systemInfo->setCurrentWidget(ui->tab_glxInfo);
    ui->configGroups->setCurrentWidget(ui->tab_mainConfig);
    ui->list_currentGPUData->setHeaderHidden(false);
    ui->execPages->setCurrentWidget(ui->page_profiles);
    setupGraphs();
    setupForcePowerLevelMenu();
    setupOptionsMenu();
    setupContextMenus();
    setupTrayIcon();

    logTime << "Loading config";
    loadConfig();

    logTime << "Figuring out driver";
    QString params = a.join(" ");
    if (params.contains("--driver xorg")) {
        device.driverByParam(XORG);
        device.initialize(true);
    }
    else if (params.contains("--driver fglrx")) {
        device.driverByParam(FGLRX);
        device.initialize(true);
    }
    else // driver object detects cards in pc and fill the list in ui //
        device.initialize();

    logTime << "Setting up UI elements for available features";
    ui->combo_gpus->addItems(device.gpuList);

    ui->tab_daemonConfig->setEnabled(device.daemonConnected());
    setupUiEnabledFeatures(device.features);

    if((device.features.GPUoverClockAvailable || device.features.memoryOverclockAvailable)
            && ui->cb_enableOverclock->isChecked() && ui->cb_overclockAtLaunch->isChecked())
        ui->btn_applyOverclock->click();

    connectSignals();

    // timer init
    timer.setInterval(ui->spin_timerInterval->value()*1000);

    logTime << "Filling tables with data at the start";
    refreshGpuData();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    ui->list_connectors->addTopLevelItems(device.getCardConnectors());
    ui->list_connectors->expandToDepth(2);
    ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    if(ui->cb_gpuData->isChecked())
        refreshUI();

    logTime << "Starting timer and adding runtime widgets";
    timer.start();
    addRuntimeWidgets();

    showWindow();
    logTime << "Initialization completed";
}

radeon_profile::~radeon_profile()
{
    delete ui;
}

void radeon_profile::connectSignals()
{
    qDebug() << "Connecting runtime signals and slots";
    // fix for warrning: QMetaObject::connectSlotsByName: No matching signal for...
    connect(ui->combo_gpus,SIGNAL(currentIndexChanged(QString)),this,SLOT(gpuChanged()));
    connect(ui->combo_pProfile,SIGNAL(currentIndexChanged(int)),this,SLOT(changeProfileFromCombo()));
    connect(ui->combo_pLevel,SIGNAL(currentIndexChanged(int)),this,SLOT(changePowerLevelFromCombo()));

    connect(&timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    connect(ui->timeSlider,SIGNAL(valueChanged(int)),this,SLOT(changeTimeRange()));
}

void radeon_profile::addRuntimeWidgets() {
    // add button for manual refresh glx info, connectors, mod params
    QPushButton *refreshBtn = new QPushButton();
    refreshBtn->setIcon(QIcon(":/icon/symbols/refresh.png"));
    ui->tabs_systemInfo->setCornerWidget(refreshBtn);
    refreshBtn->setIconSize(QSize(20,20));
    refreshBtn->show();
    connect(refreshBtn,SIGNAL(clicked()),this,SLOT(refreshBtnClicked()));

    ui->label_version->setText(tr("version %n", NULL, appVersion));

    // version label
    QLabel *l = new QLabel("v. " +QString().setNum(appVersion),this);
    QFont f;
    f.setStretch(QFont::Unstretched);
    f.setWeight(QFont::Bold);
    f.setPointSize(8);
    l->setFont(f);
    ui->mainTabs->setCornerWidget(l,Qt::BottomRightCorner);
    l->show();

    // button on exec pages
    QPushButton *btnBackProfiles = new QPushButton();
    btnBackProfiles->setText(tr("Back to profiles"));
    ui->tabs_execOutputs->setCornerWidget(btnBackProfiles);
    btnBackProfiles->show();
    connect(btnBackProfiles,SIGNAL(clicked()),this,SLOT(btnBackToProfilesClicked()));

    // set pwm buttons in group
    pwmGroup.addButton(ui->btn_pwmAuto);
    pwmGroup.addButton(ui->btn_pwmFixed);
    pwmGroup.addButton(ui->btn_pwmProfile);
}

// based on driverFeatures structure returned by gpu class, adjust ui elements
void radeon_profile::setupUiEnabledFeatures(const driverFeatures &features) {
    qDebug() << "Setting up UI elements with enabled features";
    if (features.canChangeProfile && features.pm < PM_UNKNOWN) {
        const bool dpm = features.pm == DPM; // true=dpm, false=profile
        ui->tabs_pm->setCurrentWidget(dpm ? ui->dpmProfiles : ui->stdProfiles);

        ui->stdProfiles->setEnabled( ! dpm);
        changeProfile->setEnabled( ! dpm);

        ui->dpmProfiles->setEnabled(dpm);        
        dpmMenu.setEnabled(dpm);
        ui->combo_pLevel->setEnabled(dpm);
        ui->label_noStdProfiles->setVisible(dpm);
    } else {
        ui->tabs_pm->setEnabled(false);
        changeProfile->setEnabled(false);
        dpmMenu.setEnabled(false);
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->setEnabled(false);
    }

    ui->cb_showFreqGraph->setEnabled(features.coreClockAvailable);
    if(ui->cb_gpuData->isChecked())
        ui->tab_stats->setEnabled(features.coreClockAvailable);
    ui->cb_showVoltsGraph->setEnabled(features.coreVoltAvailable);

    if (!features.temperatureAvailable) {
        ui->cb_showTempsGraph->setEnabled(false);
        ui->plotTemp->setVisible(false);
        ui->l_temp->setVisible(false);
    }

    if (!features.coreClockAvailable && !features.temperatureAvailable && !features.coreVoltAvailable)
        ui->graphTab->setEnabled(false);

    if (features.pwmAvailable && (globalStuff::globalConfig.rootMode || device.daemonConnected())) {
        qDebug() << "Fan control is available , configuring the fan control tab";
        ui->fanSpeedSlider->setMaximum(device.features.pwmMaxSpeed);
        loadFanProfiles();
        on_fanSpeedSlider_valueChanged(ui->fanSpeedSlider->value());
    } else {
        qDebug() << "Fan control is not available";
        ui->fanTab->setEnabled(false);
        ui->l_fanSpeed->setVisible(false);
    }

    if (features.pm == DPM) {
        ui->combo_pProfile->addItems(QStringList() << dpm_battery << dpm_balanced << dpm_performance);
        ui->combo_pLevel->addItems(QStringList() << dpm_auto << dpm_low << dpm_high);

        ui->combo_pProfile->setCurrentIndex(ui->combo_pLevel->findText(device.currentPowerLevel));
        ui->combo_pLevel->setCurrentIndex(ui->combo_pProfile->findText(device.currentPowerLevel));

    } else {
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->addItems(QStringList() << profile_auto << profile_default << profile_high << profile_mid << profile_low);
    }

    if( ! features.GPUoverClockAvailable && ! features.memoryOverclockAvailable){
        ui->label_overclock->setText((globalStuff::globalConfig.rootMode || device.daemonConnected()) ?
                                         tr("Your driver or your GPU does not support overclocking") :
                                         tr("You need debugfs mounted and either root rights or the daemon running"));

        ui->cb_enableOverclock->setChecked(false);
        ui->overclockTab->setEnabled(false);
    } else {
        const bool gpuOcOk = features.GPUoverClockAvailable && ui->cb_enableOverclock->isChecked(),
                memoryOcOk = features.memoryOverclockAvailable && ui->cb_enableOverclock->isChecked();

        ui->label_GPUoc->setEnabled(gpuOcOk);
        ui->slider_GPUoverclock->setEnabled(gpuOcOk);
        ui->label_GPUoverclock->setEnabled(gpuOcOk);

        ui->label_memoryOc->setEnabled(memoryOcOk);
        ui->slider_memoryOverclock->setEnabled(memoryOcOk);
        ui->label_memoryOverclock->setEnabled(memoryOcOk);
    }
}

void radeon_profile::refreshGpuData() {
    device.refreshPowerLevel();
    device.getClocks();
    device.getTemperature();

    if (device.features.pwmAvailable)
        device.getPwmSpeed();

    updateExecLogs();
}

void radeon_profile::addChild(QTreeWidget * parent, const QString &leftColumn, const QString  &rightColumn) {
    parent->addTopLevelItem(new QTreeWidgetItem(QStringList() << leftColumn << rightColumn));
}

// -1 value means that we not show in table. it's default (in gpuClocksStruct constructor), and if we
// did not alter it, it stays and in result will be not displayed
void radeon_profile::refreshUI() {
    ui->l_cClk->setText(device.gpuClocksDataString.coreClk);
    ui->l_mClk->setText(device.gpuClocksDataString.memClk);
    ui->l_mVolt->setText(device.gpuClocksDataString.memVolt);
    ui->l_cVolt->setText(device.gpuClocksDataString.coreVolt);

    // Header - Temperature
    ui->l_temp->setText(device.gpuTemeperatureDataString.current);

    // Header - Fan speed
    ui->l_fanSpeed->setText(device.gpuTemeperatureDataString.pwmSpeed);

    // GPU data list
    if (ui->mainTabs->currentWidget() == ui->infoTab) {
        ui->list_currentGPUData->clear();

        if (device.gpuClocksData.powerLevelOk)
            addChild(ui->list_currentGPUData, tr("Power level"), device.gpuClocksDataString.powerLevel);
        if (device.gpuClocksData.coreClkOk)
            addChild(ui->list_currentGPUData, tr("GPU clock"), device.gpuClocksDataString.coreClk);
        if (device.gpuClocksData.memClkOk)
            addChild(ui->list_currentGPUData, tr("Memory clock"), device.gpuClocksDataString.memClk);
        if (device.gpuClocksData.uvdCClkOk)
            addChild(ui->list_currentGPUData, tr("UVD core clock"), device.gpuClocksDataString.uvdCClk);
        if (device.gpuClocksData.uvdDClkOk)
            addChild(ui->list_currentGPUData, tr("UVD decoder clock"), device.gpuClocksDataString.uvdDClk);
        if (device.gpuClocksData.coreVoltOk)
            addChild(ui->list_currentGPUData, tr("I/O voltage"), device.gpuClocksDataString.coreVolt);
        if (device.gpuClocksData.memVoltOk)
            addChild(ui->list_currentGPUData, tr("GPU voltage"), device.gpuClocksDataString.memVolt);

        if (ui->list_currentGPUData->topLevelItemCount() == 0)
            addChild(ui->list_currentGPUData, tr("Can't read data"), tr("You need debugfs mounted and either root rights or the daemon running"));

        addChild(ui->list_currentGPUData, tr("GPU temperature"), device.gpuTemeperatureDataString.current);
    }
}

void radeon_profile::updateExecLogs() {
    for (execBin *exe : execsRunning) {
        if (exe->state() == QProcess::Running && exe->logEnabled) {
            QString logData = QDateTime::currentDateTime().toString(logDateFormat) +";" + device.gpuClocksDataString.powerLevel + ";" +
                    device.gpuClocksDataString.coreClk + ";"+
                    device.gpuClocksDataString.memClk + ";"+
                    device.gpuClocksDataString.uvdCClk + ";"+
                    device.gpuClocksDataString.uvdDClk + ";"+
                    device.gpuClocksDataString.coreVolt + ";"+
                    device.gpuClocksDataString.memVolt + ";"+
                    device.gpuTemeperatureDataString.current;
            exe->appendToLog(logData);
        }
    }
}

//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent() {
    if (ui->cb_gpuData->isChecked()) {
        if (!refreshWhenHidden->isChecked() && this->isHidden()) {
            // even if in tray, keep the fan control active (if enabled)
            device.getTemperature();
            adjustFanSpeed();
            return;
        }

        refreshGpuData();

        if(ui->cb_showCombo->isChecked()){ // If pLevel and pProfile are visible update their index
            ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device.currentPowerProfile));
            if (device.features.pm == DPM)
                ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(device.currentPowerLevel));
        }

        adjustFanSpeed();

        // lets say coreClk is essential to get stats (it is disabled in ui anyway when features.clocksAvailable is false)
        if (ui->cb_stats->isChecked() && device.gpuClocksData.coreClkOk) {
            doTheStats();

            // do the math only when user looking at stats table
            if (ui->tabs_systemInfo->currentWidget() == ui->tab_stats && ui->mainTabs->currentWidget() == ui->infoTab && !this->isHidden())
                updateStatsTable();
        }

        refreshUI();
        refreshTooltip();

        if (ui->graphTab->isEnabled())
            refreshGraphs();
    }

    if (ui->cb_glxInfo->isChecked()) {
        ui->list_glxinfo->clear();
        ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    }
    if (ui->cb_connectors->isChecked()) {
        ui->list_connectors->clear();
        ui->list_connectors->addTopLevelItems(device.getCardConnectors());
        ui->list_connectors->expandToDepth(2);
    }
    if (ui->cb_modParams->isChecked()) {
        ui->list_modInfo->clear();
        ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    }

}

void radeon_profile::adjustFanSpeed(const bool force)
{
    // If 'force' is true skip all checks.
    if (force ||
            // Otherwise check if PWM is available and enabled by the user and if the temperature has changed
            (device.features.pwmAvailable && ui->btn_pwmProfile->isChecked() &&
            device.gpuTemeperatureData.current != device.gpuTemeperatureData.currentBefore)) {

        float speed;
        if (fanSteps.contains(device.gpuTemeperatureData.current)) // Exact match
            speed = fanSteps.value(device.gpuTemeperatureData.current);
        else {
            // find bounds of current temperature
            const QMap<short, unsigned short>::const_iterator high = fanSteps.upperBound(device.gpuTemeperatureData.current),
                    low = high - 1;
            if(high == fanSteps.constBegin()) // Before the first step
                speed = high.value();
            else if(low == fanSteps.constEnd()) // After the last step
                speed = low.value();
            else  // In the middle
                // calculate two point stright line equation based on boundaries of current temperature
                // y = mx + b = (y2-y1)/(x2-x1)*(x-x1)+y1
                speed=(float)(high.value()-low.value())/(high.key()-low.key())*(device.gpuTemeperatureData.current-low.key())+low.value();
        }

        qDebug() << device.gpuTemeperatureDataString.current << " -->" << speed << "%";
        if(speed >= 0 && speed <= 100)
            device.setPwmValue(device.features.pwmMaxSpeed * speed / 100);
    }
}

void radeon_profile::refreshGraphs() {
    // count the tick and move graph to right
    ticksCounter++;

    // add data to plots and adjust vertical scale
    ui->plotTemp->graph(0)->addData(ticksCounter,device.gpuTemeperatureData.current);
    if (device.gpuTemeperatureData.max >= ui->plotTemp->yAxis->range().upper || device.gpuTemeperatureData.min <= ui->plotTemp->yAxis->range().lower)
        ui->plotTemp->yAxis->setRange(device.gpuTemeperatureData.min - 5, device.gpuTemeperatureData.max + 5);

    ui->plotClocks->graph(0)->addData(ticksCounter,device.gpuClocksData.coreClk);
    ui->plotClocks->graph(1)->addData(ticksCounter,device.gpuClocksData.memClk);
    ui->plotClocks->graph(2)->addData(ticksCounter,device.gpuClocksData.uvdCClk);
    ui->plotClocks->graph(3)->addData(ticksCounter,device.gpuClocksData.uvdDClk);
    int r = (device.gpuClocksData.memClk >= device.gpuClocksData.coreClk) ? device.gpuClocksData.memClk : device.gpuClocksData.coreClk;
    if (r > ui->plotClocks->yAxis->range().upper)
        ui->plotClocks->yAxis->setRangeUpper(r + 150);

    ui->plotVolts->graph(0)->addData(ticksCounter,device.gpuClocksData.coreVolt);
    ui->plotVolts->graph(1)->addData(ticksCounter,device.gpuClocksData.memVolt);
    if (device.gpuClocksData.coreVolt > ui->plotVolts->yAxis->range().upper)
        ui->plotVolts->yAxis->setRangeUpper(device.gpuClocksData.coreVolt + 100);

    if(ui->mainTabs->currentWidget() == ui->graphTab && !this->isHidden()) // If the graph tab is selected replot the graphs
        replotGraphs();
}

void radeon_profile::replotGraphs(){
    qDebug() << "Replotting graphs";
    const int position = ticksCounter + globalStuff::globalConfig.graphOffset;
    if(ui->cb_showTempsGraph->isChecked()){
        ui->plotTemp->xAxis->setRange(position, rangeX,Qt::AlignRight);
        ui->plotTemp->replot();
    }

    if(ui->cb_showFreqGraph->isChecked()){
        ui->plotClocks->xAxis->setRange(position,rangeX,Qt::AlignRight);
        ui->plotClocks->replot();
    }

    if(ui->cb_showVoltsGraph->isChecked()){
        ui->plotVolts->xAxis->setRange(position,rangeX,Qt::AlignRight);
        ui->plotVolts->replot();
    }

    ui->l_minMaxTemp->setText("Max: " + device.gpuTemeperatureDataString.max  + " | Min: " +
                              device.gpuTemeperatureDataString.min + " | Avg: " + QString().setNum(device.gpuTemeperatureData.sum/ticksCounter,'f',1) + QString::fromUtf8("\u00B0C"));
}

void radeon_profile::doTheStats() {
    // count ticks for stats //
    statsTickCounter++;

    if(device.gpuClocksData.powerLevelOk){
        if (pmStats.contains(device.gpuClocksDataString.powerLevel)) // This power level already exists, increment its count
            pmStats[device.gpuClocksDataString.powerLevel]++;
        else { // This power level does not exist, create it
            pmStats.insert(device.gpuClocksDataString.powerLevel, 1);

            QString coreDetails, memDetails;

            if(device.gpuClocksData.coreClkOk)
                coreDetails = device.gpuClocksDataString.coreClk;

            if(device.gpuClocksData.coreVoltOk)
                coreDetails += "  " + device.gpuClocksDataString.coreVolt;

            if(device.gpuClocksData.memClkOk)
                memDetails = device.gpuClocksDataString.memClk;

            if(device.gpuClocksData.memVoltOk)
                memDetails += "  " + device.gpuClocksDataString.memVolt;

            ui->list_stats->addTopLevelItem(new QTreeWidgetItem(QStringList() << device.gpuClocksDataString.powerLevel << coreDetails << memDetails));
        }
    }
}

void radeon_profile::updateStatsTable() {
    qDebug() << "Updating stats table";
    // do the math with percents
    for(QTreeWidgetItemIterator item(ui->list_stats); *item; item++) // For each item in list_stats
        (*item)->setText(3,QString::number(pmStats.value((*item)->text(0))*100.0/statsTickCounter)+"%");
}

void radeon_profile::refreshTooltip()
{
    QString tooltipdata = radeon_profile::windowTitle() + "\nCurrent profile: "+ device.currentPowerProfile + "  " + device.currentPowerLevel +"\n";
    for (short i = 0; i < ui->list_currentGPUData->topLevelItemCount(); i++) {
        tooltipdata += ui->list_currentGPUData->topLevelItem(i)->text(0) + ": " + ui->list_currentGPUData->topLevelItem(i)->text(1) + '\n';
    }
    tooltipdata.remove(tooltipdata.length() - 1, 1); //remove empty line at bootom
    trayIcon.setToolTip(tooltipdata);
}

void radeon_profile::configureDaemonAutoRefresh (bool enabled, int interval) {
    globalStuff::globalConfig.daemonAutoRefresh = enabled;

    if(enabled)
        globalStuff::globalConfig.interval = interval;

    device.reconfigureDaemon();
}

bool radeon_profile::fanStepIsValid(const int temperature, const int fanSpeed){
    return temperature >= minFanStepsTemp &&
            temperature <= maxFanStepsTemp &&
            fanSpeed >= minFanStepsSpeed &&
            fanSpeed <= maxFanStepsSpeed;
}

bool radeon_profile::addFanStep(const int temperature,
                                const int fanSpeed,
                                const bool alsoToList,
                                const bool alsoToGraph,
                                const bool alsoAdjustSpeed){

    if (!fanStepIsValid(temperature, fanSpeed)) {
        qWarning() << "Invalid value, can't be inserted into the fan step list:" << temperature << fanSpeed;
        return false;
    }

    qDebug() << "Adding step to fanSteps" << temperature << fanSpeed;
    fanSteps.insert(temperature,fanSpeed);

    if (alsoToList){
        const QString temperatureString = QString::number(temperature),
                speedString = QString::number(fanSpeed);
        const QList<QTreeWidgetItem*> existing = ui->list_fanSteps->findItems(temperatureString,Qt::MatchExactly);

        if(existing.isEmpty()){ // The element does not exist
            ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << temperatureString << speedString));
            ui->list_fanSteps->sortItems(0, Qt::AscendingOrder);
        } else // The element exists already, overwrite it
            existing.first()->setText(1,speedString);
    }

    if (alsoToGraph){
        ui->plotFanProfile->graph(0)->removeData(temperature);
        ui->plotFanProfile->graph(0)->addData(temperature, fanSpeed);
        ui->plotFanProfile->replot();
    }

    if (alsoAdjustSpeed){
        adjustFanSpeed(true);
    }

    return true;
}

bool radeon_profile::askConfirmation(const QString title, const QString question){
        return QMessageBox::question(this,
                                     title,
                                     question,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::Yes) == QMessageBox::Yes;
    }
