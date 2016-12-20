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
const QString auxStuffPath = QDir::homePath() + "/radeon-profile-auxstuff";

// legacy files, old version load
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
    settings.setValue("enableZeroPercentFanSpeed", ui->cb_zeroPercentFanSpeed->isChecked());

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


    QString xmlString;
    QXmlStreamWriter xml(&xmlString);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("auxStuff");

    saveRpevents(xml);
    saveExecProfiles(xml);

    xml.writeEndElement();
    xml.writeEndDocument();

    QFile f(auxStuffPath);
    if (f.open(QIODevice::WriteOnly))  {
        f.write(xmlString.toLatin1());
        f.close();
    }
}

void radeon_profile::saveExecProfiles(QXmlStreamWriter &xml) {
    xml.writeStartElement("execProfiles");

    for (int i = 0; i < ui->list_execProfiles->topLevelItemCount(); ++i) {
        xml.writeStartElement("execProfile");
        xml.writeAttribute("name", ui->list_execProfiles->topLevelItem(i)->text(PROFILE_NAME));
        xml.writeAttribute("binary", ui->list_execProfiles->topLevelItem(i)->text(BINARY));
        xml.writeAttribute("binaryParams",ui->list_execProfiles->topLevelItem(i)->text(BINARY_PARAMS) );
        xml.writeAttribute("envSettings", ui->list_execProfiles->topLevelItem(i)->text(ENV_SETTINGS));
        xml.writeAttribute("logFile",  ui->list_execProfiles->topLevelItem(i)->text(LOG_FILE));
        xml.writeAttribute("logFileDateAppend", ui->list_execProfiles->topLevelItem(i)->text(LOG_FILE_DATE_APPEND));
        xml.writeEndElement();
    }

    xml.writeEndElement();
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
    ui->cb_zeroPercentFanSpeed->setChecked(settings.value("enableZeroPercentFanSpeed",false).toBool());
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

    if (ui->cb_zeroPercentFanSpeed->isChecked())
        setupMinFanSpeedSetting(0);

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


    QFile f(auxStuffPath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QXmlStreamReader xml(&f);
    while (!xml.atEnd()) {

        if (xml.readNextStartElement()) {
            qDebug() << xml.name();

            if (xml.name().toString() == "rpevent") {
                loadRpevent(xml);
                continue;
            }

            if (xml.name().toString() == "execProfile") {
                loadExecProfile(xml);
                continue;
            }

        }
    }
    f.close();

    // legacy load
    QFile ef(QDir::homePath() + "/.radeon-profile-execProfiles");
    if (ef.open(QIODevice::ReadOnly) && ui->list_execProfiles->topLevelItemCount() == 0) {
        QStringList profiles = QString(ef.readAll()).split('\n');

        for (int i=0;i < profiles.count(); i++) {
            if (!profiles[i].isEmpty())
                ui->list_execProfiles->addTopLevelItem(new QTreeWidgetItem(QStringList() << profiles[i].split("###")));
        }
        // remove old file
        ef.remove();
        ef.close();
    }
}

void radeon_profile::loadExecProfile(const QXmlStreamReader &xml) {
    QTreeWidgetItem *item = new QTreeWidgetItem();

    item->setText(PROFILE_NAME, xml.attributes().value("name").toString());
    item->setText(BINARY,xml.attributes().value("binary").toString());
    item->setText(BINARY_PARAMS, xml.attributes().value("binaryParams").toString());
    item->setText(ENV_SETTINGS, xml.attributes().value("envSettings").toString());
    item->setText(LOG_FILE, xml.attributes().value("logFile").toString());
    item->setText(LOG_FILE_DATE_APPEND, xml.attributes().value("logFileDateAppend").toString());

    ui->list_execProfiles->addTopLevelItem(item);
}

void radeon_profile::loadFanProfiles() {
    QFile fsPath(fanStepsPath);
    ui->l_fanProfileUnsavedIndicator->setVisible(false);

    if (fsPath.open(QIODevice::ReadOnly)) {
        QStringList profiles = QString(fsPath.readAll()).trimmed().split('\n',QString::SkipEmptyParts);

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
        ui->combo_fanProfiles->addItem("default");
        ui->l_currentFanProfile->setText("default");
    }

    makeFanProfileListaAndGraph(fanProfiles.value(ui->combo_fanProfiles->currentText()));
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

void radeon_profile::saveRpevents(QXmlStreamWriter &xml) {
    xml.writeStartElement("RPEvents");

    for (RPEvent rpe : events) {
        xml.writeStartElement("rpevent");
        xml.writeAttribute("name", rpe.name);
        xml.writeAttribute("enabled", QString::number(rpe.enabled));
        xml.writeAttribute("tiggerType", QString::number(rpe.type));
        xml.writeAttribute("activationBinary", rpe.activationBinary);
        xml.writeAttribute("activationTemperature", QString::number(rpe.activationTemperature));
        xml.writeAttribute("dpmProfileChange", QString::number(rpe.dpmProfileChange));
        xml.writeAttribute("powerLevelChange", QString::number(rpe.powerLevelChange));
        xml.writeAttribute("fixedFanSpeedChange", QString::number(rpe.fixedFanSpeedChange));
        xml.writeAttribute("fanProfileNameChange",rpe.fanProfileNameChange);
        xml.writeAttribute("fanComboIndex", QString::number(rpe.fanComboIndex));
        xml.writeEndElement();
    }

    xml.writeEndElement();
}

void radeon_profile::loadRpevent(const QXmlStreamReader &xml) {

    RPEvent rpe;
    rpe.name = xml.attributes().value("name").toString();
    rpe.enabled = (xml.attributes().value("enabled") == "1");
    rpe.type = static_cast<rpeventType>(xml.attributes().value("tiggerType").toInt());
    rpe.activationBinary = xml.attributes().value("activationBinary").toString();
    rpe.activationTemperature = xml.attributes().value("activationTemperature").toInt();
    rpe.dpmProfileChange = static_cast<globalStuff::powerProfiles>(xml.attributes().value("dpmProfileChange").toInt());
    rpe.powerLevelChange = static_cast<globalStuff::forcePowerLevels>(xml.attributes().value("powerLevelChange").toInt());
    rpe.fixedFanSpeedChange = xml.attributes().value("fixedFanSpeedChange").toInt();
    rpe.fanProfileNameChange = xml.attributes().value("fanProfileNameChange").toString();
    rpe.fanComboIndex = xml.attributes().value("fanComboIndex").toInt();

    events.insert(rpe.name, rpe);

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(1, rpe.name);
    item->setCheckState(0,(rpe.enabled) ? Qt::Checked : Qt::Unchecked);
    ui->list_events->addTopLevelItem(item);
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

void radeon_profile::makeFanProfilePlot() {
    ui->plotFanProfile->graph(0)->clearData();

    for (int i = 0; i < ui->list_fanSteps->topLevelItemCount(); ++i)
        ui->plotFanProfile->graph(0)->addData(ui->list_fanSteps->topLevelItem(i)->text(0).toInt(),
                                              ui->list_fanSteps->topLevelItem(i)->text(1).toInt());

    ui->plotFanProfile->replot();
}


bool radeon_profile::isFanStepValid(const unsigned int temperature, const unsigned int fanSpeed) {
    return temperature <= maxFanStepsTemp &&
            fanSpeed >= minFanStepsSpeed &&
            fanSpeed <= maxFanStepsSpeed;
}

void radeon_profile::addFanStep(const int temperature, const int fanSpeed) {

    if (!isFanStepValid(temperature, fanSpeed)) {
        qWarning() << "Invalid value, can't be inserted into the fan step list:" << temperature << fanSpeed;
        return;
    }

    const QString temperatureString = QString::number(temperature),
            speedString = QString::number(fanSpeed);
    const QList<QTreeWidgetItem*> existing = ui->list_fanSteps->findItems(temperatureString,Qt::MatchExactly);

    if (existing.isEmpty()) { // The element does not exist
        ui->list_fanSteps->addTopLevelItem(new QTreeWidgetItem(QStringList() << temperatureString << speedString));
        ui->list_fanSteps->sortItems(0, Qt::AscendingOrder);
    } else // The element exists already, overwrite it
        existing.first()->setText(1,speedString);

    markFanProfileUnsaved(true);
    makeFanProfilePlot();
}

void radeon_profile::markFanProfileUnsaved(bool unsaved) {
    ui->l_fanProfileUnsavedIndicator->setVisible(unsaved);
}

void radeon_profile::showWindow() {
    if (ui->cb_minimizeTray->isChecked() && ui->cb_startMinimized->isChecked())
        return;

    if (ui->cb_startMinimized->isChecked())
        this->showMinimized();
    else
        this->showNormal();
}
