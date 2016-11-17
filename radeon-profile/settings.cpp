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

    settings.setValue("showLegend",optionsMenu->actions().at(0)->isChecked());
    settings.setValue("graphOffset",optionsMenu->actions().at(1)->isChecked());
    settings.setValue("graphRange",ui->timeSlider->value());
    settings.setValue("daemonAutoRefresh",ui->cb_daemonAutoRefresh->isChecked());
    settings.setValue("fanSpeedSlider",ui->fanSpeedSlider->value());
    settings.setValue("saveSelectedFanMode",ui->cb_saveFanMode->isChecked());
    settings.setValue("fanMode",ui->fanModesTabs->currentIndex());
    settings.setValue("fanProfileName",ui->l_currentFanProfile->text());

    settings.setValue("overclockEnabled", ui->cb_enableOverclock->isChecked());
    settings.setValue("overclockAtLaunch", ui->cb_overclockAtLaunch->isChecked());
    settings.setValue("overclockValue", ui->slider_overclock->value());

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
    QSettings settings(radeon_profile::settingsPath,QSettings::IniFormat);

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
    ui->cb_stats->setChecked(settings.value("powerLevelStatistics",true).toBool());
    ui->cb_alternateRow->setChecked(settings.value("aleternateRowColors",true).toBool());
    ui->cb_daemonAutoRefresh->setChecked(settings.value("daemonAutoRefresh",true).toBool());
    ui->cb_execDbcAction->setCurrentIndex(settings.value("execDbcAction",0).toInt());
    ui->fanSpeedSlider->setValue(settings.value("fanSpeedSlider",80).toInt());
    ui->cb_saveFanMode->setChecked(settings.value("saveSelectedFanMode",false).toBool());
    ui->l_currentFanProfile->setText(settings.value("fanProfileName","default").toString());
    if (ui->cb_saveFanMode->isChecked())
        ui->fanModesTabs->setCurrentIndex(settings.value("fanMode",0).toInt());

    optionsMenu->actions().at(0)->setChecked(settings.value("showLegend",true).toBool());
    optionsMenu->actions().at(1)->setChecked(settings.value("graphOffset",true).toBool());
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
    ui->slider_overclock->setValue(settings.value("overclockValue",0).toInt());

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

    ui->cb_graphs->setEnabled(ui->cb_gpuData->isChecked());
    ui->cb_stats->setEnabled(ui->cb_gpuData->isChecked());

    if (ui->cb_gpuData->isChecked()) {
        if (ui->cb_stats->isChecked())
            ui->tabs_systemInfo->setTabEnabled(3,true);
        else
            ui->tabs_systemInfo->setTabEnabled(3,false);
    } else
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << "GPU data is disabled."));

    if (ui->cb_graphs->isChecked() && ui->cb_graphs->isEnabled())
        ui->mainTabs->setTabEnabled(1,true);
    else
        ui->mainTabs->setTabEnabled(1,false);

    ui->list_currentGPUData->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_glxinfo->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_modInfo->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_connectors->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_stats->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_execProfiles->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_variables->setAlternatingRowColors(ui->cb_alternateRow->isChecked());
    ui->list_vaules->setAlternatingRowColors(ui->cb_alternateRow->isChecked());

    showLegend(optionsMenu->actions().at(0)->isChecked());
    changeTimeRange();
    on_cb_showTempsGraph_clicked(ui->cb_showTempsGraph->isChecked());
    on_cb_showFreqGraph_clicked(ui->cb_showFreqGraph->isChecked());
    on_cb_showVoltsGraph_clicked(ui->cb_showVoltsGraph->isChecked());

    globalStuff::globalConfig.interval = ui->spin_timerInterval->value();
    globalStuff::globalConfig.daemonAutoRefresh = ui->cb_daemonAutoRefresh->isChecked();
    globalStuff::globalConfig.graphOffset = ((optionsMenu->actions().at(1)->isChecked()) ? 20 : 0);

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
    QFile fsPath(fanStepsPath);

    if (fsPath.open(QIODevice::ReadOnly)) {
        QStringList profiles = QString(fsPath.readAll()).split('\n',QString::SkipEmptyParts);

        for (QString profileLine : profiles) {
            QStringList profileData = profileLine.split("|",QString::SkipEmptyParts);

            fanProfileSteps p;

            for (int i = 1; i < profileData.count(); ++i) {
                QStringList pair = profileData[i].split("#");
                p.insert(pair[0].toInt(),pair[1].toInt());
            }


            ui->combo_fanProfiles->addItem(profileData[0]);
            fanProfiles.insert(profileData[0], p);
        }

        fsPath.close();
    } else {
        //default profile if no file found

        fanProfileSteps p;
        p.insert(0,minFanStepsSpeed);
        p.insert(65,maxFanStepsSpeed);
        p.insert(90,maxFanStepsSpeed);

        fanProfiles.insert("default", p);
    }

    makeFanProfileListaAndGraph(fanProfiles.value("default"));
}

void radeon_profile::makeFanProfileListaAndGraph(const fanProfileSteps &profile) {
    ui->plotFanProfile->graph(0)->clearData();
    ui->list_fanSteps->clear();

    for (int temperature : profile.keys()) {
        ui->plotFanProfile->graph(0)->addData(temperature, profile.value(temperature));
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << QString::number(temperature) << QString::number(profile.value(temperature))));
    }

    ui->plotFanProfile->replot();
}

void radeon_profile::saveFanProfiles() {
    QFile fsPath(fanStepsPath);

    if (fsPath.open(QIODevice::WriteOnly)) {
        QString profileString;

        for (QString profile : fanProfiles.keys()) {
            profileString.append(profile+"|");

            fanProfileSteps steps = fanProfiles.value(profile);

            for (int temperature : steps.keys())
                profileString.append(QString::number(temperature)+ "#" + QString::number(steps.value(temperature))  + "|");


            profileString.append('\n');
        }

        fsPath.write(profileString.toLatin1());
        fsPath.close();
    }
}

bool radeon_profile::fanStepIsValid(const int temperature, const int fanSpeed) {
    return temperature >= minFanStepsTemp &&
            temperature <= maxFanStepsTemp &&
            fanSpeed >= minFanStepsSpeed &&
            fanSpeed <= maxFanStepsSpeed;
}

void radeon_profile::addFanStep(const int temperature, const int fanSpeed) {

    if (!fanStepIsValid(temperature, fanSpeed)) {
        qWarning() << "Invalid value, can't be inserted into the fan step list:" << temperature << fanSpeed;
        return;
    }

    currentFanProfile.insert(temperature,fanSpeed);

    const QString temperatureString = QString::number(temperature),
            speedString = QString::number(fanSpeed);
    const QList<QTreeWidgetItem*> existing = ui->list_fanSteps->findItems(temperatureString,Qt::MatchExactly);

    if (existing.isEmpty()) { // The element does not exist
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << temperatureString << speedString));
        ui->list_fanSteps->sortItems(0, Qt::AscendingOrder);
    } else // The element exists already, overwrite it
        existing.first()->setText(1,speedString);

    makeFanProfileListaAndGraph(currentFanProfile);
}

void radeon_profile::showWindow() {
    if (ui->cb_minimizeTray->isChecked() && ui->cb_startMinimized->isChecked())
        return;

    if (ui->cb_startMinimized->isChecked())
        this->showMinimized();
    else
        this->showNormal();
}
