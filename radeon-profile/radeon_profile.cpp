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

radeon_profile::radeon_profile(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);

    appHomePath = QDir::homePath() + "/.radeon-profile";
    QDir appHome = appHomePath;
    if (!appHome.exists())
        appHome.mkdir(QDir::homePath() + "/.radeon-profile");

    QTimer *timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(timerEvent()));

    timer->start(1000);
    timerEvent();
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
                                gpuData << "Current GPU clock: " + QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz";
                                break;
                            }
                        };
                        case 3: {
                            if (s.contains("current memory clock")) {
                                gpuData << "Current mem clock: " +QString().setNum(s.split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz";
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
                if (data[1].contains("sclk"))
                    gpuData << "Current GPU clock: " + QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[4].toFloat() / 100) + " MHz";
                if (data[1].contains("mclk"))
                    gpuData << "Current mem clock: " +QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[6].toFloat() / 100) + " MHz";
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
    system(QString("sensors | grep VGA | cut -b 19-25 > "+ appHomePath + "/vgatemp").toStdString().c_str());
    QFile gpuTempFile(appHomePath + "/vgatemp");
    if (gpuTempFile.open(QIODevice::ReadOnly))
       return "Current GPU temp: "+gpuTempFile.readLine(8);
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

void radeon_profile::on_pushButton_clicked()
{
    setProfile(dpmState,"battery");
}

void radeon_profile::on_pushButton_2_clicked()
{
    setProfile(dpmState,"balanced");
}

void radeon_profile::on_pushButton_3_clicked()
{
    setProfile(dpmState,"performance");
}
