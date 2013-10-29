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
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>

#define startClocksScaleL 100
#define startClocksScaleH 400
#define startVoltsScaleL 500
#define startVoltsScaleH 650

const int appVersion = 20131029;

radeon_profile::radeon_profile(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);

    appHomePath = QDir::homePath() + "/.radeon-profile";
    QDir appHome = appHomePath;
    if (!appHome.exists())
        appHome.mkdir(QDir::homePath() + "/.radeon-profile");

    ui->list_glxinfo->addItems(getGLXInfo());
    ui->mainTabs->setCurrentIndex(0);
    ui->tabWidget_2->setCurrentIndex(0);
    ui->plotVolts->setVisible(false);
    setupGraphs();
    setupForcePowerLevelMenu();
    setupOptionsMenu();
    setupTrayIcon();
    testSensor();
    getModuleInfo();

    QTimer *timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    connect(ui->timeSlider,SIGNAL(valueChanged(int)),this,SLOT(changeTimeRange()));
    timer->start(1000);
    timerEvent();

    if (ui->stdProfiles->isEnabled())
        dpmMenu->setEnabled(false);
    if (ui->dpmProfiles->isEnabled())
        changeProfile->setEnabled(false);

    radeon_profile::setWindowTitle("Radeon Profile (v. "+QString().setNum(appVersion)+")");
}

radeon_profile::~radeon_profile()
{
    delete ui;
}

void radeon_profile::setValueToFile(const QString filePath, const QStringList profiles) {
    bool ok;
    QString newProfile = QInputDialog::getItem(this, "Select new Radeon profile (ONLY ROOT)", "Profile",profiles,0,false,&ok);

    if (ok) {
        system(QString("echo "+ newProfile + " > "+ filePath ).toStdString().c_str());
    }
}

void radeon_profile::setValueToFile(const QString filePath, const QString newValue) {
    system(QString("echo "+ newValue + " > "+ filePath ).toStdString().c_str());
}

void radeon_profile::on_chProfile_clicked() {
    setValueToFile(radeon_profile::profilePath, QStringList() << "default" << "auto" << "low" << "mid" << "high");
}

void radeon_profile::timerEvent() {
    if (!refreshWhenHidden->isChecked() && this->isHidden())
        return;

    QString s = getPowerMethod();
    if (s.contains("profile",Qt::CaseInsensitive)) {
        ui->tabWidget->setCurrentIndex(0);
        ui->tabWidget->setTabEnabled(1,false);

        ui->l_profile->setText(getCurrentPowerProfile(profilePath));
        ui->list_currentGPUData->addItems(fillGpuDataTable("profile"));
    } else if (s.contains("dpm",Qt::CaseInsensitive)) {
        ui->tabWidget->setCurrentIndex(1);
        ui->tabWidget->setTabEnabled(0,false);

        ui->l_profile->setText(getCurrentPowerProfile(dpmStateFile));
        ui->list_currentGPUData->addItems(fillGpuDataTable("dpm"));
    } else {
        ui->list_currentGPUData->addItem("Can't read data");
    }

    // count seconds to move graph to right
    i++;
    ui->plotTemp->xAxis->setRange(i+20, rangeX,Qt::AlignRight);
    ui->plotTemp->replot();

    ui->plotColcks->xAxis->setRange(i+20,rangeX,Qt::AlignRight);
    ui->plotColcks->replot();

    ui->plotVolts->xAxis->setRange(i+20,rangeX,Qt::AlignRight);
    ui->plotVolts->replot();

    refreshTooltip();
}

void radeon_profile::refreshTooltip()
{
    QString tooltipdata = radeon_profile::windowTitle() + "\nCurrent profile: "+ui->l_profile->text() +"\n";
    for (short i = 0; i < ui->list_currentGPUData->count(); i++) {
        tooltipdata += ui->list_currentGPUData->item(i)->text() + '\n';
    }
    trayIcon->setToolTip(tooltipdata);
}

QString radeon_profile::getPowerMethod() {
    QFile powerMethodFile(radeon_profile::powerMethodFile);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString pMethod = powerMethodFile.readLine(20);
        return pMethod;
    } else
        return QString::null;
}

QStringList radeon_profile::getClocks(const QString powerMethod) {
    QStringList gpuData;
    double coreClock = 0,memClock= 0, voltsGPU = 0, uvdvclk = 0, uvddclk = 0;  // for plots

    if (QFile(radeon_profile::clocksPath).exists()) {
        system(QString("cp " + radeon_profile::clocksPath + " " + appHomePath + "/").toStdString().c_str()); // must copy thus file in sys are update too fast to read? //
        QFile clocks(appHomePath + "/radeon_pm_info");

        if (clocks.open(QIODevice::ReadOnly)) {
            if (powerMethod == "profile") {
                short i=0;

                while (!clocks.atEnd()) {
                    QString s = clocks.readLine(100);

                    switch (i) {
                        case 1: {
                            if (s.contains("current engine clock")) {
                                coreClock = QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                                gpuData << "Current GPU clock: " + QString().setNum(coreClock) + " MHz";
                                break;
                            }
                        };
                        case 3: {
                            if (s.contains("current memory clock")) {
                                memClock = QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                                gpuData << "Current mem clock: " + QString().setNum(memClock) + " MHz";
                                break;
                            }
                        }
                        case 4: {
                            if (s.contains("voltage")) {
                                gpuData << "-------------------------";
                                voltsGPU = QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toDouble();
                                gpuData << "Voltage: " + QString().setNum(voltsGPU) + " mV";
                                break;
                            }
                        }
                    }
                    i++;
                }
            }
            else if (powerMethod == "dpm") {
                QString data[2];
                short i=0;

                while (!clocks.atEnd()) {
                    data[i] = clocks.readLine(500);
                    i++;
                }

                QRegExp rx;

                rx.setPattern("sclk:\\s\\d+");
                rx.indexIn(data[1]);
                if (!rx.cap(0).isEmpty()) {
                    coreClock = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                    gpuData << "Current GPU clock: " + QString().setNum(coreClock) + " MHz";
                }

                rx.setPattern("mclk:\\s\\d+");
                rx.indexIn(data[1]);
                if (!rx.cap(0).isEmpty()) {
                    memClock = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                    gpuData << "Current mem clock: " + QString().setNum(memClock) + " MHz";
                }

                rx.setPattern("vclk:\\s\\d+");
                rx.indexIn(data[0]);
                if (!rx.cap(0).isEmpty()) {
                    uvdvclk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                    if (uvdvclk != 0)
                        gpuData << "UVD clock (vclk): " + QString().setNum(uvdvclk) + " MHz";
                }

                rx.setPattern("dclk:\\s\\d+");
                rx.indexIn(data[0]);
                if (!rx.cap(0).isEmpty()) {
                    uvddclk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
                    if (uvddclk != 0)
                        gpuData << "UVD clock (dclk): " + QString().setNum(uvddclk) + " MHz";
                }

                rx.setPattern("vddc:\\s\\d+");
                rx.indexIn(data[1]);
                if (!rx.cap(0).isEmpty()) {
                    voltsGPU = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();
                    gpuData << "------------------------";
                    gpuData << "Voltage (vddc): " + QString().setNum(voltsGPU) + " mV";
                }

                rx.setPattern("vddci:\\s\\d+");
                rx.indexIn(data[1]);
                if (!rx.cap(0).isEmpty()) {
                    gpuData << "Voltage (vddci): " + rx.cap(0).split(' ',QString::SkipEmptyParts)[1] + " mV";
                }
            }
            else {
                gpuData << "Current GPU clock: " + radeon_profile::err;
                gpuData << "Current mem clock: " + radeon_profile::err;
                gpuData << "Voltage: "+radeon_profile::err;
                ui->cb_showFreqGraph->setChecked(false),ui->cb_showFreqGraph->setEnabled(false),ui->cb_showVoltsGraph->setEnabled(false),
                        ui->plotColcks->setVisible(false),ui->plotVolts->setVisible(false);
            }
        }
        else {
            gpuData << "Current GPU clock: " + radeon_profile::noValues;
            gpuData << "Current mem clock: " + radeon_profile::noValues;
            gpuData << "------------------------";
            gpuData << "Voltage: "+ radeon_profile::noValues;
            ui->cb_showFreqGraph->setChecked(false),ui->cb_showFreqGraph->setEnabled(false),ui->cb_showVoltsGraph->setEnabled(false),
                    ui->plotColcks->setVisible(false),ui->plotVolts->setVisible(false);
        }
        gpuData << "------------------------";

        clocks.close();

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

        return gpuData;
    }
    else {
        gpuData << "Current GPU clock: " + radeon_profile::noValues + " (root rights? debugfs mounted?)";
        gpuData << "Current mem clock: " + radeon_profile::noValues + " (root rights? debugfs mounted?)";
        gpuData << "------------------------";
        gpuData << "Voltage: "+ radeon_profile::noValues + " (root rights? debugfs mounted?)";
        gpuData << "------------------------";
        ui->cb_showFreqGraph->setChecked(false),ui->cb_showFreqGraph->setEnabled(false),ui->cb_showVoltsGraph->setEnabled(false),
                ui->plotColcks->setVisible(false),ui->plotVolts->setVisible(false);
        return gpuData;
    }
}

QString radeon_profile::getCurrentPowerProfile(const QString filePath) {
    QFile profile(filePath);
    QFile forceProfile(forcePowerLevelFile);
    QString pp;

    if (profile.exists()) {
        if (profile.open(QIODevice::ReadOnly)) {
            pp = profile.readLine(13);
            if (forceProfile.open(QIODevice::ReadOnly))
                pp += " | " + forceProfile.readLine(5);
        } else
            pp = radeon_profile::err;
    } else
       pp = radeon_profile::noValues;

    return pp.remove('\n');
}

void radeon_profile::testSensor() {
    system(QString("sensors | grep radeon-pci > "+ appHomePath + "/vgatemp").toStdString().c_str());
    QFile gpuTempFile(appHomePath + "/vgatemp");

    if (gpuTempFile.open(QIODevice::ReadOnly)) {
        if (!gpuTempFile.readLine(50).isEmpty())
            pciSensor = true;
        else
            pciSensor = false;
    }
    gpuTempFile.close();
}

QString radeon_profile::getGPUTemp() {
    if (pciSensor)
        system(QString("sensors | grep radeon-pci -A 2 | grep temp  > "+ appHomePath + "/vgatemp").toStdString().c_str());
    else
        system(QString("sensors | grep VGA  > "+ appHomePath + "/vgatemp").toStdString().c_str());

    QFile gpuTempFile(appHomePath + "/vgatemp");
    if (gpuTempFile.open(QIODevice::ReadOnly)) {
        QString temp = gpuTempFile.readLine(50);
        gpuTempFile.close();
        if (!temp.isEmpty()) {
            temp = temp.split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°"); 
            current = temp.toDouble();
            tempSum += current;

            if (minT == 0)
                minT = current;
            maxT = (current >= maxT) ? current : maxT;
            minT = (current <= minT) ? current : minT;

            if (maxT >= ui->plotTemp->yAxis->range().upper || minT <= ui->plotTemp->yAxis->range().lower)
                ui->plotTemp->yAxis->setRange(minT - 5, maxT + 5);

            ui->plotTemp->graph(0)->addData(i,current);
            ui->l_minMaxTemp->setText("Now: " + QString().setNum(current) + "C | Max: " + QString().setNum(maxT) + "C | Min: " + QString().setNum(minT) + "C | Avg: " + QString().setNum(tempSum/i,'f',1));
            return "Current GPU temp: "+temp+"C";
        } else {
            ui->plotTemp->setVisible(false),ui->cb_showTempsGraph->setEnabled(false),
                    ui->cb_showTempsGraph->setChecked(false),ui->l_minMaxTemp->setVisible(false);
            return "";
        }
    } else return "";
}

QStringList radeon_profile::fillGpuDataTable(const QString profile) {
    ui->list_currentGPUData->clear();
    QStringList completeData;
    completeData << getClocks(profile);
    completeData << getGPUTemp();
    return completeData;
}

void radeon_profile::on_btn_dpmBattery_clicked() {
    setValueToFile(dpmStateFile,"battery");
}

void radeon_profile::on_btn_dpmBalanced_clicked() {
    setValueToFile(dpmStateFile,"balanced");
}

void radeon_profile::on_btn_dpmPerformance_clicked() {
    setValueToFile(dpmStateFile,"performance");
}

void radeon_profile::setupGraphs()
{
    ui->plotColcks->setBackground(Qt::darkGray);
    ui->plotTemp->setBackground(Qt::darkGray);
    ui->plotVolts->setBackground(Qt::darkGray);

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

    QPen pen;
    pen.setWidth(2);
    pen.setCapStyle(Qt::SquareCap);
    pen.setColor(Qt::yellow);
    ui->plotTemp->graph(0)->setPen(pen);
    pen.setColor(Qt::black);
    ui->plotColcks->graph(1)->setPen(pen);
    pen.setColor(Qt::cyan);
    ui->plotColcks->graph(0)->setPen(pen);
    pen.setColor(Qt::red);
    ui->plotColcks->graph(2)->setPen(pen);
    pen.setColor(Qt::green);
    ui->plotColcks->graph(3)->setPen(pen);
    pen.setColor(Qt::blue);
    ui->plotVolts->graph(0)->setPen(pen);

    // legend clocks //
    ui->plotColcks->graph(0)->setName("GPU clock");
    ui->plotColcks->graph(1)->setName("Memory clock");
    ui->plotColcks->graph(2)->setName("UVD (vclk)");
    ui->plotColcks->graph(3)->setName("UVD (dclk)");
    ui->plotColcks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotColcks->legend->setVisible(true);

    ui->plotColcks->replot();
    ui->plotTemp->replot();
    ui->plotVolts->replot();
}

QStringList radeon_profile::getGLXInfo() {
    QStringList data;
    QFile gpuInfo(appHomePath + "/gpuinfo");

    system(QString("lspci | grep VGA > "+ appHomePath + "/gpuinfo").toStdString().c_str());
    system(QString("xdriinfo >>"+ appHomePath + "/gpuinfo").toStdString().c_str());
    system(QString("glxinfo | grep direct >> "+ appHomePath + "/gpuinfo").toStdString().c_str());
    system(QString("glxinfo | grep OpenGL >>"+ appHomePath + "/gpuinfo").toStdString().c_str());

    if (gpuInfo.open(QIODevice::ReadOnly)) {
        while (!gpuInfo.atEnd()) {
            QString s;
            s = gpuInfo.readLine(300);

            if (s.contains("VGA")) {
                QString tmps = s.split(":",QString::SkipEmptyParts)[2];
                data.append("VGA:"+tmps.left(tmps.indexOf('\n')));
            }
            if (s.contains("Screen 0:"))
                data.append("Driver:"+s.left(s.indexOf('\n')).split(':',QString::SkipEmptyParts)[1]);
            if (s.contains("direct rendering"))
                data.append(s.left(s.indexOf('\n')));
            if (s.contains("OpenGL renderer string"))
                data.append(s.left(s.indexOf('\n')));
            if (s.contains("OpenGL core profile version string:"))
                data.append(s.left(s.indexOf('\n')));
            if (s.contains("OpenGL core profile shading language version string"))
                data.append(s.left(s.indexOf('\n')));
            if (s.contains("OpenGL version string"))
                data.append(s.left(s.indexOf('\n')));
        }
    }
    gpuInfo.close();
    return data;
}

void radeon_profile::changeTimeRange() {
    rangeX = ui->timeSlider->value();
}

void radeon_profile::setupTrayIcon() {
    trayMenu = new QMenu();

    //close //
    closeApp = new QAction(this);
    closeApp->setText("Quit");
    connect(closeApp,SIGNAL(triggered()),this,SLOT(close()));

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
    dpmSetBattery->setIcon(QIcon(":/icon/arrow1.png"));
    dpmSetBalanced->setText("Balanced");
    dpmSetBalanced->setIcon(QIcon(":/icon/arrow2.png"));
    dpmSetPerformance->setText("Performance");
    dpmSetPerformance->setIcon(QIcon(":/icon/arrow3.png"));

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
    QIcon appicon(":/icon/icon.png");
    trayIcon = new QSystemTrayIcon(appicon,this);
    trayIcon->show();
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

void radeon_profile::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        if (isHidden()) show(); else hide();
            break;
    }
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

void radeon_profile::resetGraphs() {
        ui->plotColcks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
        ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);
        ui->plotTemp->yAxis->setRange(10,20);
}

void radeon_profile::showLegend(bool checked) {
        ui->plotColcks->legend->setVisible(checked);
        ui->plotColcks->replot();
}

void radeon_profile::setupForcePowerLevelMenu() {
    forcePowerMenu = new QMenu(this);
    ui->btn_forcePowerLevel->setMenu(forcePowerMenu);

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

void radeon_profile::forceAuto() {
    setValueToFile(forcePowerLevelFile,"auto");
}

void radeon_profile::forceLow() {
    setValueToFile(forcePowerLevelFile,"low");
}

void radeon_profile::forceHigh() {
    setValueToFile(forcePowerLevelFile,"high");
}

void radeon_profile::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange) {
        if(isMinimized())
            this->hide();

        event->ignore();
    }
}

void radeon_profile::getModuleInfo() {
    system(QString("modinfo -p radeon | sort >" + appHomePath + "/moddesc").toStdString().c_str());
    system(QString("grep . /sys/module/radeon/parameters/* >" + appHomePath + "/modsettings").toStdString().c_str());
    QFile modDescFile(appHomePath + "/moddesc");
    QFile modSettingsFile(appHomePath + "/modsettings");

    if (modDescFile.open(QIODevice::ReadOnly) && modSettingsFile.open(QIODevice::ReadOnly)) {
        while (!modDescFile.atEnd() && !modSettingsFile.atEnd()) {
            QString mdl = modDescFile.readLine(500);
            QString mds = modSettingsFile.readLine(500);

            if (mdl.contains(":") && mds.contains(":")) {
                QString modName = mdl.split(":",QString::SkipEmptyParts)[0];
                QString modDesc = mdl.split(":",QString::SkipEmptyParts)[1];
                QString modValue = mds.split(":",QString::SkipEmptyParts)[1];

                QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.left(modName.indexOf('\n')) << modValue.left(modValue.indexOf('\n')) << modDesc.left(modDesc.indexOf('\n')));
                ui->list_modInfo->addTopLevelItem(item);
            }
        }
        modDescFile.close();
        modSettingsFile.close();
    }
}
