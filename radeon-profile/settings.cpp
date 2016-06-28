// copyright marazmista @ 30.03.2014

// there is a settings handling, save and load from ini //

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include <QSettings>
#include <QMenu>
#include <QDir>
#include <QTreeWidgetItem>
#include <QDesktopWidget>
#include <QRect>

const QString radeon_profile::settingsPath = QDir::homePath() + "/.radeon-profile-settings";
const QString execProfilesPath = QDir::homePath() + "/.radeon-profile-execProfiles";
const QString fanStepsPath = QDir::homePath() + "/.radeon-profile-fanSteps";

// init of static struct with setting exposed to global scope
globalStuff::globalCfgStruct globalStuff::globalConfig;

void radeon_profile::saveConfig() {
    qDebug() << "Saving config to" << radeon_profile::settingsPath;
    QSettings settings(radeon_profile::settingsPath,QSettings::IniFormat);

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
    settings.setValue("powerLevelStatistics", ui->cb_stats->isChecked());
    settings.setValue("aleternateRowColors",ui->cb_alternateRow->isChecked());

    settings.setValue("showLegend",optionsMenu.actions().at(0)->isChecked());
    settings.setValue("graphOffset",optionsMenu.actions().at(1)->isChecked());
    settings.setValue("graphRange",ui->timeSlider->value());
    settings.setValue("daemonAutoRefresh",ui->cb_daemonAutoRefresh->isChecked());
    settings.setValue("fanSpeedSlider",ui->fanSpeedSlider->value());
    settings.setValue("showAlwaysGpuSelector", ui->cb_showAlwaysGpuSelector->isChecked());
    settings.setValue("showCombo", ui->cb_showCombo->isChecked());

    settings.setValue("overclockEnabled", ui->cb_enableOverclock->isChecked());
    settings.setValue("overclockAtLaunch", ui->cb_overclockAtLaunch->isChecked());
    settings.setValue("GPUoverclockValue", ui->slider_GPUoverclock->value());
    settings.setValue("memoryOverclockValue", ui->slider_memoryOverclock->value());

    // Graph settings
    settings.setValue("graphLineThickness",ui->spin_lineThick->value());
    settings.setValue("graphTempBackground",ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1));
    settings.setValue("graphClocksBackground",ui->graphColorsList->topLevelItem(CLOCKS_BG)->backgroundColor(1));
    settings.setValue("graphVoltsBackground",ui->graphColorsList->topLevelItem(VOLTS_BG)->backgroundColor(1));
    settings.setValue("graphTempLine",ui->graphColorsList->topLevelItem(TEMP_LINE)->backgroundColor(1));
    settings.setValue("graphGPUClockLine",ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->backgroundColor(1));
    settings.setValue("graphMemClockLine",ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->backgroundColor(1));
    settings.setValue("graphUVDVideoLine",ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->backgroundColor(1));
    settings.setValue("graphUVDDecoderLine",ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->backgroundColor(1));
    settings.setValue("graphVoltsLine",ui->graphColorsList->topLevelItem(CORE_VOLTS_LINE)->backgroundColor(1));
    settings.setValue("graphMemVoltsLine",ui->graphColorsList->topLevelItem(MEM_VOLTS_LINE)->backgroundColor(1));

    settings.setValue("showTempGraphOnStart",ui->cb_showTempsGraph->isChecked());
    settings.setValue("showFreqGraphOnStart",ui->cb_showFreqGraph->isChecked());
    settings.setValue("showVoltsGraphOnStart",ui->cb_showVoltsGraph->isChecked());
    settings.setValue("execDbcAction",ui->cb_execDbcAction->currentIndex());
    settings.setValue("appendSysEnv",ui->cb_execSysEnv->isChecked());

    // save profiles from Exec tab
    QFile ef(execProfilesPath);
    if (ef.open(QIODevice::WriteOnly)) {
        for (int i = 0; i < ui->list_execProfiles->topLevelItemCount(); i++) {
            QString profile = ui->list_execProfiles->topLevelItem(i)->text(PROFILE_NAME) + "###" +
                    ui->list_execProfiles->topLevelItem(i)->text(BINARY) + "###" +
                    ui->list_execProfiles->topLevelItem(i)->text(BINARY_PARAMS) + "###" +
                    ui->list_execProfiles->topLevelItem(i)->text(ENV_SETTINGS) + "###" +
                    ui->list_execProfiles->topLevelItem(i)->text(LOG_FILE) + "###" +
                    ui->list_execProfiles->topLevelItem(i)->text(LOG_FILE_DATE_APPEND) + "\n";

            ef.write(profile.toLatin1());
        }
        ef.close();
    }
}

void radeon_profile::loadConfig() {
    qDebug() << "Loading config from" << radeon_profile::settingsPath;
    QSettings settings(radeon_profile::settingsPath,QSettings::IniFormat);

    ui->cb_startMinimized->setChecked(settings.value("startMinimized",false).toBool());
    ui->cb_minimizeTray->setChecked(settings.value("minimizeToTray",false).toBool());
    ui->cb_closeTray->setChecked(settings.value("closeToTray",false).toBool());
    ui->spin_timerInterval->setValue(settings.value("updateInterval",1).toInt());
    ui->cb_gpuData->setChecked(settings.value("updateGPUData",true).toBool());
    ui->cb_graphs->setChecked(settings.value("updateGraphs",true).toBool());
    ui->cb_glxInfo->setChecked(settings.value("updateGLXInfo",false).toBool());
    ui->cb_connectors->setChecked(settings.value("updateConnectors",false).toBool());
    ui->cb_modParams->setChecked(settings.value("updateModParams",false).toBool());
    ui->cb_saveWindowGeometry->setChecked(settings.value("saveWindowGeometry").toBool());
    ui->cb_stats->setChecked(settings.value("powerLevelStatistics",true).toBool());
    ui->cb_alternateRow->setChecked(settings.value("aleternateRowColors",true).toBool());
    ui->cb_daemonAutoRefresh->setChecked(settings.value("daemonAutoRefresh",true).toBool());
    ui->cb_execDbcAction->setCurrentIndex(settings.value("execDbcAction",0).toInt());
    ui->fanSpeedSlider->setValue(settings.value("fanSpeedSlider",80).toInt());
    ui->cb_showAlwaysGpuSelector->setChecked(settings.value("showAlwaysGpuSelector",true).toBool());
    ui->cb_showCombo->setChecked(settings.value("showCombo",true).toBool());

    optionsMenu.actions().at(0)->setChecked(settings.value("showLegend",true).toBool());
    optionsMenu.actions().at(1)->setChecked(settings.value("graphOffset",true).toBool());
    ui->timeSlider->setValue(settings.value("graphRange",180).toInt());
    // Graphs settings
    ui->spin_lineThick->setValue(settings.value("graphLineThickness",2).toInt());
    // detalis: http://qt-project.org/doc/qt-4.8/qvariant.html#a-note-on-gui-types
    //ok, color is saved as QVariant, and read and convertsion it to QColor is below
    ui->graphColorsList->topLevelItem(TEMP_BG)->setBackgroundColor(1,settings.value("graphTempBackground",QColor(Qt::darkGray)).value<QColor>());
    ui->graphColorsList->topLevelItem(CLOCKS_BG)->setBackgroundColor(1,settings.value("graphClocksBackground",QColor(Qt::darkGray)).value<QColor>());
    ui->graphColorsList->topLevelItem(VOLTS_BG)->setBackgroundColor(1,settings.value("graphVoltsBackground",QColor(Qt::darkGray)).value<QColor>());
    ui->graphColorsList->topLevelItem(TEMP_LINE)->setBackgroundColor(1,settings.value("graphTempLine",QColor(Qt::yellow)).value<QColor>());
    ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphGPUClockLine",QColor(Qt::black)).value<QColor>());
    ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphMemClockLine",QColor(Qt::cyan)).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->setBackgroundColor(1,settings.value("graphUVDVideoLine",QColor(Qt::red)).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->setBackgroundColor(1,settings.value("graphUVDDecoderLine",QColor(Qt::green)).value<QColor>());
    ui->graphColorsList->topLevelItem(CORE_VOLTS_LINE)->setBackgroundColor(1,settings.value("graphVoltsLine",QColor(Qt::blue)).value<QColor>());
    ui->graphColorsList->topLevelItem(MEM_VOLTS_LINE)->setBackgroundColor(1,settings.value("graphMemVoltsLine",QColor(Qt::cyan)).value<QColor>());
    setupGraphsStyle();

    ui->cb_showTempsGraph->setChecked(settings.value("showTempGraphOnStart",true).toBool());
    ui->cb_showFreqGraph->setChecked(settings.value("showFreqGraphOnStart",true).toBool());
    ui->cb_showVoltsGraph->setChecked(settings.value("showVoltsGraphOnStart",false).toBool());
    ui->cb_execSysEnv->setChecked(settings.value("appendSysEnv",true).toBool());

    ui->cb_enableOverclock->setChecked(settings.value("overclockEnabled",false).toBool());
    ui->cb_overclockAtLaunch->setChecked(settings.value("overclockAtLaunch",false).toBool());
    ui->slider_GPUoverclock->setValue(settings.value("GPUoverclockValue",0).toInt());
    ui->slider_memoryOverclock->setValue(settings.value("memoryOverclockValue",0).toInt());

    // apply some settings to ui on start //
    if (ui->cb_saveWindowGeometry->isChecked())
        this->setGeometry(settings.value("windowGeometry").toRect());
    else {
        // Set up the size at 1/2 of the screen size, centered
        const QRect desktopSize = QDesktopWidget().availableGeometry(this);
        this->setGeometry(desktopSize.width() / 4, // Left offset
                         desktopSize.height() / 4, // Top offset
                         desktopSize.width() / 2, // Width
                         desktopSize.height() / 2); // Height
    }



    ui->list_currentGPUData->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_glxinfo->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_modInfo->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_connectors->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_stats->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_execProfiles->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_variables->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_vaules->setAlternatingRowColors(ui->cb_alternateRow->isChecked());

    showLegend(optionsMenu.actions().at(0)->isChecked());
    ui->cb_showTempsGraph->setChecked(ui->cb_showTempsGraph->isChecked());
    ui->cb_showFreqGraph->setChecked(ui->cb_showFreqGraph->isChecked());
    ui->cb_showVoltsGraph->setChecked(ui->cb_showVoltsGraph->isChecked());

    globalStuff::globalConfig.interval = static_cast<ushort>(ui->spin_timerInterval->value());
    globalStuff::globalConfig.daemonAutoRefresh = ui->cb_daemonAutoRefresh->isChecked();
    graphOffset = ((optionsMenu.actions().at(1)->isChecked()) ? 20 : 0);

    QFile ef(execProfilesPath);
    if (ef.open(QIODevice::ReadOnly)) {
        QStringList profiles = QString(ef.readAll()).split('\n');

        for (int i=0;i <profiles.count(); i++) {
            if (!profiles[i].isEmpty())
                ui->list_execProfiles->addTopLevelItem(new QTreeWidgetItem(QStringList() << profiles[i].split("###")));
        }

    }
}

void radeon_profile::loadFanProfiles() {
    qDebug() << "Loading fan profiles from" << fanStepsPath;
    QFile fsPath(fanStepsPath);
    if (fsPath.open(QIODevice::ReadOnly)) {
        const QStringList steps = QString(fsPath.readAll()).split("|",QString::SkipEmptyParts);

        if (steps.count() > 1) {
            for (int i = 1; i < steps.count(); ++i) {
                QStringList pair = steps[i].split("#");
                if(pair.size() == 2){
                    bool tempOk, speedOk;
                    const temp temperature = pair[0].toFloat(&tempOk);
                    const percentage speed = pair[1].toUShort(&speedOk);
                    if(tempOk && speedOk)
                        addFanStep(temperature, speed, true, true, false);
                }
            }

            fsPath.close();
        }
    }

    if(fanSteps.isEmpty()){
        qWarning() << "No saved fan steps found, using default profile";
        addFanStep(0,20,true, true, false);
        addFanStep(65,100,true, true, false);
        addFanStep(90,100, true, true, false);

    }

}

void radeon_profile::makeFanProfileGraph() {
    ui->plotFanProfile->graph(0)->clearData();

    for (short temperature : fanSteps.keys())
        ui->plotFanProfile->graph(0)->addData(temperature, fanSteps.value(temperature));

    ui->plotFanProfile->replot();
}

void radeon_profile::saveFanProfiles() {
    qDebug() << "Saving fan profiles to" << fanStepsPath;
    QFile fsPath(fanStepsPath);
    if (fsPath.open(QIODevice::WriteOnly)) {
        QString profile = "default|";
        for (short temperature : fanSteps.keys())
            profile.append(QString::number(temperature)+ "#" + QString::number(fanSteps.value(temperature))  + "|");

        fsPath.write(profile.toLatin1());
        fsPath.close();
    }
}

void radeon_profile::showWindow() {
    if (ui->cb_startMinimized->isChecked())
        this->showMinimized();
    else
        this->showNormal();
}
