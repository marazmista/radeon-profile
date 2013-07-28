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
#include <QMessageBox>

radeon_profile::radeon_profile(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::radeon_profile)
{
    ui->setupUi(this);

    QTimer *timer = new QTimer(this);
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
    QString newProfile = QInputDialog::getItem(this,
                                            "Select new Radeon profile (ONLY ROOT)",
                                            "Profile",
                                            profiles,
                                            0,
                                            false,
                                            &ok);

    if (ok) {
        system(QString("echo "+ newProfile + " >> "+ filePath ).toStdString().c_str());
    }
}

void radeon_profile::on_chProfile_clicked() {
    setProfile(radeon_profile::profilePath, QStringList() << "default" << "auto" << "low" << "mid" << "high");
}

void radeon_profile::on_btn_chDpmProfile_clicked()
{
    setProfile(radeon_profile::dpmState, QStringList() << "balanced" << "battery" << "performance");
}


void radeon_profile::timerEvent() {

    if (getPowerMethod().contains("profile",Qt::CaseInsensitive)) {
        ui->tabWidget->setCurrentIndex(0);
        ui->tabWidget->setTabEnabled(1,false);
        ui->currentProfile->setText(getCurrentPowerProfile(profilePath));
        getClocks("profile");

    } else if (getPowerMethod().contains("dpm",Qt::CaseInsensitive)) {
        ui->tabWidget->setCurrentIndex(1);
        ui->tabWidget->setTabEnabled(0,false);
        ui->currentDpmProfile->setText(getCurrentPowerProfile(dpmState));
        getClocks("dpm");

    } else {
        ui->currentGPU->setText(radeon_profile::err);
        ui->currentMEM->setText(radeon_profile::err);
        ui->currentVolts->setText(radeon_profile::err);
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

void radeon_profile::getClocks(const QString powerMethod) {

    system(QString("cp " + radeon_profile::clocksPath + " " + QApplication::applicationDirPath() + "/").toStdString().c_str()); // must copy thus file in sys are update too fast to read? //
    QFile clocks(QApplication::applicationDirPath() + "/radeon_pm_info");

    if (clocks.exists()) {
        if (clocks.open(QIODevice::ReadOnly)) {
            if (powerMethod == "profile") {

                QString data[6];
                short i=0;

                while (!clocks.atEnd()) {
                    data[i] = clocks.readLine(100);
                    i++;
                }

                ui->currentGPU->setText(QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz");
                ui->currentMEM->setText(QString().setNum(data[3].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz");
                ui->currentVolts->setText(QString().setNum(data[4].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()) + " mV");
            }
            else if (powerMethod == "dpm") {
                QString data[2];
                short i=0;

                while (!clocks.atEnd()) {
                    data[i] = clocks.readLine(500);
                    i++;
                }

                ui->currentGPU->setText(QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[4].toFloat() / 100) + " MHz");
                ui->currentMEM->setText(QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[6].toFloat() / 100) + " MHz");
                ui->currentVolts->setText(QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[8].toFloat()) + " mV");
            }

            else {
                ui->currentGPU->setText(radeon_profile::err);
                ui->currentMEM->setText(radeon_profile::err);
                ui->currentVolts->setText(radeon_profile::err);
            }
        }
        else {
            ui->currentGPU->setText(radeon_profile::noValues);
            ui->currentMEM->setText(radeon_profile::noValues);
            ui->currentVolts->setText(radeon_profile::noValues);
        }

        ui->currentProfile->setText(getCurrentPowerProfile(radeon_profile::profilePath));

        clocks.remove();
        clocks.close();
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
