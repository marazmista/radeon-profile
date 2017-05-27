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

unsigned int radeon_profile::minFanStepsSpeed = 10;

radeon_profile::radeon_profile(QStringList a,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    rangeX = 180;
    ticksCounter = 0;
    statsTickCounter = 0;
	savedState = nullptr;

    ui->setupUi(this);
    timer = new QTimer(this);

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
    if (params.contains("--driver xorg"))
        ui->combo_gpus->addItems(device.initialize(gpu::XORG));
    else if (params.contains("--driver fglrx"))
        ui->combo_gpus->addItems(device.initialize(gpu::FGLRX));
    else
        ui->combo_gpus->addItems(device.initialize());

    ui->configGroups->setTabEnabled(2,device.daemonConnected());
    setupUiEnabledFeatures(device.features);

    if(ui->cb_enableOverclock->isChecked() && ui->cb_overclockAtLaunch->isChecked())
        ui->btn_applyOverclock->click();

    // timer init
    timer->setInterval(ui->spin_timerInterval->value()*1000);

    // fill tables with data at the start //
    refreshGpuData();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    fillConnectors();
    fillModInfo();
    refreshUI();

    timer->start();
    timerEvent();

    addRuntimeWidgets();
    connectSignals();

    showWindow();
    qDebug() << "Initialization completed";
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
void radeon_profile::setupUiEnabledFeatures(const globalStuff::driverFeatures &features) {
    if (features.canChangeProfile && features.pm < globalStuff::PM_UNKNOWN) {
        ui->tabs_pm->setTabEnabled(0,features.pm == globalStuff::PROFILE);

        ui->tabs_pm->setTabEnabled(1,features.pm == globalStuff::DPM);
        changeProfile->setEnabled(features.pm == globalStuff::PROFILE);
        dpmMenu->setEnabled(features.pm == globalStuff::DPM);
        ui->combo_pLevel->setEnabled(features.pm == globalStuff::DPM);
    } else {
        ui->tabs_pm->setEnabled(false);
        changeProfile->setEnabled(false);
        dpmMenu->setEnabled(false);
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->setEnabled(false);
        ui->cb_eventsTracking->setEnabled(false);
        ui->cb_eventsTracking->setChecked(false);
    }

    ui->cb_showFreqGraph->setEnabled(features.coreClockAvailable);
    ui->tabs_systemInfo->setTabEnabled(3,features.coreClockAvailable);
    ui->cb_showVoltsGraph->setEnabled(features.coreVoltAvailable);

    if (!features.temperatureAvailable) {
        ui->cb_showTempsGraph->setEnabled(false);
        ui->plotTemp->setVisible(false);
        ui->l_temp->setVisible(false);
    }

    if (!features.coreClockAvailable && !features.temperatureAvailable && !features.coreVoltAvailable)
        ui->mainTabs->setTabEnabled(1,false);

    if (!features.coreClockAvailable && !features.memClockAvailable) {
        ui->l_cClk->setVisible(false);
        ui->l_mClk->setVisible(false);
    }

    if (!features.coreVoltAvailable && !features.memVoltAvailable) {
        ui->l_cVolt->setVisible(false);
        ui->l_mVolt->setVisible(false);
    }

    if (features.pwmAvailable && features.canChangeProfile) {
        qDebug() << "Fan control is available , configuring the fan control tab";
        on_fanSpeedSlider_valueChanged(ui->fanSpeedSlider->value());
        ui->l_fanProfileUnsavedIndicator->setVisible(false);

        setupFanProfilesMenu();

        if (ui->cb_saveFanMode->isChecked()) {
            switch (ui->fanModesTabs->currentIndex()) {
                case 1:
                    ui->btn_pwmFixed->click();
                    break;
                case 2:
                    device.getTemperature();
                    ui->btn_pwmProfile->click();
                    break;
            }
        }
    } else {
        qDebug() << "Fan control is not available";
        ui->mainTabs->setTabEnabled(3,false);
        ui->l_fanSpeed->setVisible(false);
    }

    if (Q_LIKELY(features.pm == globalStuff::DPM)) {
        ui->combo_pProfile->addItems(globalStuff::createDPMCombo());
        ui->combo_pLevel->addItems(globalStuff::createPowerLevelCombo());
    } else {
        ui->combo_pLevel->setEnabled(false);
        ui->combo_pProfile->addItems(globalStuff::createProfileCombo());
    }

     ui->mainTabs->setTabEnabled(2,features.overClockAvailable);
}

void radeon_profile::refreshGpuData() {
    device.refreshPowerLevel();
    device.getClocks();
    device.getTemperature();

    if (device.features.pwmAvailable)
        device.getPwmSpeed();

    if (Q_LIKELY(execsRunning.count() == 0))
        return;

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
    if (ui->mainTabs->currentIndex() == 0) {
        ui->list_currentGPUData->clear();

        if (device.gpuClocksData.powerLevel != -1)
            addChild(ui->list_currentGPUData, tr("Power level"), device.gpuClocksDataString.powerLevel);
        if (device.gpuClocksData.coreClk != -1)
            addChild(ui->list_currentGPUData, tr("GPU clock"), device.gpuClocksDataString.coreClk);
        if (device.gpuClocksData.memClk != -1)
            addChild(ui->list_currentGPUData, tr("Memory clock"), device.gpuClocksDataString.memClk);
        if (device.gpuClocksData.uvdCClk != -1)
            addChild(ui->list_currentGPUData, tr("UVD core clock (cclk)"), device.gpuClocksDataString.uvdCClk);
        if (device.gpuClocksData.uvdDClk != -1)
            addChild(ui->list_currentGPUData, tr("UVD decoder clock (dclk)"), device.gpuClocksDataString.uvdDClk);
        if (device.gpuClocksData.coreVolt != -1)
            addChild(ui->list_currentGPUData, tr("GPU voltage (vddc)"), device.gpuClocksDataString.coreVolt);
        if (device.gpuClocksData.memVolt != -1)
            addChild(ui->list_currentGPUData, tr("I/O voltage (vddci)"), device.gpuClocksDataString.memVolt);

        if (ui->list_currentGPUData->topLevelItemCount() == 0)
            addChild(ui->list_currentGPUData, tr("Can't read data"), tr("You need debugfs mounted and either root rights or the daemon running"));

        addChild(ui->list_currentGPUData, tr("GPU temperature"), device.gpuTemeperatureDataString.current);
    }
}

void radeon_profile::updateExecLogs() {
    for (int i = 0; i < execsRunning.count(); i++) {
        if (execsRunning.at(i)->getExecState() == QProcess::Running && execsRunning.at(i)->logEnabled) {
            QString logData = QDateTime::currentDateTime().toString(logDateFormat) +";" + device.gpuClocksDataString.powerLevel + ";" +
                    device.gpuClocksDataString.coreClk + ";"+
                    device.gpuClocksDataString.memClk + ";"+
                    device.gpuClocksDataString.uvdCClk + ";"+
                    device.gpuClocksDataString.uvdDClk + ";"+
                    device.gpuClocksDataString.coreVolt + ";"+
                    device.gpuClocksDataString.memVolt + ";"+
                    device.gpuTemeperatureDataString.current;
            execsRunning.at(i)->appendToLog(logData);
        }
    }
}
//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent() {
    if (!refreshWhenHidden->isChecked() && this->isHidden()) {

        // even if in tray, keep the fan control active (if enabled)
        if (device.features.pwmAvailable && ui->btn_pwmProfile->isChecked()) {
            device.getTemperature();
            adjustFanSpeed();
        }
        return;
    }

    if (Q_LIKELY(ui->cb_gpuData->isChecked())) {
        refreshGpuData();

        ui->combo_pProfile->setCurrentIndex(ui->combo_pProfile->findText(device.currentPowerProfile));
        if (device.features.pm == globalStuff::DPM)
            ui->combo_pLevel->setCurrentIndex(ui->combo_pLevel->findText(device.currentPowerLevel));

        if (device.features.pwmAvailable && ui->btn_pwmProfile->isChecked())
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

    if (Q_LIKELY(ui->cb_graphs->isChecked()))
        refreshGraphs();

    if (ui->cb_glxInfo->isChecked()) {
        ui->list_glxinfo->clear();
        ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    }

    if (ui->cb_connectors->isChecked())
        fillConnectors();

    if (ui->cb_modParams->isChecked())
        fillModInfo();

    refreshTooltip();

    if (ui->cb_eventsTracking->isChecked())
        checkEvents();
}

/**
 * If the fan profile contains a direct match for the current temperature, it is used.<br>
 * Otherwise find the closest matches and use linear interpolation:<br>
 * \f$speed(temp) = \frac{hSpeed - lSpeed}{hTemperature - lTemperature} * (temp - lTemperature)  + lSpeed\f$<br>
 * Where:<br>
 * \f$(hTemperature, hSpeed)\f$ is the higher closest match;<br>
 * \f$(lTemperature, lSpeed)\f$ is the lower closest match;<br>
 * \f$temp\f$ is the current temperature;<br>
 * \f$speed\f$ is the speed to apply.
 */
void radeon_profile::adjustFanSpeed() {
    if (device.gpuTemeperatureData.current != device.gpuTemeperatureData.currentBefore) {
        if (currentFanProfile.contains(device.gpuTemeperatureData.current)) {  // Exact match
            device.setPwmValue(currentFanProfile.value(device.gpuTemeperatureData.current));
            return;
        }

        // find bounds of current temperature
        QMap<int,unsigned int>::const_iterator high = currentFanProfile.upperBound(device.gpuTemeperatureData.current);
        QMap<int,unsigned int>::const_iterator low = (currentFanProfile.size() > 1 ? high - 1 : high);

        int hSpeed = high.value(),
			lSpeed = low.value();

        if (high == currentFanProfile.constBegin()) {
            device.setPwmValue(hSpeed);
            return;
        }

        if (low == currentFanProfile.constEnd()) {
            device.setPwmValue(lSpeed);
            return;
        }

        // calculate two point stright line equation based on boundaries of current temperature
        // y = mx + b = (y2-y1)/(x2-x1)*(x-x1)+y1
        int hTemperature = high.key(),
                lTemperature = low.key();

        float speed = (float)(hSpeed - lSpeed) / (float)(hTemperature - lTemperature)  * (device.gpuTemeperatureData.current - lTemperature)  + lSpeed;

        device.setPwmValue((speed < minFanStepsSpeed) ? minFanStepsSpeed : speed);
    }
}


void radeon_profile::refreshGraphs() {
    // count the tick and move graph to right
    ticksCounter++;
    ui->plotTemp->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset, rangeX,Qt::AlignRight);
    ui->plotTemp->replot();

    ui->plotClocks->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset,rangeX,Qt::AlignRight);
    ui->plotClocks->replot();

    ui->plotVolts->xAxis->setRange(ticksCounter + globalStuff::globalConfig.graphOffset,rangeX,Qt::AlignRight);
    ui->plotVolts->replot();

    // choose bigger clock and adjust plot scale
    int r = (device.gpuClocksData.memClk >= device.gpuClocksData.coreClk) ? device.gpuClocksData.memClk : device.gpuClocksData.coreClk;
    if (r > ui->plotClocks->yAxis->range().upper)
        ui->plotClocks->yAxis->setRangeUpper(r + 150);

    // add data to plots
    ui->plotClocks->graph(0)->addData(ticksCounter,device.gpuClocksData.coreClk);
    ui->plotClocks->graph(1)->addData(ticksCounter,device.gpuClocksData.memClk);
    ui->plotClocks->graph(2)->addData(ticksCounter,device.gpuClocksData.uvdCClk);
    ui->plotClocks->graph(3)->addData(ticksCounter,device.gpuClocksData.uvdDClk);

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
    volt = (device.gpuClocksData.coreVolt == -1) ? "" : "(" + device.gpuClocksDataString.coreVolt+")";
    pmLevelName = (device.gpuClocksData.coreClk == -1) ? pmLevelName : pmLevelName + " Core:" +device.gpuClocksDataString.coreClk + volt;

    volt = (device.gpuClocksData.memVolt == -1) ? "" : "(" + device.gpuClocksDataString.memVolt + ")";
    pmLevelName = (device.gpuClocksData.memClk == -1) ? pmLevelName : pmLevelName + " Mem:" + device.gpuClocksDataString.memClk +  volt;

    if (pmStats.contains(pmLevelName)) // This power level already exists, increment its count
        pmStats[pmLevelName]++;
    else { // This power level does not exist, create it
        pmStats.insert(pmLevelName, 1);
        ui->list_stats->addTopLevelItem(new QTreeWidgetItem(QStringList() << pmLevelName));
        ui->list_stats->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

void radeon_profile::updateStatsTable() {
    // do the math with percents
    for(QTreeWidgetItemIterator item(ui->list_stats); *item; item++) // For each item in list_stats
        (*item)->setText(1,QString::number(pmStats.value((*item)->text(0))*100.0/statsTickCounter)+"%");
}

void radeon_profile::refreshTooltip()
{
    QString tooltipdata = radeon_profile::windowTitle() + "\n" + tr("Current profile: ")+ device.currentPowerProfile + "  " + device.currentPowerLevel +"\n";

    for (short i = 0; i < ui->list_currentGPUData->topLevelItemCount(); i++)
        tooltipdata += ui->list_currentGPUData->topLevelItem(i)->text(0) + ": " + ui->list_currentGPUData->topLevelItem(i)->text(1) + '\n';

    tooltipdata.remove(tooltipdata.length() - 1, 1); //remove empty line at bootom
    trayIcon->setToolTip(tooltipdata);
}

void radeon_profile::configureDaemonAutoRefresh (bool enabled, int interval) {
    globalStuff::globalConfig.daemonAutoRefresh = enabled;

    if(enabled)
        globalStuff::globalConfig.interval = interval;

    device.reconfigureDaemon();
}

bool radeon_profile::askConfirmation(const QString title, const QString question){
    return QMessageBox::Yes ==
            QMessageBox::question(this, title, question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
}

void radeon_profile::showWindow() {
    if (ui->cb_minimizeTray->isChecked() && ui->cb_startMinimized->isChecked())
        return;

    if (ui->cb_startMinimized->isChecked())
        this->showMinimized();
    else
        this->showNormal();
}
