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
    setupGraphs();
    setupTrayIcon();

    QTimer *timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));
    connect(ui->timeSlider,SIGNAL(valueChanged(int)),this,SLOT(changeRange()));
    timer->start(1000);
    timerEvent();

    if (ui->stdProfiles->isEnabled())
        dpmMenu->setEnabled(false);
    if (ui->dpmProfiles->isEnabled())
        changeProfile->setEnabled(false);
}

radeon_profile::~radeon_profile()
{
    delete ui;
}

void radeon_profile::setProfile(const QString filePath, const QStringList profiles) {
    bool ok;
    QString newProfile = QInputDialog::getItem(this, "Select new Radeon profile (ONLY ROOT)", "Profile",profiles,0,false,&ok);

    if (ok) {
        system(QString("echo "+ newProfile + " > "+ filePath ).toStdString().c_str());
    }
}

void radeon_profile::setProfile(const QString filePath, const QString newProfile) {
    system(QString("echo "+ newProfile + " > "+ filePath ).toStdString().c_str());
}

void radeon_profile::on_chProfile_clicked() {
    setProfile(radeon_profile::profilePath, QStringList() << "default" << "auto" << "low" << "mid" << "high");
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

        ui->l_profile->setText(getCurrentPowerProfile(dpmState));
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

    //tray icon tooltip
    QString tooltipdata = "Current profile: "+ui->l_profile->text()+ '\n';
    for (short i = 0; i < ui->list_currentGPUData->count(); i++) {
        tooltipdata += ui->list_currentGPUData->item(i)->text() + '\n';
    }
    trayIcon->setToolTip(tooltipdata);
}

QString radeon_profile::getPowerMethod() {
    QFile powerMethodFile(radeon_profile::powerMethod);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString pMethod = powerMethodFile.readLine(20);
        return pMethod;
    } else
        return QString::null;
}

QStringList radeon_profile::getClocks(const QString powerMethod) {
    QStringList gpuData;
    double coreClock = 0,memClock= 0;  // for plots

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
                                gpuData << "Voltage: " + QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()) + " mV";
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
                if (data[1].contains("sclk")) {
                    coreClock = QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[4].toFloat() / 100).toDouble();
                    gpuData << "Current GPU clock: " + QString().setNum(coreClock) + " MHz";
                }
                if (data[1].contains("mclk")) {
                    memClock = QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[6].toFloat() / 100).toDouble();
                    gpuData << "Current mem clock: " + QString().setNum(memClock) + " MHz";
                }

                if (data[1].contains("vddc")) {
                    gpuData << "------------------------";
                    gpuData << "Voltage (vddc): " +QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[8].toFloat()) + " mV";
                }
                if (data[1].contains("vddci"))
                    gpuData << "Voltage (vddci): " +QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[10].toFloat()) + " mV";
            }
            else {
                gpuData << "Current GPU clock: " + radeon_profile::err;
                gpuData << "Current mem clock: " + radeon_profile::err;
                gpuData << "Voltage: "+radeon_profile::err;
            }
        }
        else {
            gpuData << "Current GPU clock: " + radeon_profile::noValues;
            gpuData << "Current mem clock: " + radeon_profile::noValues;
            gpuData << "------------------------";
            gpuData << "Voltage: "+ radeon_profile::noValues;
        }
        gpuData << "------------------------";

        clocks.close();

        // update plots
        if (memClock > ui->plotColcks->yAxis->range().upper)
            ui->plotColcks->yAxis->setRange(0,memClock + 100);

        ui->plotColcks->graph(0)->addData(i,coreClock);
        ui->plotColcks->graph(1)->addData(i,memClock);

        return gpuData;
    }
    else {
        gpuData << "Current GPU clock: " + radeon_profile::noValues;
        gpuData << "Current mem clock: " + radeon_profile::noValues;
        gpuData << "------------------------";
        gpuData << "Voltage: "+ radeon_profile::noValues;
        gpuData << "------------------------";
        return gpuData;
    }
}

QString radeon_profile::getCurrentPowerProfile(const QString filePath) {

    QFile profile(filePath);

    if (profile.exists()) {
        if (profile.open(QIODevice::ReadOnly)) {
            return profile.readLine(13);
        } else
            return radeon_profile::err;
    } else
       return radeon_profile::noValues;
}

QString radeon_profile::getGPUTemp() {
    system(QString("sensors | grep VGA > "+ appHomePath + "/vgatemp").toStdString().c_str());
    QFile gpuTempFile(appHomePath + "/vgatemp");
    if (gpuTempFile.open(QIODevice::ReadOnly)) {
        QString temp = gpuTempFile.readLine(50);
        temp = temp.split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        current = temp.toDouble();

        if (minT == 0)
            minT = current;
        maxT = (current >= maxT) ? current : maxT;
        minT = (current <= minT) ? current : minT;

        if (maxT > ui->plotTemp->yAxis->range().upper)
            ui->plotTemp->yAxis->setRange(minT - 10,maxT+10);
        ui->plotTemp->graph(0)->addData(i,current);
        ui->l_minMaxTemp->setText("Current: " + QString().setNum(current) + "C | Max: " + QString().setNum(maxT) + "C | Min: " + QString().setNum(minT) +"C");
        return "Current GPU temp: "+temp+"C";
    }
    else
       return "No temps.";
}

QStringList radeon_profile::fillGpuDataTable(const QString profile) {
    ui->list_currentGPUData->clear();
    QStringList completeData;
    completeData << getClocks(profile);
    completeData << getGPUTemp();
    return completeData;
}

void radeon_profile::on_btn_dpmBattery_clicked() {
    setProfile(dpmState,"battery");
}

void radeon_profile::on_btn_dpmBalanced_clicked() {
    setProfile(dpmState,"balanced");
}

void radeon_profile::on_btn_dpmPerformance_clicked() {
    setProfile(dpmState,"performance");
}

void radeon_profile::setupGraphs()
{
    ui->plotColcks->setBackground(Qt::darkGray);
    ui->plotTemp->setBackground(Qt::darkGray);

    ui->plotTemp->yAxis->setRange(0,20);
    ui->plotColcks->yAxis->setRange(0,100);

    ui->plotTemp->xAxis->setLabel("time");
    ui->plotTemp->yAxis->setLabel("temperature");
    ui->plotColcks->xAxis->setLabel("time");
    ui->plotColcks->yAxis->setLabel("MHz");

    ui->plotTemp->addGraph(); // temp graph
    ui->plotColcks->addGraph(); // core clock graph
    ui->plotColcks->addGraph(); // mem clock graph

    ui->plotTemp->graph(0)->setPen(QPen(Qt::yellow));
    ui->plotColcks->graph(1)->setPen(QPen(Qt::black));
    ui->plotColcks->graph(0)->setPen(QPen(Qt::white));

    // legend //
    ui->plotColcks->graph(0)->setName("GPU clock");
    ui->plotColcks->graph(1)->setName("Memory clock");
    ui->plotColcks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotColcks->legend->setVisible(true);

    ui->plotColcks->replot();
    ui->plotTemp->replot();
}


QStringList radeon_profile::getGLXInfo() {
    QStringList data;
    QFile gpuInfo(appHomePath + "/gpuinfo");

    system(QString("lspci | grep VGA > "+ appHomePath + "/gpuinfo").toStdString().c_str());
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

    return data;
}

void radeon_profile::changeRange() {
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
    dpmSetBalanced->setText("Balanced");
    dpmSetPerformance->setText("Performance");

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(on_btn_dpmBattery_clicked()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(on_btn_dpmBalanced_clicked()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(on_btn_dpmPerformance_clicked()));

    dpmMenu->addAction(dpmSetBattery);
    dpmMenu->addAction(dpmSetBalanced);
    dpmMenu->addAction(dpmSetPerformance);

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
