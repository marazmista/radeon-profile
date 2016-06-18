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

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "dfglrx.h"
#include "dxorg.h"
#include "dintel.h"

#include <QTimer>
#include <QTextStream>
#include <QMenu>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QTime>

#define logTime qDebug() << QTime::currentTime().toString("ss.zzz")


/**
 * @brief radeon_profile::radeon_profile    Starting point of Radeon Profile.
 * First off the correct driver class is loaded (either through arguments or through driver detection)
 * Then  the UI and its elements get initialized.
 * Then the options are loaded and the driver class is initialized.
 * Finally UI elements that depend on driver features are filled [or disabled]
 * @param a arguments
 * @param parent parent widget
 */
radeon_profile::radeon_profile(QStringList a,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    rangeX = 180;
    ticksCounter = statsTickCounter = graphOffset = 0;
    closeFromTrayMenu = false;

    // checks if running as root
    globalStuff::globalConfig.rootMode = (globalStuff::grabSystemInfo("whoami")[0] == "root");

    logTime << "Figuring out the driver";
    QString params = a.join(" ");
    if (params.contains("--driver xorg"))
        device = new dXorg();
    else if (params.contains("--driver fglrx"))
        device = new dFglrx();
    else if(params.contains("--driver intel"))
        device = new dIntel();
    else
        device = detectDriver();

    logTime << "Setting up UI";
    ui->setupUi(this);


    logTime << "Setting up ui elements";
    ui->label_rootWarrning->setVisible(globalStuff::globalConfig.rootMode);
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
    setupExportDialog();

    logTime << "Loading config";
    loadConfig();


    logTime << "Setting up UI elements for available features";
    if(device->gpuList.isEmpty())
        on_combo_gpus_currentIndexChanged(-1);
    else
        ui->combo_gpus->addItems(device->gpuList);
    // The current index of combo_gpus changes (device->changeGpu() and setupUiEnabledFeatures() are called, list_glxinfo is filled)

    ui->tab_daemonConfig->setEnabled(device->daemonConnected());

    if((device->features.GPUoverClockAvailable || device->features.memoryOverclockAvailable)
            && ui->cb_enableOverclock->isChecked() && ui->cb_overclockAtLaunch->isChecked())
        ui->btn_applyOverclock->click();


    logTime << "Filling tables with data at the start";
    refreshGpuData();
    ui->list_connectors->addTopLevelItems(device->getCardConnectors());
    ui->list_connectors->expandToDepth(2);
    ui->list_modInfo->addTopLevelItems(device->getModuleInfo());
    if(ui->cb_gpuData->isChecked())
        refreshUI();

    logTime << "Starting timer and adding runtime widgets";
    timerID = startTimer(ui->spin_timerInterval->value()*1000);
    addRuntimeWidgets();

    showWindow();
    logTime << "Initialization completed";
}

radeon_profile::~radeon_profile()
{
    delete ui;
    delete device;
}

gpu * radeon_profile::detectDriver(){
    const QStringList lsmod = globalStuff::grabSystemInfo("lsmod");

    if (!lsmod.filter("radeon").isEmpty())
        return new dXorg("radeon");

    if (!lsmod.filter("amdgpu").isEmpty())
        return new dXorg("amdgpu");

    if (!lsmod.filter("fglrx").isEmpty())
        return new dFglrx();

    if(!lsmod.filter("i915").isEmpty())
        return new dIntel();

    return new gpu();
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
    qDebug() << "Power profile enabled: " << features.canChangeProfile;
    qDebug() << "Core {clock, volt} enabled: " << features.coreClockAvailable << features.coreVoltAvailable;
    qDebug() << "Memory {core, volt} enabled: " << features.memClockAvailable << features.memClockAvailable;
    qDebug() << "Temperature enabled: " << features.temperatureAvailable;
    qDebug() << "Fan speed enabled: " << features.pwmAvailable;
    qDebug() << "Overclock {GPU, memory} enabled: " << features.GPUoverClockAvailable << features.memoryOverclockAvailable;

    const bool gpuDataAvailable = ui->cb_gpuData->isChecked(),
            profilesAvailable = features.canChangeProfile && (features.pm < PM_UNKNOWN),
            dpm = profilesAvailable && (features.pm == DPM),
            standardProfiles = profilesAvailable && (features.pm == PROFILE),
            temperatureAvailable = gpuDataAvailable && features.temperatureAvailable,
            clocksAvailable = gpuDataAvailable && (features.coreClockAvailable || features.memClockAvailable),
            voltsAvailable = gpuDataAvailable && (features.coreVoltAvailable || features.memVoltAvailable),
            graphsEnabled = gpuDataAvailable && ui->cb_graphs->isChecked(),
            graphsAvailable = graphsEnabled && (clocksAvailable || temperatureAvailable || voltsAvailable),
            rootAccessAvailable = globalStuff::globalConfig.rootMode || device->daemonConnected(),
            fanSpeedAvailable = gpuDataAvailable && features.pwmAvailable,
            fanControlAvailable = rootAccessAvailable && features.pwmAvailable,
            fanProfileAvailable = temperatureAvailable && fanControlAvailable,
            overclockAvailable = rootAccessAvailable && (features.GPUoverClockAvailable || features.memoryOverclockAvailable);

    // Power managment
    ui->tabs_pm->setEnabled(profilesAvailable);
    ui->combo_pProfile->setEnabled(profilesAvailable);

    ui->stdProfiles->setEnabled(standardProfiles);
    ui->label_noStdProfiles->setVisible( ! standardProfiles);

    if(trayIconAvailable){
        changeProfile->setEnabled(standardProfiles);
        dpmMenu.setEnabled(dpm);
    }

    ui->dpmProfiles->setEnabled(dpm);
    ui->combo_pLevel->setEnabled(dpm);

    if(profilesAvailable) {
        qDebug() << "Power profiles are available, configuring";
        ui->tabs_pm->setCurrentWidget(dpm ? ui->dpmProfiles : ui->stdProfiles);

        ui->combo_pProfile->clear();
        ui->combo_pLevel->clear();
        if (dpm) {
            qDebug() << "Setting up DPM power profiles";
            ui->combo_pProfile->addItem(dpm_battery); // See on_combo_pProfile_currentIndexChanged(), count=1
            ui->combo_pProfile->addItems({dpm_balanced, dpm_performance});
            ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device->currentPowerProfile));

            ui->combo_pLevel->addItem(dpm_auto); // See on_combo_pLevel_currentIndexChanged(), count=1
            ui->combo_pLevel->addItems({dpm_low, dpm_high});
            ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(device->currentPowerLevel));

        } else if(standardProfiles) {
            qDebug() << "Setting up standard profiles";
            ui->combo_pProfile->addItem(profile_auto); // See on_combo_pProfile_currentIndexChanged(), count=1
            ui->combo_pProfile->addItems({profile_default, profile_low, profile_mid, profile_high});
            ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device->currentPowerProfile));
        }
    }

    // GPU stats
    ui->list_currentGPUData->setEnabled(gpuDataAvailable);
    ui->tab_stats->setEnabled(gpuDataAvailable && features.coreClockAvailable && ui->cb_stats->isChecked());

    // Graphs
    ui->graphTab->setEnabled(graphsAvailable);
    if(graphsAvailable){
        ui->cb_showTempsGraph->setEnabled(temperatureAvailable);
        ui->cb_showFreqGraph->setEnabled(clocksAvailable);
        ui->cb_showVoltsGraph->setEnabled(voltsAvailable);

        exportUi.cb_temperature->setEnabled(temperatureAvailable);
        exportUi.cb_frequency->setEnabled(clocksAvailable);
        exportUi.cb_volts->setEnabled(voltsAvailable);
    }

    ui->plotTemp->setVisible(temperatureAvailable && ui->cb_showTempsGraph->isChecked());
    ui->plotClocks->setVisible(clocksAvailable && ui->cb_showFreqGraph->isChecked());
    ui->plotVolts->setVisible(voltsAvailable && ui->cb_showVoltsGraph->isChecked());

    // Labels
    ui->l_minMaxTemp->setEnabled(temperatureAvailable);
    ui->l_cClk->setEnabled(gpuDataAvailable && features.coreClockAvailable);
    ui->l_cVolt->setEnabled(gpuDataAvailable && features.coreVoltAvailable);
    ui->l_mClk->setEnabled(gpuDataAvailable && features.memClockAvailable);
    ui->l_mVolt->setEnabled(gpuDataAvailable && features.memVoltAvailable);
    ui->l_temp->setEnabled(temperatureAvailable);
    ui->l_fanSpeed->setEnabled(fanSpeedAvailable);

    // Fan control
    ui->fanTab->setEnabled(fanControlAvailable);

    ui->btn_pwmAuto->setEnabled(fanControlAvailable);
    ui->btn_pwmFixed->setEnabled(fanControlAvailable);
    ui->btn_pwmProfile->setEnabled(fanProfileAvailable);
    ui->page_profile->setEnabled(fanProfileAvailable);

    if(fanProfileAvailable && fanSteps.isEmpty())
        loadFanProfiles();

    if (fanControlAvailable) {
        qDebug() << "Fan control is available , configuring the fan control tab";
        ui->fanSpeedSlider->setMaximum(device->features.pwmMaxSpeed);
        on_fanSpeedSlider_valueChanged(ui->fanSpeedSlider->value());
    }

    // Overclock
    ui->cb_enableOverclock->setChecked(overclockAvailable);
    ui->overclockTab->setEnabled(overclockAvailable);
    if( ! overclockAvailable){
        qDebug() << "Overclock is not available";
        ui->label_overclock->setText(rootAccessAvailable ?
                                         tr("Your driver or your GPU does not support overclocking") :
                                         tr("You need debugfs mounted and either root rights or the daemon running"));

    } else {
        qDebug() << "Setting up overclock";
        const bool gpuOcOk = features.GPUoverClockAvailable && ui->cb_enableOverclock->isChecked(),
                memoryOcOk = features.memoryOverclockAvailable && ui->cb_enableOverclock->isChecked();

        ui->label_GPUoc->setEnabled(gpuOcOk);
        ui->slider_GPUoverclock->setEnabled(gpuOcOk);
        ui->label_GPUoverclock->setEnabled(gpuOcOk);

        ui->label_memoryOc->setEnabled(memoryOcOk);
        ui->slider_memoryOverclock->setEnabled(memoryOcOk);
        ui->label_memoryOverclock->setEnabled(memoryOcOk);
    }

    // Settings page
    ui->cb_graphs->setEnabled(gpuDataAvailable);
    ui->cb_stats->setEnabled(gpuDataAvailable);
}

void radeon_profile::refreshGpuData() {
    if(device->features.canChangeProfile)
        device->refreshPowerLevel();

    if(device->updateClocksDataIsAvailable())
        device->updateClocksData();

    if(device->features.temperatureAvailable)
        device->updateTemperatureData();

    updateExecLogs();
}

void radeon_profile::addChild(QTreeWidget * parent, const QString &leftColumn, const QString  &rightColumn) {
    parent->addTopLevelItem(new QTreeWidgetItem(QStringList() << leftColumn << rightColumn));
}

// -1 value means that we not show in table. it's default (in gpuClocksStruct constructor), and if we
// did not alter it, it stays and in result will be not displayed
void radeon_profile::refreshUI() {
    ui->l_cClk->setText(device->gpuClocksDataString.coreClk);
    ui->l_mClk->setText(device->gpuClocksDataString.memClk);
    ui->l_mVolt->setText(device->gpuClocksDataString.memVolt);
    ui->l_cVolt->setText(device->gpuClocksDataString.coreVolt);

    // Header - Temperature
    ui->l_temp->setText(device->gpuTemeperatureDataString.current);

    // Header - Fan speed
    ui->l_fanSpeed->setText(device->gpuTemeperatureDataString.pwmSpeed);

    // GPU data list
    if (ui->mainTabs->currentWidget() == ui->infoTab || this->isHidden()) {
        // Update currentGPUData if is visible or the window is hidden (since the tray tooltip copies data from currentGPUData)
        qDebug() << "Updating GPU data list";
        ui->list_currentGPUData->clear();

        if (device->gpuClocksData.powerLevelOk)
            addChild(ui->list_currentGPUData, tr("Power level"), device->gpuClocksDataString.powerLevel);

        if(device->features.coreMaxClkAvailable)
            addChild(ui->list_currentGPUData, tr("Maximum GPU clock"), device->maxCoreFreqString);
        if (device->gpuClocksData.coreClkOk)
            addChild(ui->list_currentGPUData, tr("GPU clock"), device->gpuClocksDataString.coreClk);
        if(device->features.coreMinClkAvailable)
            addChild(ui->list_currentGPUData, tr("Minimum GPU clock"), device->minCoreFreqString);

        if (device->gpuClocksData.memClkOk)
            addChild(ui->list_currentGPUData, tr("Memory clock"), device->gpuClocksDataString.memClk);
        if (device->gpuClocksData.uvdCClkOk)
            addChild(ui->list_currentGPUData, tr("UVD core clock"), device->gpuClocksDataString.uvdCClk);
        if (device->gpuClocksData.uvdDClkOk)
            addChild(ui->list_currentGPUData, tr("UVD decoder clock"), device->gpuClocksDataString.uvdDClk);
        if (device->gpuClocksData.coreVoltOk)
            addChild(ui->list_currentGPUData, tr("GPU voltage"), device->gpuClocksDataString.coreVolt);
        if (device->gpuClocksData.memVoltOk)
            addChild(ui->list_currentGPUData, tr("I/O voltage"), device->gpuClocksDataString.memVolt);

        if (ui->list_currentGPUData->topLevelItemCount() == 0)
            addChild(ui->list_currentGPUData, tr("Can't read data"), tr("You need debugfs mounted and either root rights or the daemon running"));

        if(device->features.temperatureAvailable)
            addChild(ui->list_currentGPUData, tr("GPU temperature"), device->gpuTemeperatureDataString.current);
    }
}

void radeon_profile::updateExecLogs() {
    for (execBin *exe : execsRunning) {
        if (exe->state() == QProcess::Running && exe->logEnabled) {
            QString logData = QDateTime::currentDateTime().toString(logDateFormat) +";" + device->gpuClocksDataString.powerLevel + ";" +
                    device->gpuClocksDataString.coreClk + ";"+
                    device->gpuClocksDataString.memClk + ";"+
                    device->gpuClocksDataString.uvdCClk + ";"+
                    device->gpuClocksDataString.uvdDClk + ";"+
                    device->gpuClocksDataString.coreVolt + ";"+
                    device->gpuClocksDataString.memVolt + ";"+
                    device->gpuTemeperatureDataString.current;
            exe->appendToLog(logData);
        }
    }
}

//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent(QTimerEvent * event) {
    if(event)
        event->accept();

    if (trayIconAvailable && !refreshWhenHidden->isChecked() && this->isHidden()) {
        // even if in tray, keep the fan control active (if enabled)
        device->updateTemperatureData();
        adjustFanSpeed();
        return;
    }

    if (ui->cb_gpuData->isChecked()) {

        refreshGpuData();

        if(ui->cb_showCombo->isChecked() && device->features.canChangeProfile){ // If pLevel and pProfile are visible update their index
            ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device->currentPowerProfile));
            if (device->features.pm == DPM)
                ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(device->currentPowerLevel));
        }

        adjustFanSpeed();

        // lets say coreClk is essential to get stats (it is disabled in ui anyway when features.clocksAvailable is false)
        if (ui->cb_stats->isChecked() && device->gpuClocksData.coreClkOk) {
            doTheStats();

            // do the math only when user looking at stats table
            if (ui->tabs_systemInfo->currentWidget() == ui->tab_stats && ui->mainTabs->currentWidget() == ui->infoTab && !this->isHidden())
                updateStatsTable();
        }

        refreshUI();

        if(trayIconAvailable && this->isHidden())
            refreshTooltip();

        if (ui->graphTab->isEnabled())
            refreshGraphs();
    }

    if (ui->cb_glxInfo->isChecked()) {
        ui->list_glxinfo->clear();
        ui->list_glxinfo->addItems(device->getGLXInfo(ui->combo_gpus->currentText()));
    }
    if (ui->cb_connectors->isChecked()) {
        ui->list_connectors->clear();
        ui->list_connectors->addTopLevelItems(device->getCardConnectors());
        ui->list_connectors->expandToDepth(2);
    }
    if (ui->cb_modParams->isChecked()) {
        ui->list_modInfo->clear();
        ui->list_modInfo->addTopLevelItems(device->getModuleInfo());
    }

}

void radeon_profile::adjustFanSpeed(const bool force)
{
    // If 'force' is true skip all checks.
    if (force ||
            // Otherwise check if PWM is available and enabled by the user and if the temperature has changed
            (device->features.pwmAvailable && ui->btn_pwmProfile->isChecked() &&
            std::abs(device->gpuTemeperatureData.current - device->gpuTemeperatureData.currentBefore) > 0.1f)) { // current != currentBefore

        const short temperature = static_cast<short>(device->gpuTemeperatureData.current);
        float speed;
        if (fanSteps.contains(temperature)) // Exact match
            speed = fanSteps.value(temperature);
        else {
            // find bounds of current temperature
            const QMap<short, unsigned short>::const_iterator high = fanSteps.upperBound(temperature),
                    low = high - 1;
            if(high == fanSteps.constBegin()) // Before the first step
                speed = high.value();
            else if(low == fanSteps.constEnd()) // After the last step
                speed = low.value();
            else  // In the middle
                // calculate two point stright line equation based on boundaries of current temperature
                // y = mx + b = (y2-y1)/(x2-x1)*(x-x1)+y1
                speed=static_cast<float>(high.value()-low.value())/(high.key()-low.key())*(temperature-low.key())+low.value();
        }

        qDebug() << device->gpuTemeperatureDataString.current << "-->" << speed << "%";
        if(speed >= 0 && speed <= 100)
            device->setPwmValue(static_cast<ushort>(device->features.pwmMaxSpeed * speed / 100));
    }
}

void radeon_profile::refreshGraphs() {
    // count the tick and move graph to right
    ticksCounter++;

    // Temperature grqaph
    ui->plotTemp->graph(0)->addData(ticksCounter, static_cast<double>(device->gpuTemeperatureData.current));

    // Frequency graph
    ui->plotClocks->graph(0)->addData(ticksCounter,device->gpuClocksData.coreClk);
    ui->plotClocks->graph(1)->addData(ticksCounter,device->gpuClocksData.memClk);
    ui->plotClocks->graph(2)->addData(ticksCounter,device->gpuClocksData.uvdCClk);
    ui->plotClocks->graph(3)->addData(ticksCounter,device->gpuClocksData.uvdDClk);
    const double freqUpperRange = ((device->gpuClocksData.memClk >= device->gpuClocksData.coreClk) ? device->gpuClocksData.memClk : device->gpuClocksData.coreClk) + 150;
    if (freqUpperRange > ui->plotClocks->yAxis->range().upper)
        ui->plotClocks->yAxis->setRangeUpper(freqUpperRange);

    // Volts graph
    ui->plotVolts->graph(0)->addData(ticksCounter,device->gpuClocksData.coreVolt);
    ui->plotVolts->graph(1)->addData(ticksCounter,device->gpuClocksData.memVolt);
    const double voltsUpperRange = device->gpuClocksData.coreVolt + 100;
    if (voltsUpperRange > ui->plotVolts->yAxis->range().upper)
        ui->plotVolts->yAxis->setRangeUpper(voltsUpperRange);

    if(ui->mainTabs->currentWidget() == ui->graphTab && !this->isHidden()) // If the graph tab is selected replot the graphs
        replotGraphs();
}

void radeon_profile::replotGraphs(){
    qDebug() << "Replotting graphs";
    const int position = ticksCounter + graphOffset;
    if(ui->cb_showTempsGraph->isChecked()){
        ui->plotTemp->xAxis->setRange(position, rangeX,Qt::AlignRight);

        const double upperRange = device->gpuTemeperatureData.max + 5, lowerRange = device->gpuTemeperatureData.min - 5;
        if ((upperRange > ui->plotTemp->yAxis->range().upper) || (lowerRange < ui->plotTemp->yAxis->range().lower))
            ui->plotTemp->yAxis->setRange(lowerRange, upperRange);

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

    ui->l_minMaxTemp->setText("Max: " + device->gpuTemeperatureDataString.max  + " | Min: " +
                              device->gpuTemeperatureDataString.min + " | Avg: " + QString().setNum(device->gpuTemeperatureData.sum/ticksCounter,'f',1) + QString::fromUtf8("\u00B0C"));
}

QString radeon_profile::getCoreDetails() const {
    QString coreDetails;

    // The string MUST start with gpuClocksDataString.coreClk, if available
    // see doTheStats()
    if(device->gpuClocksData.coreClkOk)
        coreDetails = device->gpuClocksDataString.coreClk;

    if(device->gpuClocksData.coreVoltOk)
        coreDetails += "  " + device->gpuClocksDataString.coreVolt;

    return coreDetails;
}

QString radeon_profile::getMemDetails() const {
    QString memDetails;

    if(device->gpuClocksData.memClkOk)
        memDetails = device->gpuClocksDataString.memClk;

    if(device->gpuClocksData.memVoltOk)
        memDetails += "  " + device->gpuClocksDataString.memVolt;

    return memDetails;
}

QString radeon_profile::getNextPmStatsKey() const {
    for(ushort i = 0; i < USHRT_MAX; i++){
        QString str = QString::number(i);
        if( ! pmStats.contains(str))
            return str;
    }

    return QString();
}

void radeon_profile::doTheStats() {
    // count ticks for stats //
    statsTickCounter++;

    if(device->gpuClocksData.powerLevelOk){
        // Use the power level as key
        if (pmStats.contains(device->gpuClocksDataString.powerLevel)) // This power level already exists, increment its count
            pmStats[device->gpuClocksDataString.powerLevel]++;
        else { // This power level does not exist, create it
            pmStats.insert(device->gpuClocksDataString.powerLevel, 1);


            ui->list_stats->addTopLevelItem(new QTreeWidgetItem({device->gpuClocksDataString.powerLevel, getCoreDetails(), getMemDetails()}));
            ui->list_stats->sortItems(0, Qt::AscendingOrder);
        }
    } else if (device->gpuClocksData.coreClkOk) {
        // Power level is not available, use the core clock as fallback key
        QList<QTreeWidgetItem *> items = ui->list_stats->findItems(device->gpuClocksDataString.coreClk, Qt::MatchStartsWith, 1);
        if(items.size() == 1){ // This level already exists, increment its count
            pmStats[items[0]->text(0)]++;
        } else {
            QString key = getNextPmStatsKey();

            pmStats.insert(key, 1);


            ui->list_stats->addTopLevelItem(new QTreeWidgetItem({key, getCoreDetails(), getMemDetails()}));
            ui->list_stats->sortItems(1, Qt::AscendingOrder);
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
    QString tooltipdata = radeon_profile::windowTitle() + "\n";

    if(device->features.canChangeProfile)
        tooltipdata += "Power profile: "+ device->currentPowerProfile + "  " + device->currentPowerLevel +"\n";

    for (ushort i = 0; i < ui->list_currentGPUData->topLevelItemCount(); i++) {
        tooltipdata += ui->list_currentGPUData->topLevelItem(i)->text(0) + ": " + ui->list_currentGPUData->topLevelItem(i)->text(1) + '\n';
    }
    tooltipdata.remove(tooltipdata.length() - 1, 1); //remove empty line at bootom
    trayIcon.setToolTip(tooltipdata);
}

void radeon_profile::configureDaemonAutoRefresh (bool enabled, int interval) {
    globalStuff::globalConfig.daemonAutoRefresh = enabled && (interval > 0);

    if(globalStuff::globalConfig.daemonAutoRefresh)
        globalStuff::globalConfig.interval = static_cast<ushort>(interval);

    device->reconfigureDaemon();
}

bool radeon_profile::fanStepIsValid(const int temperature, const int fanSpeed){
    return temperature >= minFanStepsTemp &&
            temperature <= maxFanStepsTemp &&
            fanSpeed >= minFanStepsSpeed &&
            fanSpeed <= maxFanStepsSpeed;
}

bool radeon_profile::addFanStep(const short temperature,
                                const ushort fanSpeed,
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
        qDebug() << "Adding step to list_fanSteps";
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
        qDebug() << "Adding step to plotFanProfile";
        ui->plotFanProfile->graph(0)->removeData(temperature);
        ui->plotFanProfile->graph(0)->addData(temperature, fanSpeed);
        ui->plotFanProfile->replot();
    }

    if (alsoAdjustSpeed){
        qDebug() << "Adjusting fan speed";
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
