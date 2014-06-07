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

const int appVersion = 20140607;

int ticksCounter = 0, statsTickCounter = 0;
double rangeX = 180;
QList<pmLevel> pmStats;

radeon_profile::radeon_profile(QStringList a,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);
    timer = new QTimer();

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

    setupUiEnabledFeatures(device.gpuFeatures);
    ui->configGroups->setTabEnabled(2,device.daemonConnected());

    // fix for warrning: QMetaObject::connectSlotsByName: No matching signal for...
    connect(ui->combo_gpus,SIGNAL(currentIndexChanged(QString)),this,SLOT(gpuChanged()));

    // timer init
    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    connect(ui->timeSlider,SIGNAL(valueChanged(int)),this,SLOT(changeTimeRange()));
    timer->setInterval(ui->spin_timerInterval->value()*1000);

    // fill tables with data at the start //
    refreshGpuData();
    ui->list_glxinfo->addItems(device.getGLXInfo(ui->combo_gpus->currentText()));
    ui->list_connectors->addTopLevelItems(device.getCardConnectors());
    ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    ui->l_profile->setText(device.getCurrentPowerProfile());

    timer->start();

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

    if (globalStuff::grabSystemInfo("whoami")[0] == "root")
        ui->label_rootWarrning->setVisible(true);
    else
        ui->label_rootWarrning->setVisible(false);
}

radeon_profile::~radeon_profile()
{
    delete execProcess;
    delete ui;
}

// based on driverFeatures structure returned by gpu class, adjust ui elements
void radeon_profile::setupUiEnabledFeatures(const globalStuff::driverFeatures &features) {
    if (features.canChangeProfile && features.pm < globalStuff::PM_UNKNOWN) {
        ui->tabs_pm->setTabEnabled(0,features.pm == globalStuff::PROFILE ? true : false);

        ui->tabs_pm->setTabEnabled(1,features.pm == globalStuff::DPM ? true : false);
        changeProfile->setEnabled(features.pm == globalStuff::PROFILE ? true : false);
        dpmMenu->setEnabled(features.pm == globalStuff::DPM ? true : false);
    } else {
        ui->tabs_pm->setEnabled(false);
        changeProfile->setEnabled(false);
        dpmMenu->setEnabled(false);
    }

    if (!features.clocksAvailable) {
        ui->plotColcks->setVisible(false),
        ui->cb_showFreqGraph->setEnabled(false);
        ui->tabs_systemInfo->setTabEnabled(3,false);
    }

    if (!features.temperatureAvailable) {
        ui->cb_showTempsGraph->setEnabled(false);
        ui->plotTemp->setVisible(false);
    }

    if (!features.voltAvailable) {
        ui->cb_showVoltsGraph->setEnabled(false);
        ui->plotVolts->setVisible(false);
    }

    if (!features.clocksAvailable && !features.temperatureAvailable && !features.voltAvailable)
        ui->mainTabs->setTabEnabled(1,false);
}

// -1 value means that we not show in table. it's default (in gpuClocksStruct constructor), and if we
// did not alter it, it stays and in result will be not displayed
void radeon_profile::refreshGpuData() {
    ui->list_currentGPUData->clear();

    device.getClocks();
    if (device.gpuData.powerLevel != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Current power level" << QString().setNum(device.gpuData.powerLevel)));
    if (device.gpuData.coreClk != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Current GPU clock" << QString().setNum(device.gpuData.coreClk) + " MHz"));
    if (device.gpuData.memClk != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Current mem clock" << QString().setNum(device.gpuData.memClk) + " MHz"));
    if (device.gpuData.uvdCClk != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "UVD video core clock (vclk)" << QString().setNum(device.gpuData.uvdCClk) + " MHz"));
    if (device.gpuData.uvdDClk != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "UVD decoder clock (dclk)" << QString().setNum(device.gpuData.uvdDClk) + " MHz"));
    if (device.gpuData.coreVolt != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Voltage (vddc)" << QString().setNum(device.gpuData.coreVolt) + " mV"));
    if (device.gpuData.memVolt != -1)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Voltage (vddci)" << QString().setNum(device.gpuData.memVolt) + " mV"));

    if (ui->list_currentGPUData->topLevelItemCount() == 0)
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Can't read data. (debugfs mounted? daemon is running? root rights?)"));

    device.getTemperature();
    ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Current GPU temp" << QString().setNum(device.gpuTemeperatureData.current) + QString::fromUtf8("\u00B0C")));

    if (execProcess->state() == QProcess::Running && !execData.logFilename.isEmpty()) {
        QString logData = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") +";" + QString().setNum(device.gpuData.powerLevel) + ";" +
                                QString().setNum(device.gpuData.coreClk) + ";"+
                                QString().setNum(device.gpuData.memClk) + ";"+
                                QString().setNum(device.gpuData.uvdCClk) + ";"+
                                QString().setNum(device.gpuData.uvdDClk) + ";"+
                                QString().setNum(device.gpuData.coreVolt) + ";"+
                                QString().setNum(device.gpuData.memVolt) + ";"+
                                QString().setNum(device.gpuTemeperatureData.current);
        execData.log.append(logData);
    }
}

//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent() {
    if (!refreshWhenHidden->isChecked() && this->isHidden())
        return;

    if (ui->cb_gpuData->isChecked()) {
        refreshGpuData();
        ui->l_profile->setText(device.getCurrentPowerProfile());

        // lets say coreClk is essential to get stats (it is disabled in ui anyway when features.clocksAvailable is false)
        if (ui->cb_stats->isChecked() && device.gpuData.coreClk != -1) {
            doTheStats(device.gpuData);

            // do the math only when user looking at stats table
            if (ui->tabs_systemInfo->currentIndex() == 3)
                updateStatsTable();
        }
    }

    if (ui->cb_graphs->isChecked())
        refreshGraphs(device.gpuData, device.gpuTemeperatureData);

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

void radeon_profile::refreshGraphs(const globalStuff::gpuClocksStruct &_gpuData, const globalStuff::gpuTemperatureStruct &_gpuTemperatureData) {
    // count the tick and move graph to right
    ticksCounter++;
    ui->plotTemp->xAxis->setRange(ticksCounter+20, rangeX,Qt::AlignRight);
    ui->plotTemp->replot();

    ui->plotColcks->xAxis->setRange(ticksCounter+20,rangeX,Qt::AlignRight);
    ui->plotColcks->replot();

    ui->plotVolts->xAxis->setRange(ticksCounter+20,rangeX,Qt::AlignRight);
    ui->plotVolts->replot();

    // choose bigger clock and adjust plot scale
    int r = (_gpuData.memClk >= _gpuData.coreClk) ? _gpuData.memClk : _gpuData.coreClk;
    if (r > ui->plotColcks->yAxis->range().upper)
        ui->plotColcks->yAxis->setRangeUpper(r + 150);

    // add data to plots
    ui->plotColcks->graph(0)->addData(ticksCounter,_gpuData.coreClk);
    ui->plotColcks->graph(1)->addData(ticksCounter,_gpuData.memClk);
    ui->plotColcks->graph(2)->addData(ticksCounter,_gpuData.uvdCClk);
    ui->plotColcks->graph(3)->addData(ticksCounter,_gpuData.uvdDClk);

    if (_gpuData.coreVolt > ui->plotVolts->yAxis->range().upper)
        ui->plotVolts->yAxis->setRangeUpper(_gpuData.coreVolt + 100);

    ui->plotVolts->graph(0)->addData(ticksCounter,_gpuData.coreVolt);
    ui->plotVolts->graph(1)->addData(ticksCounter,_gpuData.memVolt);

    // temperature graph
    if (_gpuTemperatureData.max >= ui->plotTemp->yAxis->range().upper || _gpuTemperatureData.min <= ui->plotTemp->yAxis->range().lower)
        ui->plotTemp->yAxis->setRange(_gpuTemperatureData.min - 5, _gpuTemperatureData.max + 5);

    ui->plotTemp->graph(0)->addData(ticksCounter,_gpuTemperatureData.current);
    ui->l_minMaxTemp->setText("Now: " + QString().setNum(_gpuTemperatureData.current) + "C | Max: " + QString().setNum(_gpuTemperatureData.max) + "C | Min: " +
                              QString().setNum(_gpuTemperatureData.min) + "C | Avg: " + QString().setNum(_gpuTemperatureData.sum/ticksCounter,'f',1));

}

void radeon_profile::doTheStats(const globalStuff::gpuClocksStruct &_gpuData) {
    // count ticks for stats //
    statsTickCounter++;

    // figure out pm level based on data provided
    QString pmLevelName = (_gpuData.powerLevel == -1) ? "" : "Power level:" + QString().setNum(_gpuData.powerLevel), volt;
    volt = (_gpuData.coreVolt == -1) ? "" : "(" + QString().setNum(_gpuData.coreVolt) + "mV)";
    pmLevelName = (_gpuData.coreClk == -1) ? pmLevelName : pmLevelName + " Core:" +QString().setNum(_gpuData.coreClk) + "MHz" + volt;

    volt = (_gpuData.memVolt == -1) ? "" : "(" + QString().setNum(_gpuData.memVolt) + "mV)";
    pmLevelName = (_gpuData.memClk == -1) ? pmLevelName : pmLevelName + " Mem:" + QString().setNum(_gpuData.memClk) + "MHz" + volt;

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
    QString tooltipdata = radeon_profile::windowTitle() + "\nCurrent profile: "+ui->l_profile->text() +"\n";
    for (short i = 0; i < ui->list_currentGPUData->topLevelItemCount(); i++) {
        tooltipdata += ui->list_currentGPUData->topLevelItem(i)->text(0) + ": " + ui->list_currentGPUData->topLevelItem(i)->text(1) + '\n';
    }
    tooltipdata.remove(tooltipdata.length() - 1, 1); //remove empty line at bootom
    trayIcon->setToolTip(tooltipdata);
}
