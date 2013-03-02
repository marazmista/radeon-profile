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

void radeon_profile::on_chProfile_clicked()
{
    bool ok;
    QString newProfile = QInputDialog::getItem(this,
                                            "Select new Radeon profile (ONLY ROOT)",
                                            "Profile",
                                            QStringList() << "default" << "auto" << "low" << "mid" << "high",
                                            0,
                                            false,
                                            &ok);

    if (ok && !newProfile.isEmpty()) {
        system(QString("echo "+ newProfile + " >> "+ radeon_profile::profilePath).toStdString().c_str());
    }
}

void radeon_profile::timerEvent() {
    system(QString("cp " + radeon_profile::clocksPath + " " + QApplication::applicationDirPath() + "/").toStdString().c_str()); // must copy thus file in sys are update too fast to read? //

    QFile clocks(QApplication::applicationDirPath() + "/radeon_pm_info");
    QFile profile(radeon_profile::profilePath);

    if (clocks.exists()) {
        if (clocks.open(QIODevice::ReadOnly)) {
            QString data[6];
            short i=0;

            while (!clocks.atEnd()) {
                data[i] = clocks.readLine(100);
                i++;
            }

            ui->currentGPU->setText(QString().setNum(data[1].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz");
            ui->currentMEM->setText(QString().setNum(data[3].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000) + " MHz");
            ui->currentVolts->setText(QString().setNum(data[4].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()) + " mV");
        } else {
            ui->currentGPU->setText(radeon_profile::err);
            ui->currentMEM->setText(radeon_profile::err);
            ui->currentVolts->setText(radeon_profile::err);
        }
    } else {
        ui->currentGPU->setText(radeon_profile::noValues);
        ui->currentMEM->setText(radeon_profile::noValues);
        ui->currentVolts->setText(radeon_profile::noValues);
    }

    if (profile.exists()) {
        if (profile.open(QIODevice::ReadOnly)) {
            ui->currentProfile->setText(profile.readLine(10));
        } else
            ui->currentProfile->setText(radeon_profile::err);
    } else
        ui->currentProfile->setText(radeon_profile::noValues);

    clocks.remove();
    clocks.close();
    profile.close();


}
