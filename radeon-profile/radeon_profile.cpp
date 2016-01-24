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

const int appVersion = 20160124;

int ticksCounter = 0, statsTickCounter = 0;
double rangeX = 180;
QList<pmLevel> pmStats;


radeon_profile::radeon_profile(QStringList a,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);
    timer = new QTimer();
    execsRunning = new QList<execBin*>();

    // checks if running as root
    if (globalStuff::grabSystemInfo("whoami")[0] == "root") {
         globalStuff::globalConfig.rootMode = true;
        ui->label_rootWarrning->setVisible(true);
    } else {
        globalStuff::globalConfig.rootMode = false;
        ui->label_rootWarrning->setVisible(false);
    }

    // setup ui elemensts
    ui->mainTabs->setCurrentIndex(0);
    ui->tabs_systemInfo->setCurrentIndex(0);
    ui->configGroups->setCurrentIndex(0);
    ui->list_currentGPUData->setHeaderHidden(false);
    ui->execPages->setCurrentIndex(0);
    setupGraphs();
    setupForcePowerLevelMenu();
    setupOptionsMenu();
    setupContextMenus();
    setupTrayIcon();

    loadConfig();

    //figure out parameters
    QString params = a.join(" ");
    if (params.contains("--driver xorg")) {
        device.driverByParam(gpu::XORG);
        ui->combo_gpus->addItems(device.initialize(true));
    }
    else if (params.contains("--driver fglrx")) {
        device.driverByParam(gpu::FGLRX);
        ui->combo_gpus->addItems(device.initialize(true));
    }
    else // driver object detects cards in pc and fill the list in ui //
        ui->combo_gpus->addItems(device.initialize());

    ui->configGroups->setTabEnabled(2,device.daemonConnected());
    setupUiEnabledFeatures(device.features);

    connectSignals();

    // timer init
    timer->setInterval(ui->spin_timerInterval->value()*1000);

    // fill tables with data at the start //
    refreshGpuData();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    ui->list_connectors->addTopLevelItems(device.getCardConnectors());
    ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    refreshUI();

    timer->start();
    addRuntimeWidgets();
}

radeon_profile::~radeon_profile()
{
    delete ui;
}

void radeon_profile::connectSignals()
{
    // fix for warrning: QMetaObject::connectSlotsByName: No matching signal for...
    connect(ui->combo_gpus,SIGNAL(currentIndexChanged(QString)),this,SLOT(gpuChanged()));
    connect(ui->combo_pProfile,SIGNAL(currentIndexChanged(int)),this,SLOT(changeProfileFromCombo()));
    connect(ui->combo_pLevel,SIGNAL(currentIndexChanged(int)),this,SLOT(changePowerLevelFromCombo()));

    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
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
    btnBackProfiles->setText("Back to profiles");
    ui->tabs_execOutputs->setCornerWidget(btnBackProfiles);
    btnBackProfiles->show();
    connect(btnBackProfiles,SIGNAL(clicked()),this,SLOT(btnBackToProfilesClicked()));

    // set pwm buttons in group
    QButtonGroup *pwmGroup = new QButtonGroup();
    pwmGroup->addButton(ui->btn_pwmAuto);
    pwmGroup->addButton(ui->btn_pwmFixed);
    pwmGroup->addButton(ui->btn_pwmProfile);
}

// based on driverFeatures structure returned by gpu class, adjust ui elements
void radeon_profile::setupUiEnabledFeatures(const globalStuff::driverFeatures &features) {
    if (features.canChangeProfile && features.pm < globalStuff::PM_UNKNOWN) {
        ui->tabs_pm->setTabEnabled(0,features.pm == globalStuff::PROFILE ? true : false);

        ui->tabs_pm->setTabEnabled(1,features.pm == globalStuff::DPM ? true : false);
        changeProfile->setEnabled(features.pm == globalStuff::PROFILE ? true : false);
        dpmMenu->setEnabled(features.pm == globalStuff::DPM ? true : false);
        ui->combo_pLevel->setEnabled(features.pm == globalStuff::DPM ? true : false);
    } else {
        ui->tabs_pm->setEnabled(false);
        changeProfile->setEnabled(false);
        dpmMenu->setEnabled(false);
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->setEnabled(false);
    }

    ui->l_cClk->setVisible(features.coreClockAvailable);
    ui->cb_showFreqGraph->setEnabled(features.coreClockAvailable);
    ui->tabs_systemInfo->setTabEnabled(3,features.coreClockAvailable);
    ui->l_cClk->setVisible(features.coreClockAvailable);
    ui->label_14->setVisible(features.coreClockAvailable);

    ui->cb_showVoltsGraph->setEnabled(features.coreVoltAvailable);
    ui->l_cVolt->setVisible(features.coreVoltAvailable);
    ui->label_20->setVisible(features.coreVoltAvailable);

    ui->l_mClk->setVisible(features.memClockAvailable);
    ui->label_18->setVisible(features.memClockAvailable);

    ui->l_mVolt->setVisible(features.memVoltAvailable);
    ui->label_19->setVisible(features.memVoltAvailable);

    if (!features.temperatureAvailable) {
        ui->cb_showTempsGraph->setEnabled(false);
        ui->plotTemp->setVisible(false);
        ui->l_temp->setVisible(false);
        ui->label_21->setVisible(false);
    }

    if (!features.coreClockAvailable && !features.temperatureAvailable && !features.coreVoltAvailable)
        ui->mainTabs->setTabEnabled(1,false);

    if (!features.pwmAvailable) {
        actuallyFanControlNotAvaible:
            ui->mainTabs->setTabEnabled(2,false);
            ui->l_fanSpeed->setVisible(false);
            ui->label_24->setVisible(false);
    } else {
        if (!globalStuff::globalConfig.rootMode && (!device.daemonConnected() && !globalStuff::globalConfig.rootMode))
            goto actuallyFanControlNotAvaible;

        ui->fanSpeedSlider->setMaximum(device.features.pwmMaxSpeed);

        loadFanProfiles();
        on_fanSpeedSlider_valueChanged(ui->fanSpeedSlider->value());
    }

    if (features.pm == globalStuff::DPM) {
        ui->combo_pProfile->addItems(QStringList() << dpm_battery << dpm_balanced << dpm_performance);
        ui->combo_pLevel->addItems(QStringList() << dpm_auto << dpm_low << dpm_high);

        ui->combo_pProfile->setCurrentIndex(ui->combo_pLevel->findText(device.currentPowerLevel));
        ui->combo_pLevel->setCurrentIndex(ui->combo_pProfile->findText(device.currentPowerLevel));

    } else {
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->addItems(QStringList() << profile_auto << profile_default << profile_high << profile_mid << profile_low);
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

// -1 value means that we not show in table. it's default (in gpuClocksStruct constructor), and if we
// did not alter it, it stays and in result will be not displayed
void radeon_profile::refreshUI() {
     ui->l_temp->setText(QString().setNum(device.gpuTemeperatureData.current));
     ui->l_cClk->setText(device.gpuClocksDataString.coreClk);
     ui->l_mClk->setText(device.gpuClocksDataString.memClk);
     ui->l_mVolt->setText(device.gpuClocksDataString.memVolt);
     ui->l_cVolt->setText(device.gpuClocksDataString.coreVolt);

     if (device.features.pwmAvailable)
         ui->l_fanSpeed->setText(device.gpuTemeperatureDataString.pwmSpeed);


    if (ui->mainTabs->currentIndex() == 0) {
        ui->list_currentGPUData->clear();

        if (device.gpuClocksData.powerLevel != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_currentPowerLevel << device.gpuClocksDataString.powerLevel));
        if (device.gpuClocksData.coreClk != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_currentGPUClock << device.gpuClocksDataString.coreClk + " MHz"));
        if (device.gpuClocksData.memClk != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_currentMemClock << device.gpuClocksDataString.memClk + " MHz"));
        if (device.gpuClocksData.uvdCClk != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_uvdVideoCoreClock << device.gpuClocksDataString.uvdCClk + " MHz"));
        if (device.gpuClocksData.uvdDClk != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_uvdDecoderClock <<device.gpuClocksDataString.uvdDClk + " MHz"));
        if (device.gpuClocksData.coreVolt != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_vddc << device.gpuClocksDataString.coreVolt + " mV"));
        if (device.gpuClocksData.memVolt != -1)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_vddci << device.gpuClocksDataString.memVolt + " mV"));

        if (ui->list_currentGPUData->topLevelItemCount() == 0)
            ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_errorReadingData));

        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << label_currentGPUTemperature << device.gpuTemeperatureDataString.current));
    }
}

void radeon_profile::updateExecLogs() {
    for (int i = 0; i < execsRunning->count(); i++) {
        if (execsRunning->at(i)->getExecState() == QProcess::Running && execsRunning->at(i)->logEnabled) {
            QString logData = QDateTime::currentDateTime().toString(logDateFormat) +";" + device.gpuClocksDataString.powerLevel + ";" +
                    device.gpuClocksDataString.coreClk + ";"+
                    device.gpuClocksDataString.memClk + ";"+
                    device.gpuClocksDataString.uvdCClk + ";"+
                    device.gpuClocksDataString.uvdDClk + ";"+
                    device.gpuClocksDataString.coreVolt + ";"+
                    device.gpuClocksDataString.memVolt + ";"+
                    device.gpuTemeperatureDataString.current;
            execsRunning->at(i)->appendToLog(logData);
        }
    }
}
//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent() {
    if (!refreshWhenHidden->isChecked() && this->isHidden()) {
        // even if in tray, keep the fan control active (if enabled)
        device.getTemperature();
        adjustFanSpeed();
        return;
    }

    if (ui->cb_gpuData->isChecked()) {
        refreshGpuData();

        ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device.currentPowerProfile));
        if (device.features.pm == globalStuff::DPM)
            ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(device.currentPowerLevel));

        adjustFanSpeed();

        // lets say coreClk is essential to get stats (it is disabled in ui anyway when features.clocksAvailable is false)
        if (ui->cb_stats->isChecked() && device.gpuClocksData.coreClk != -1) {
            doTheStats();

            // do the math only when user looking at stats table
            if (ui->tabs_systemInfo->currentIndex() == 3 && ui->mainTabs->currentIndex() == 0)
                updateStatsTable();
        }
        refreshUI();
    }

    if (ui->cb_graphs->isChecked())
        refreshGraphs();

    if (ui->cb_glxInfo->isChecked()) {
        ui->list_glxinfo->clear();
        ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    }
    if (ui->cb_connectors->isChecked()) {
        ui->list_connectors->clear();
        ui->list_connectors->addTopLevelItems(device.getCardConnectors());
    }
    if (ui->cb_modParams->isChecked()) {
        ui->list_modInfo->clear();
        ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    }

    refreshTooltip();
}

void radeon_profile::adjustFanSpeed()
{
    if (device.features.pwmAvailable && ui->btn_pwmProfile->isChecked())
        if (device.gpuTemeperatureData.current != device.gpuTemeperatureData.currentBefore) {

            // find bounds of current temperature
            for (int i = 1; i < fanSteps.count(); i++)
                if (fanSteps.at(i-1).temperature <= device.gpuTemeperatureData.current && fanSteps.at(i).temperature > device.gpuTemeperatureData.current) {
                    fanStepPair h = fanSteps.at(i), l = fanSteps.at(i-1);

                    // calculate two point stright line equation based on boundaries of current temperature
                    // y = mx + b
                    float m = (float)(h.speed - l.speed) / (float)(h.temperature - l.temperature);
                    float b = (float)(h.speed - (m * h.temperature));

                    // apply current temperature to calculated equation to figure out fan speed on slope
                    int speed = device.gpuTemeperatureData.current * m + b;

                    device.setPwmValue(device.features.pwmMaxSpeed * ((float)speed / 100));
                    break;
                }
        }
}

void radeon_profile::refreshGraphs() {
    // count the tick and move graph to right
    ticksCounter++;
    ui->plotTemp->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset, rangeX,Qt::AlignRight);
    ui->plotTemp->replot();

    ui->plotColcks->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset,rangeX,Qt::AlignRight);
    ui->plotColcks->replot();

    ui->plotVolts->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset,rangeX,Qt::AlignRight);
    ui->plotVolts->replot();

    // choose bigger clock and adjust plot scale
    int r = (device.gpuClocksData.memClk >= device.gpuClocksData.coreClk) ? device.gpuClocksData.memClk : device.gpuClocksData.coreClk;
    if (r > ui->plotColcks->yAxis->range().upper)
        ui->plotColcks->yAxis->setRangeUpper(r + 150);

    // add data to plots
    ui->plotColcks->graph(0)->addData(ticksCounter,device.gpuClocksData.coreClk);
    ui->plotColcks->graph(1)->addData(ticksCounter,device.gpuClocksData.memClk);
    ui->plotColcks->graph(2)->addData(ticksCounter,device.gpuClocksData.uvdCClk);
    ui->plotColcks->graph(3)->addData(ticksCounter,device.gpuClocksData.uvdDClk);

    if (device.gpuClocksData.coreVolt > ui->plotVolts->yAxis->range().upper)
        ui->plotVolts->yAxis->setRangeUpper(device.gpuClocksData.coreVolt + 100);

    ui->plotVolts->graph(0)->addData(ticksCounter,device.gpuClocksData.coreVolt);
    ui->plotVolts->graph(1)->addData(ticksCounter,device.gpuClocksData.memVolt);

    // temperature graph
    if (device.gpuTemeperatureData.max >= ui->plotTemp->yAxis->range().upper || device.gpuTemeperatureData.min <= ui->plotTemp->yAxis->range().lower)
        ui->plotTemp->yAxis->setRange(device.gpuTemeperatureData.min - 5, device.gpuTemeperatureData.max + 5);

    ui->plotTemp->graph(0)->addData(ticksCounter,device.gpuTemeperatureData.current);
    ui->l_minMaxTemp->setText("Max: " + device.gpuTemeperatureDataString.max  + " | Min: " +
                              device.gpuTemeperatureDataString.min + " | Avg: " + QString().setNum(device.gpuTemeperatureData.sum/ticksCounter,'f',1) + QString::fromUtf8("\u00B0C"));
}

void radeon_profile::doTheStats() {
    // count ticks for stats //
    statsTickCounter++;

    // figure out pm level based on data provided
    QString pmLevelName = (device.gpuClocksData.powerLevel == -1) ? "" : "Power level:" + device.gpuClocksDataString.powerLevel, volt;
    volt = (device.gpuClocksData.coreVolt == -1) ? "" : "(" + device.gpuClocksDataString.coreVolt + "mV)";
    pmLevelName = (device.gpuClocksData.coreClk == -1) ? pmLevelName : pmLevelName + " Core:" +device.gpuClocksDataString.coreClk + "MHz" + volt;

    volt = (device.gpuClocksData.memVolt == -1) ? "" : "(" + device.gpuClocksDataString.memVolt + "mV)";
    pmLevelName = (device.gpuClocksData.memClk == -1) ? pmLevelName : pmLevelName + " Mem:" + device.gpuClocksDataString.memClk + "MHz" + volt;

    // find index of current pm level in stats list
    char index = -1;
    for (int i = 0; i < pmStats.count(); i++) {
        if (pmStats.at(i).name == pmLevelName) {
            index = i;
            break;
        }
    }

    // if found, count this tick to current pm level, if not add to list and count tick
    if (index != -1)
        pmStats[index].time++;
    else {
        pmLevel p;
        p.name = pmLevelName;
        p.time = 1;
        pmStats.append(p);

        QTreeWidgetItem *t = new QTreeWidgetItem();
        t->setText(0,p.name);
        ui->list_stats->addTopLevelItem(t);
    }
}

void radeon_profile::updateStatsTable() {
    // do the math with percents
    for (int i = 0;i < ui->list_stats->topLevelItemCount() ; i++) {
        ui->list_stats->topLevelItem(i)->setText(1,QString().setNum(pmStats.at(i).time/statsTickCounter * 100,'f',2)+"%");
    }
}

void radeon_profile::refreshTooltip()
{
    QString tooltipdata = radeon_profile::windowTitle() + "\nCurrent profile: "+ device.currentPowerProfile + "  " + device.currentPowerLevel +"\n";
    for (short i = 0; i < ui->list_currentGPUData->topLevelItemCount(); i++) {
        tooltipdata += ui->list_currentGPUData->topLevelItem(i)->text(0) + ": " + ui->list_currentGPUData->topLevelItem(i)->text(1) + '\n';
    }
    tooltipdata.remove(tooltipdata.length() - 1, 1); //remove empty line at bootom
    trayIcon->setToolTip(tooltipdata);
}

