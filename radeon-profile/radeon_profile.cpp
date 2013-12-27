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

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QTimer>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QProcess>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QColorDialog>

#define startClocksScaleL 100
#define startClocksScaleH 400
#define startVoltsScaleL 500
#define startVoltsScaleH 650

const int appVersion = 20131227;

static int i = 0;
static double maxT = 0.0, minT = 0.0, current, tempSum = 0, rangeX = 180;
static char selectedPowerMethod, selectedTempSensor, sensorsGPUtempIndex;
static bool closeFromTrayMenu;
static QString
    powerMethodFilePath, profilePath, dpmStateFilePath, clocksPath, forcePowerLevelFilePath, sysfsHwmonPath, moduleParamsPath,
    err = "Err",
    noValues = "no values";
static const QString settingsPath = QDir::homePath() + "/.radeon-profile-settings";

radeon_profile::radeon_profile(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);
    timer = new QTimer();

    detectCards();
    figureOutGPUDataPaths(ui->combo_gpus->currentText());

    // setup ui elemensts
    ui->mainTabs->setCurrentIndex(0);
    ui->tabs_systemInfo->setCurrentIndex(0);
    ui->plotVolts->setVisible(false);
    setupGraphs();
    setupForcePowerLevelMenu();
    setupOptionsMenu();
    setupTrayIcon();

    loadConfig();

    // first get some info about system
    getGLXInfo();
    testSensor();
    getModuleInfo();
    getPowerMethod();
    getCardConnectors();

   // timer init
    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    connect(ui->timeSlider,SIGNAL(valueChanged(int)),this,SLOT(changeTimeRange()));
    timer->setInterval(ui->spin_timerInterval->value()*1000);

    // dpm or profile tab enable
    switch (selectedPowerMethod) {
    case DPM: {
        changeProfile->setEnabled(false);
        ui->tabs_pm->setCurrentIndex(1);
        ui->tabs_pm->setTabEnabled(0,false);
        break;
    }
    case PROFILE: {
        dpmMenu->setEnabled(false);
        ui->tabs_pm->setCurrentIndex(0);
        ui->tabs_pm->setTabEnabled(1,false);
        break;
    }
    case PM_UNKNOWN: {
        dpmMenu->setEnabled(false);
        changeProfile->setEnabled(false);
        break;
    }
    }

    timerEvent();
    timer->start();
    radeon_profile::setWindowTitle("Radeon Profile (v. "+QString().setNum(appVersion)+")");

    // add button for manual refresh glx info, connectors, mod params
    QPushButton *refreshBtn = new QPushButton();
    refreshBtn->setIcon(QIcon(":/icon/symbols/refresh.png"));
    ui->tabs_systemInfo->setCornerWidget(refreshBtn);
    refreshBtn->setIconSize(QSize(20,20));
    refreshBtn->show();
    connect(refreshBtn,SIGNAL(clicked()),this,SLOT(refreshBtnClicked()));
}

radeon_profile::~radeon_profile()
{
    delete ui;
}

//===================================
// === Main timer loop  === //
void radeon_profile::timerEvent() {
    if (!refreshWhenHidden->isChecked() && this->isHidden())
        return;

    if (ui->cb_gpuData->isChecked()) {
        switch (selectedPowerMethod) {
        case DPM:
        case PROFILE: {
            ui->l_profile->setText(getCurrentPowerProfile());
            break;
        }
        case PM_UNKNOWN: {
            ui->tabs_pm->setEnabled(false);
            ui->list_currentGPUData->addItem("Can't read data. Card not detected.");
            return;
            break;
        }
        }

        ui->list_currentGPUData->addItems(fillGpuDataTable());

        if (ui->cb_graphs->isChecked()) {
            // count seconds to move graph to right
            i++;
            ui->plotTemp->xAxis->setRange(i+20, rangeX,Qt::AlignRight);
            ui->plotTemp->replot();

            ui->plotColcks->xAxis->setRange(i+20,rangeX,Qt::AlignRight);
            ui->plotColcks->replot();

            ui->plotVolts->xAxis->setRange(i+20,rangeX,Qt::AlignRight);
            ui->plotVolts->replot();
        }
        refreshTooltip();
    }
    if (ui->cb_glxInfo->isChecked())
        getGLXInfo();
    if (ui->cb_connectors->isChecked())
        getCardConnectors();
    if (ui->cb_modParams->isChecked())
        getModuleInfo();
}
//========

// === Run commands to read sysinfo
QStringList radeon_profile::grabSystemInfo(const QString cmd) {
    QProcess *p = new QProcess();
    p->setProcessChannelMode(QProcess::MergedChannels);

    p->start(cmd,QIODevice::ReadOnly);
    p->waitForFinished();

    return QString(p->readAllStandardOutput()).split('\n');
}

QStringList radeon_profile::grabSystemInfo(const QString cmd, const QProcessEnvironment env) {
    QProcess *p = new QProcess();
    p->setProcessChannelMode(QProcess::MergedChannels);
    p->setProcessEnvironment(env);

    p->start(cmd,QIODevice::ReadOnly);
    p->waitForFinished();

    return QString(p->readAllStandardOutput()).split('\n');
}
// === Fill the table with data
QStringList radeon_profile::fillGpuDataTable() {
    ui->list_currentGPUData->clear();
    QStringList completeData;
    completeData << getClocks();
    completeData << getGPUTemp();
    return completeData;
}

//===================================
// === Run on startup system info ===//
void radeon_profile::getPowerMethod() {
    QFile powerMethodFile(powerMethodFilePath);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            selectedPowerMethod = DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            selectedPowerMethod = PROFILE;
        else
            selectedPowerMethod = PM_UNKNOWN;
    } else
        selectedPowerMethod = PM_UNKNOWN;
}

void radeon_profile::testSensor() {
    QFile hwmon(sysfsHwmonPath);

    // first, try read temp from sysfs (no need for lm_sensors), if it fails, try other methods
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty()) {
            selectedTempSensor = SYSFS_HWMON;
            return;
        }
    } else {
        QStringList out = grabSystemInfo("sensors");
        if (out.indexOf(QRegExp("radeon-pci.+")) != -1) {
            selectedTempSensor = PCI_SENSOR;
            sensorsGPUtempIndex = out.indexOf(QRegExp("radeon-pci.+"));  // in order to not search for it again in timer loop
            return;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            selectedTempSensor = MB_SENSOR;
            sensorsGPUtempIndex = out.indexOf(QRegExp("VGA_TEMP.+"));
            return;
        }
        else
            selectedTempSensor = TS_UNKNOWN;
    }
}

void radeon_profile::getModuleInfo() {
    ui->list_modInfo->clear();
    QStringList modInfo = grabSystemInfo("modinfo -p radeon");
    modInfo.sort();

    for (int i =0; i < modInfo.count(); i++) {
        if (modInfo[i].contains(":")) {
            // read module param name and description from modinfo command
            QString modName = modInfo[i].split(":",QString::SkipEmptyParts)[0],
                    modDesc = modInfo[i].split(":",QString::SkipEmptyParts)[1],
                    modValue;

            // read current param values
            QFile mp(moduleParamsPath+modName);
            modValue = (mp.open(QIODevice::ReadOnly)) ?  modValue = mp.readLine(20) : "unknown";

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.left(modName.indexOf('\n')) << modValue.left(modValue.indexOf('\n')) << modDesc.left(modDesc.indexOf('\n')));
            ui->list_modInfo->addTopLevelItem(item);
            mp.close();
        }
    }
}

void radeon_profile::getGLXInfo() {
    ui->list_glxinfo->clear();
    QStringList data, gpus = grabSystemInfo("lspci").filter("Radeon",Qt::CaseInsensitive);
    gpus.removeAt(gpus.indexOf(QRegExp(".+Audio.+"))); //remove radeon audio device

    // loop for multi gpu
    for (int i = 0; i < gpus.count(); i++)
        data << "VGA:"+gpus[i].split(":",QString::SkipEmptyParts)[2];

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DRI_PRIME",ui->combo_gpus->currentText().at(ui->combo_gpus->currentText().length()-1));

    QStringList driver = grabSystemInfo("xdriinfo",env).filter("Screen 0:",Qt::CaseInsensitive);
    if (!driver.isEmpty())  // because of segfault when no xdriinfo
        data << "Driver:"+ driver.filter("Screen 0:",Qt::CaseInsensitive)[0].split(":",QString::SkipEmptyParts)[1];

    data << grabSystemInfo("glxinfo",env).filter(QRegExp("direct|OpenGL.+:.+"));
    ui->list_glxinfo->addItems(data);
}

void radeon_profile::getCardConnectors() {
    ui->list_connectors->clear();
    QStringList out = grabSystemInfo("xrandr -q --verbose"), screens;
    screens = out.filter(QRegExp("Screen\\s\\d"));
    for (int i = 0; i < screens.count(); i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << screens[i].split(':')[0] << screens[i].split(",")[1].remove(" current "));
        ui->list_connectors->addTopLevelItem(item);
    }
    ui->list_connectors->addTopLevelItem(new QTreeWidgetItem(QStringList() << "------"));

    for(int i = 0; i < out.size(); i++) {
        if(!out[i].startsWith("\t") && out[i].contains("connected")) {
            QString connector = out[i].split(' ')[0],
                    status = out[i].split(' ')[1],
                    res = out[i].split(' ')[2].split('+')[0];

            if(status == "connected") {
                QString monitor, edid = monitor = "";

                // find EDID
                for (int i = out.indexOf(QRegExp(".+EDID.+"))+1; i < out.count(); i++)
                    if (out[i].startsWith(("\t\t")))
                        edid += out[i].remove("\t\t");
                    else
                        break;

                // Parse EDID
                // See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
                if(edid.size() >= 256) {
                    QStringList hex;
                    bool found = false, ok = true;
                    int i2 = 108;

                    for(int i3 = 0; i3 < 4; i3++) {
                        if(edid.mid(i2, 2).toInt(&ok, 16) == 0 && ok &&
                                edid.mid(i2 + 2, 2).toInt(&ok, 16) == 0) {
                            // Other Monitor Descriptor found
                            if(ok && edid.mid(i2 + 6, 2).toInt(&ok, 16) == 0xFC && ok) {
                                // Monitor name found
                                for(int i4 = i2 + 10; i4 < i2 + 34; i4 += 2)
                                    hex << edid.mid(i4, 2);
                                found = true;
                                break;
                            }
                        }
                        if(!ok)
                            break;
                        i2 += 36;
                    }
                    if(ok && found) {
                        // Hex -> String
                        for(i2 = 0; i2 < hex.size(); i2++) {
                            monitor += QString(hex[i2].toInt(&ok, 16));
                            if(!ok)
                                break;
                        }

                        if(ok)
                            monitor = " (" + monitor.left(monitor.indexOf('\n')) + ")";
                    }
                }
                status += monitor + " @ " + QString((res.contains('x')) ? res : "unknown");
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << connector << status);
            ui->list_connectors->addTopLevelItem(item);
        }
    }
}

void radeon_profile::detectCards() {
    QStringList out = grabSystemInfo("ls /sys/class/drm/").filter("card");
    for (char i = 0; i < out.count(); i++) {
        QFile f("/sys/class/drm/"+out[i]+"/device/uevent");
        if (f.open(QIODevice::ReadOnly)) {
            if (f.readLine(50).contains("DRIVER=radeon"))
                ui->combo_gpus->addItem(f.fileName().split('/')[4]);
        }
    }
}

void radeon_profile::figureOutGPUDataPaths(const QString gpuName) {
    powerMethodFilePath = "/sys/class/drm/"+gpuName+"/device/power_method",
    profilePath = "/sys/class/drm/"+gpuName+"/device/power_profile",
    dpmStateFilePath = "/sys/class/drm/"+gpuName+"/device/power_dpm_state",
    clocksPath = "/sys/kernel/debug/dri/"+QString(gpuName.at(gpuName.length()-1))+"/radeon_pm_info",  // this path contains only index
    forcePowerLevelFilePath = "/sys/class/drm/"+gpuName+"/device/power_dpm_force_performance_level",
    moduleParamsPath = "/sys/class/drm/"+gpuName+"/device/driver/module/holders/radeon/parameters/";

    QString hwmonDevice = grabSystemInfo("ls /sys/class/drm/"+gpuName+"/device/hwmon/")[0]; // look for hwmon devices in card dir
    sysfsHwmonPath = "/sys/class/drm/"+gpuName+"/device/hwmon/" + QString((hwmonDevice.isEmpty()) ? "hwmon0" : hwmonDevice) + "/temp1_input";
}
//========

//===================================
// === Core of the app, get clocks, power profile and temps ===//
QStringList radeon_profile::getClocks() {
    QStringList gpuData;
    double coreClock = 0,memClock= 0, voltsGPU = 0, uvdvclk = 0, uvddclk = 0;  // for plots

    if (QFile(clocksPath).exists()) {  // check for debugfs access
        QStringList out = grabSystemInfo("cat "+clocksPath);

        switch (selectedPowerMethod) {
        case DPM: {
            QRegExp rx;

            rx.setPattern("sclk:\\s\\d+");
            rx.indexIn(out[1]);
            if (!rx.cap(0).isEmpty()) {
                coreClock = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                gpuData << "Current GPU clock: " + QString().setNum(coreClock) + " MHz";
            }

            rx.setPattern("mclk:\\s\\d+");
            rx.indexIn(out[1]);
            if (!rx.cap(0).isEmpty()) {
                memClock = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                gpuData << "Current mem clock: " + QString().setNum(memClock) + " MHz";
            }

            rx.setPattern("vclk:\\s\\d+");
            rx.indexIn(out[0]);
            if (!rx.cap(0).isEmpty()) {
                uvdvclk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                if (uvdvclk != 0)
                    gpuData << "UVD video core clock (vclk): " + QString().setNum(uvdvclk) + " MHz";
            }

            rx.setPattern("dclk:\\s\\d+");
            rx.indexIn(out[0]);
            if (!rx.cap(0).isEmpty()) {
                uvddclk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                if (uvddclk != 0)
                    gpuData << "UVD decoder clock (dclk): " + QString().setNum(uvddclk) + " MHz";
            }

            rx.setPattern("vddc:\\s\\d+");
            rx.indexIn(out[1]);
            if (!rx.cap(0).isEmpty()) {
                voltsGPU = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();
                gpuData << "------------------------";
                gpuData << "Voltage (vddc): " + QString().setNum(voltsGPU) + " mV";
            }

            rx.setPattern("vddci:\\s\\d+");
            rx.indexIn(out[1]);
            if (!rx.cap(0).isEmpty())
                gpuData << "Voltage (vddci): " + rx.cap(0).split(' ',QString::SkipEmptyParts)[1] + " mV";

            break;
        }
        case PROFILE: {
            for (int i=0; i< out.count(); i++) {
                switch (i) {
                case 1: {
                    if (out[i].contains("current engine clock")) {
                        coreClock = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                        gpuData << "Current GPU clock: " + QString().setNum(coreClock) + " MHz";
                        break;
                    }
                };
                case 3: {
                    if (out[i].contains("current memory clock")) {
                        memClock = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                        gpuData << "Current mem clock: " + QString().setNum(memClock) + " MHz";
                        break;
                    }
                }
                case 4: {
                    if (out[i].contains("voltage")) {
                        gpuData << "-------------------------";
                        voltsGPU = QString().setNum(out[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toDouble();
                        gpuData << "Voltage: " + QString().setNum(voltsGPU) + " mV";
                        break;
                    }
                }
                }
            }
            break;
        }
        case PM_UNKNOWN: {
            gpuData << "Power method unknown.";
            ui->cb_showFreqGraph->setChecked(false),ui->cb_showFreqGraph->setEnabled(false),ui->cb_showVoltsGraph->setEnabled(false),
                    ui->plotColcks->setVisible(false),ui->plotVolts->setVisible(false);
            return gpuData;
            break;
        }
        }
    }
    else {
        gpuData << "Current GPU clock: " + noValues + " (root rights? debugfs mounted?)";
        gpuData << "Current mem clock: " + noValues + " (root rights? debugfs mounted?)";
        gpuData << "------------------------";
        gpuData << "Voltage: "+ noValues + " (root rights? debugfs mounted?)";
        ui->cb_showFreqGraph->setChecked(false),ui->cb_showFreqGraph->setEnabled(false),ui->cb_showVoltsGraph->setEnabled(false),
                ui->plotColcks->setVisible(false),ui->plotVolts->setVisible(false);
    }
    gpuData << "------------------------";

    if (ui->cb_graphs->isChecked()) {
        // update plots
        if (memClock > ui->plotColcks->yAxis->range().upper && memClock != 0) // assume that mem clocks are often bigger than core
            ui->plotColcks->yAxis->setRangeUpper(memClock + 150);
        else if (coreClock != 0 && memClock == 0)
            ui->plotColcks->yAxis->setRangeUpper(coreClock + 150);

        ui->plotColcks->graph(0)->addData(i,coreClock);
        ui->plotColcks->graph(1)->addData(i,memClock);
        ui->plotColcks->graph(2)->addData(i,uvdvclk);
        ui->plotColcks->graph(3)->addData(i,uvddclk);

        if (voltsGPU != 0)  {
            if (voltsGPU > ui->plotVolts->yAxis->range().upper)
                ui->plotVolts->yAxis->setRangeUpper(voltsGPU + 100);

            ui->plotVolts->graph(0)->addData(i,voltsGPU);
        }
        else
            ui->cb_showVoltsGraph->setEnabled(false);
    }
    return gpuData;
}

QString radeon_profile::getCurrentPowerProfile() {
    QFile forceProfile(forcePowerLevelFilePath);
    QString pp;

    switch (selectedPowerMethod) {
    case DPM: {
        QFile profile(dpmStateFilePath);
            if (profile.open(QIODevice::ReadOnly)) {
                pp = profile.readLine(13);
                if (forceProfile.open(QIODevice::ReadOnly))
                    pp += " | " + forceProfile.readLine(5);
            } else
                pp = err;
            break;
    }
    case PROFILE: {
        QFile profile(profilePath);
        if (profile.open(QIODevice::ReadOnly))
            pp = profile.readLine(13);
        break;
    }
    case PM_UNKNOWN: {
        pp = err;
        break;
    }
    }

    return pp.remove('\n');
}

QString radeon_profile::getGPUTemp() {
    QString temp;

    switch (selectedTempSensor) {
    case SYSFS_HWMON: {
        QFile hwmon(sysfsHwmonPath);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        current = temp.toDouble() / 1000;
        break;
    }
    case PCI_SENSOR: {
        QStringList out = grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        current = temp.toDouble();
        break;
    }
    case MB_SENSOR: {
        QStringList out = grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        current = temp.toDouble();
        break;
    }
    case TS_UNKNOWN: {
        ui->plotTemp->setVisible(false),ui->cb_showTempsGraph->setEnabled(false),
                ui->cb_showTempsGraph->setChecked(false),ui->l_minMaxTemp->setVisible(false);
        return "";
        break;
    }
    }

    tempSum += current;

    if (minT == 0)
        minT = current;
    maxT = (current >= maxT) ? current : maxT;
    minT = (current <= minT) ? current : minT;

    if (maxT >= ui->plotTemp->yAxis->range().upper || minT <= ui->plotTemp->yAxis->range().lower)
        ui->plotTemp->yAxis->setRange(minT - 5, maxT + 5);

    ui->plotTemp->graph(0)->addData(i,current);
    ui->l_minMaxTemp->setText("Now: " + QString().setNum(current) + "C | Max: " + QString().setNum(maxT) + "C | Min: " + QString().setNum(minT) + "C | Avg: " + QString().setNum(tempSum/i,'f',1));

    return "Current GPU temp: "+QString().setNum(current) + QString::fromUtf8("\u00B0C");
}
//========

//===================================
// === GUI events === //
// == menu forcePowerLevel
void radeon_profile::forceAuto() {
    setValueToFile(forcePowerLevelFilePath,"auto");
}

void radeon_profile::forceLow() {
    setValueToFile(forcePowerLevelFilePath,"low");
}

void radeon_profile::forceHigh() {
    setValueToFile(forcePowerLevelFilePath,"high");
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

// == others
void radeon_profile::on_btn_dpmBattery_clicked() {
    setValueToFile(dpmStateFilePath,"battery");
}

void radeon_profile::on_btn_dpmBalanced_clicked() {
    setValueToFile(dpmStateFilePath,"balanced");
}

void radeon_profile::on_btn_dpmPerformance_clicked() {
    setValueToFile(dpmStateFilePath,"performance");
}

void radeon_profile::resetMinMax() { minT = 0; maxT = 0; }

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
        ui->plotColcks->replot();
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

void radeon_profile::on_combo_gpus_currentTextChanged(const QString &arg1)
{
    figureOutGPUDataPaths(arg1); // resolve paths for newly selected card

    // do initial stuff once again for new card
    testSensor();
    getModuleInfo();
    getPowerMethod();
    getCardConnectors();
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
    }
}

void radeon_profile::closeEvent(QCloseEvent *e) {
    if (ui->cb_closeTray->isChecked() && !closeFromTrayMenu) {
        this->hide();
        e->ignore();
    }
    saveConfig();
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
    if (ui->cb_graphs->isChecked())
        ui->mainTabs->setTabEnabled(1,checked);
}

void radeon_profile::refreshBtnClicked() {
    getGLXInfo();
    getCardConnectors();
    getModuleInfo();
}

void radeon_profile::on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    item->setBackgroundColor(1,QColorDialog::getColor(item->backgroundColor(1)));
    // apply colors
    setupGraphsStyle();
}
//========

//===================================
// === Additional functions === //
void radeon_profile::setValueToFile(const QString filePath, const QStringList profiles) {
    bool ok;
    QString newProfile = QInputDialog::getItem(this, "Select new Radeon profile (ONLY ROOT)", "Profile",profiles,0,false,&ok);

    if (ok)
        setValueToFile(filePath, newProfile);
}

void radeon_profile::setValueToFile(const QString filePath, const QString newValue) {
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&file);
    stream << newValue + "\n";
    file.flush();
    file.close();
}

void radeon_profile::on_chProfile_clicked() {
    setValueToFile(profilePath, QStringList() << "default" << "auto" << "low" << "mid" << "high");
}

void radeon_profile::refreshTooltip()
{
    QString tooltipdata = radeon_profile::windowTitle() + "\nCurrent profile: "+ui->l_profile->text() +"\n";
    for (short i = 0; i < ui->list_currentGPUData->count(); i++) {
        tooltipdata += ui->list_currentGPUData->item(i)->text() + '\n';
    }
    trayIcon->setToolTip(tooltipdata);
}

void radeon_profile::saveConfig() {
    QSettings settings(settingsPath,QSettings::IniFormat);

    settings.setValue("startMinimized",ui->cb_startMinimized->isChecked());
    settings.setValue("minimizeToTray",ui->cb_minimizeTray->isChecked());
    settings.setValue("closeToTray",ui->cb_closeTray->isChecked());
    settings.setValue("updateInterval",ui->spin_timerInterval->value());
    settings.setValue("updateGPUData",ui->cb_gpuData->isChecked());
    settings.setValue("updateGraphs",ui->cb_graphs->isChecked());
    settings.setValue("updateGLXInfo",ui->cb_glxInfo->isChecked());
    settings.setValue("updateConnectors",ui->cb_connectors->isChecked());
    settings.setValue("updateModParams",ui->cb_modParams->isChecked());
    settings.setValue("saveWindowGeometry",ui->cb_saveWindowGeometry->isChecked());
    settings.setValue("windowGeometry",this->geometry());

    settings.setValue("graphLineThickness",ui->spin_lineThick->value());
    settings.setValue("graphTempBackground",ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1));
    settings.setValue("graphClocksBackground",ui->graphColorsList->topLevelItem(CLOCKS_BG)->backgroundColor(1));
    settings.setValue("graphVoltsBackground",ui->graphColorsList->topLevelItem(VOLTS_BG)->backgroundColor(1));
    settings.setValue("graphTempLine",ui->graphColorsList->topLevelItem(TEMP_LINE)->backgroundColor(1));
    settings.setValue("graphGPUClockLine",ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->backgroundColor(1));
    settings.setValue("graphMemClockLine",ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->backgroundColor(1));
    settings.setValue("graphUVDVideoLine",ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->backgroundColor(1));
    settings.setValue("graphUVDDecoderLine",ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->backgroundColor(1));
    settings.setValue("graphVoltsLine",ui->graphColorsList->topLevelItem(VOLTS_LINE)->backgroundColor(1));
}

void radeon_profile::loadConfig() {
    QSettings settings(settingsPath,QSettings::IniFormat);

    ui->cb_startMinimized->setChecked(settings.value("startMinimized",false).toBool());
    ui->cb_minimizeTray->setChecked(settings.value("minimizeToTray",false).toBool());
    ui->cb_closeTray->setChecked(settings.value("closeToTray",false).toBool());
    ui->spin_timerInterval->setValue(settings.value("updateInterval",1).toDouble());
    ui->cb_gpuData->setChecked(settings.value("updateGPUData",true).toBool());
    ui->cb_graphs->setChecked(settings.value("updateGraphs",true).toBool());
    ui->cb_glxInfo->setChecked(settings.value("updateGLXInfo",false).toBool());
    ui->cb_connectors->setChecked(settings.value("updateConnectors",false).toBool());
    ui->cb_modParams->setChecked(settings.value("updateModParams",false).toBool());
    ui->cb_saveWindowGeometry->setChecked(settings.value("saveWindowGeometry").toBool());

    ui->spin_lineThick->setValue(settings.value("graphLineThickness",2).toInt());

    // detalis: http://qt-project.org/doc/qt-4.8/qvariant.html#a-note-on-gui-types
    //ok, color is saved as QVariant, and read and convertsion it to QColor is below
    ui->graphColorsList->topLevelItem(TEMP_BG)->setBackgroundColor(1,settings.value("graphTempBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(CLOCKS_BG)->setBackgroundColor(1,settings.value("graphClocksBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(VOLTS_BG)->setBackgroundColor(1,settings.value("graphVoltsBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(TEMP_LINE)->setBackgroundColor(1,settings.value("graphTempLine",Qt::yellow).value<QColor>());
    ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphGPUClockLine",Qt::black).value<QColor>());
    ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphMemClockLine",Qt::cyan).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->setBackgroundColor(1,settings.value("graphUVDVideoLine",Qt::red).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->setBackgroundColor(1,settings.value("graphUVDDecoderLine",Qt::green).value<QColor>());
    ui->graphColorsList->topLevelItem(VOLTS_LINE)->setBackgroundColor(1,settings.value("graphVoltsLine",Qt::blue).value<QColor>());
    setupGraphsStyle();

    // apply some settings to ui on start //
    if (ui->cb_saveWindowGeometry->isChecked())
        this->setGeometry(settings.value("windowGeometry").toRect());

    if (ui->cb_startMinimized->isChecked())
        this->window()->hide();
    else
        showNormal();

    ui->cb_graphs->setEnabled(ui->cb_gpuData->isChecked());

    if (ui->cb_graphs->isChecked() && ui->cb_graphs->isEnabled())
        ui->mainTabs->setTabEnabled(1,true);
    else
        ui->mainTabs->setTabEnabled(1,false);
}
//========

//===================================
// === GUI setup functions === //
void radeon_profile::setupGraphs()
{


    ui->plotColcks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);

    ui->plotTemp->xAxis->setLabel("time");
    ui->plotTemp->yAxis->setLabel("temperature");
    ui->plotColcks->xAxis->setLabel("time");
    ui->plotColcks->yAxis->setLabel("MHz");
    ui->plotVolts->xAxis->setLabel("time");
    ui->plotVolts->yAxis->setLabel("mV");
    ui->plotTemp->xAxis->setTickLabels(false);
    ui->plotColcks->xAxis->setTickLabels(false);
    ui->plotVolts->xAxis->setTickLabels(false);

    ui->plotTemp->addGraph(); // temp graph
    ui->plotColcks->addGraph(); // core clock graph
    ui->plotColcks->addGraph(); // mem clock graph
    ui->plotColcks->addGraph(); // uvd
    ui->plotColcks->addGraph(); // uvd
    ui->plotVolts->addGraph(); // volts gpu

    setupGraphsStyle();

    // legend clocks //
    ui->plotColcks->graph(0)->setName("GPU clock");
    ui->plotColcks->graph(1)->setName("Memory clock");
    ui->plotColcks->graph(2)->setName("Video core clock");
    ui->plotColcks->graph(3)->setName("Decoder clock");
    ui->plotColcks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotColcks->legend->setVisible(true);

    ui->plotColcks->replot();
    ui->plotTemp->replot();
    ui->plotVolts->replot();
}

void radeon_profile::setupGraphsStyle()
{
    QPen pen;
    pen.setWidth(ui->spin_lineThick->value());
    pen.setCapStyle(Qt::SquareCap);
    pen.setColor(ui->graphColorsList->topLevelItem(TEMP_LINE)->backgroundColor(1));
    ui->plotTemp->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->backgroundColor(1));
    ui->plotColcks->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->backgroundColor(1));
    ui->plotColcks->graph(1)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->backgroundColor(1));
    ui->plotColcks->graph(2)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->backgroundColor(1));
    ui->plotColcks->graph(3)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(VOLTS_LINE)->backgroundColor(1));
    ui->plotVolts->graph(0)->setPen(pen);

    ui->plotTemp->setBackground(QBrush(ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1)));
    ui->plotColcks->setBackground(QBrush(ui->graphColorsList->topLevelItem(CLOCKS_BG)->backgroundColor(1)));
    ui->plotVolts->setBackground(QBrush(ui->graphColorsList->topLevelItem(VOLTS_BG)->backgroundColor(1)));
}

void radeon_profile::setupTrayIcon() {
    trayMenu = new QMenu();
    setWindowState(Qt::WindowMinimized);
    //close //
    closeApp = new QAction(this);
    closeApp->setText("Quit");
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // standard profiles
    changeProfile = new QAction(this);
    changeProfile->setText("Change standard profile");
    connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

    // refresh when hidden
    refreshWhenHidden = new QAction(this);
    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(true);
    refreshWhenHidden->setText("Keep refreshing when hidden");

    // dpm menu //
    dpmMenu = new QMenu(this);
    dpmMenu->setTitle("DPM");

    dpmSetBattery = new QAction(this);
    dpmSetBalanced = new QAction(this);
    dpmSetPerformance = new QAction(this);

    dpmSetBattery->setText("Battery");
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText("Balanced");
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText("Performance");
    dpmSetPerformance->setIcon(QIcon(":/icon/symbols/arrow3.png"));

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(on_btn_dpmBattery_clicked()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(on_btn_dpmBalanced_clicked()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(on_btn_dpmPerformance_clicked()));

    dpmMenu->addAction(dpmSetBattery);
    dpmMenu->addAction(dpmSetBalanced);
    dpmMenu->addAction(dpmSetPerformance);
    dpmMenu->addSeparator();
    dpmMenu->addMenu(forcePowerMenu);

    // add stuff above to menu //
    trayMenu->addAction(refreshWhenHidden);
    trayMenu->addSeparator();
    trayMenu->addAction(changeProfile);
    trayMenu->addMenu(dpmMenu);
    trayMenu->addSeparator();
    trayMenu->addAction(closeApp);

    // setup icon finally //
    QIcon appicon(":/icon/extra/radeon-profile.png");
    trayIcon = new QSystemTrayIcon(appicon,this);
    trayIcon->show();
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

void radeon_profile::setupOptionsMenu()
{
    optionsMenu = new QMenu(this);
    ui->btn_options->setMenu(optionsMenu);

    QAction *resetMinMax = new QAction(this);
    resetMinMax->setText("Reset min/max temperature");

    QAction *resetGraphs = new QAction(this);
    resetGraphs->setText("Reset graphs vertical scale");

    QAction *showLegend = new QAction(this);
    showLegend->setText("Show legend");
    showLegend->setCheckable(true);
    showLegend->setChecked(true);

    optionsMenu->addAction(showLegend);
    optionsMenu->addSeparator();
    optionsMenu->addAction(resetMinMax);
    optionsMenu->addAction(resetGraphs);

    connect(resetMinMax,SIGNAL(triggered()),this,SLOT(resetMinMax()));
    connect(resetGraphs,SIGNAL(triggered()),this,SLOT(resetGraphs()));
    connect(showLegend,SIGNAL(triggered(bool)),this,SLOT(showLegend(bool)));
}

void radeon_profile::setupForcePowerLevelMenu() {
    forcePowerMenu = new QMenu(this);

    QAction *forceAuto = new QAction(this);
    forceAuto->setText("Auto");

    QAction *forceLow = new QAction(this);
    forceLow->setText("Low");

    QAction *forceHigh = new QAction(this);
    forceHigh->setText("High");

    forcePowerMenu->setTitle("Force power level");
    forcePowerMenu->addAction(forceAuto);
    forcePowerMenu->addSeparator();
    forcePowerMenu->addAction(forceLow);
    forcePowerMenu->addAction(forceHigh);

    connect(forceAuto,SIGNAL(triggered()),this,SLOT(forceAuto()));
    connect(forceLow,SIGNAL(triggered()),this,SLOT(forceLow()));
    connect(forceHigh,SIGNAL(triggered()),this,SLOT(forceHigh()));
}
//========
