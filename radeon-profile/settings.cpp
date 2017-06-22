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
const QString auxStuffPath = QDir::homePath() + "/.radeon-profile-auxstuff";

// init of static struct with setting exposed to global scope
globalStuff::globalCfgStruct globalStuff::globalConfig;

void radeon_profile::saveConfig() {
    QSettings settings(radeon_profile::settingsPath,QSettings::IniFormat);

    settings.setValue("startMinimized",ui->cb_startMinimized->isChecked());
    settings.setValue("minimizeToTray",ui->cb_minimizeTray->isChecked());
    settings.setValue("closeToTray",ui->cb_closeTray->isChecked());
    settings.setValue("updateInterval",ui->spin_timerInterval->value());
    settings.setValue("updateGraphs",ui->cb_graphs->isChecked());
    settings.setValue("saveWindowGeometry",ui->cb_saveWindowGeometry->isChecked());
    settings.setValue("windowGeometry",this->geometry());
    settings.setValue("powerLevelStatistics", ui->cb_stats->isChecked());
    settings.setValue("aleternateRowColors",ui->cb_alternateRow->isChecked());

    settings.setValue("graphOffset", ui->cb_plotsRightGap->isChecked());
    settings.setValue("graphRange",ui->slider_timeRange->value());
    settings.setValue("daemonAutoRefresh",ui->cb_daemonAutoRefresh->isChecked());
    settings.setValue("fanSpeedSlider",ui->fanSpeedSlider->value());
    settings.setValue("saveSelectedFanMode",ui->cb_saveFanMode->isChecked());
    settings.setValue("fanMode",ui->fanModesTabs->currentIndex());
    settings.setValue("fanProfileName",ui->l_currentFanProfile->text());
    settings.setValue("enableZeroPercentFanSpeed", ui->cb_zeroPercentFanSpeed->isChecked());

    settings.setValue("overclockEnabled", ui->cb_enableOverclock->isChecked());
    settings.setValue("overclockAtLaunch", ui->cb_overclockAtLaunch->isChecked());
    settings.setValue("overclockValue", ui->slider_overclock->value());

    settings.setValue("execDbcAction",ui->cb_execDbcAction->currentIndex());
    settings.setValue("appendSysEnv",ui->cb_execSysEnv->isChecked());
    settings.setValue("eventsTracking", ui->cb_eventsTracking->isChecked());

    settings.setValue("daemonData", ui->cb_daemonData->isChecked());

    QString xmlString;
    QXmlStreamWriter xml(&xmlString);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("auxStuff");

    saveRpevents(xml);
    saveExecProfiles(xml);
    saveFanProfiles(xml);
    savePlotSchemas(xml);

    xml.writeEndElement();
    xml.writeEndDocument();

    QFile f(auxStuffPath);
    if (f.open(QIODevice::WriteOnly))  {
        f.write(xmlString.toLatin1());
        f.close();
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

void radeon_profile::saveExecProfiles(QXmlStreamWriter &xml) {
    xml.writeStartElement("ExecProfiles");

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

void radeon_profile::saveFanProfiles(QXmlStreamWriter &xml) {
    xml.writeStartElement("FanProfiles");

    for (QString k : fanProfiles.keys()) {
        xml.writeStartElement("fanProfile");
        xml.writeAttribute("name", k);

        fanProfileSteps fps = fanProfiles.value(k);

        for (auto ks : fps.keys()) {
            xml.writeStartElement("step");
            xml.writeAttribute("temperature", QString::number(ks));
            xml.writeAttribute("speed", QString::number(fps.value(ks)));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

void radeon_profile::writePlotAxisSchemaToXml(QXmlStreamWriter &xml, const QString side, const PlotAxisSchema &pas) {
    xml.writeStartElement("axis");
    xml.writeAttribute("align", side);
    xml.writeAttribute("enabled", QString::number(pas.enabled));
    xml.writeAttribute("unit", QString::number(pas.unit));
    xml.writeAttribute("ticks", QString::number(pas.ticks));
    xml.writeAttribute("penStyle", QString::number(pas.penGrid.style()));
    xml.writeAttribute("penWidth", QString::number(pas.penGrid.width()));
    xml.writeAttribute("penColor", pas.penGrid.color().name());
    xml.writeEndElement();

    for (const ValueID &sk : pas.dataList.keys()) {
        xml.writeStartElement("serie");
        xml.writeAttribute("align", side);
        xml.writeAttribute("id", QString::number(sk));
        xml.writeAttribute("color", pas.dataList.value(sk).name());
        xml.writeEndElement();
    }
}

void radeon_profile::savePlotSchemas(QXmlStreamWriter &xml) {
    xml.writeStartElement("Plots");

    for (const QString &k : plotManager.schemas.keys()) {
        PlotDefinitionSchema pds = plotManager.schemas.value(k);

        xml.writeStartElement("plot");
        xml.writeAttribute("name", k);
        xml.writeAttribute("enabled", QString::number(pds.enabled));
        xml.writeAttribute("background", pds.background.name());

        writePlotAxisSchemaToXml(xml, "left", pds.left);
        writePlotAxisSchemaToXml(xml, "right", pds.right);

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
    ui->cb_graphs->setChecked(settings.value("updateGraphs",true).toBool());
    ui->cb_saveWindowGeometry->setChecked(settings.value("saveWindowGeometry").toBool());
    ui->cb_stats->setChecked(settings.value("powerLevelStatistics",true).toBool());
    ui->cb_alternateRow->setChecked(settings.value("aleternateRowColors",true).toBool());
    ui->cb_daemonAutoRefresh->setChecked(settings.value("daemonAutoRefresh",true).toBool());
    ui->cb_execDbcAction->setCurrentIndex(settings.value("execDbcAction",0).toInt());
    ui->fanSpeedSlider->setValue(settings.value("fanSpeedSlider",20).toInt());
    ui->cb_saveFanMode->setChecked(settings.value("saveSelectedFanMode",false).toBool());
    ui->l_currentFanProfile->setText(settings.value("fanProfileName","default").toString());
    ui->cb_zeroPercentFanSpeed->setChecked(settings.value("enableZeroPercentFanSpeed",false).toBool());
    if (ui->cb_saveFanMode->isChecked())
        ui->fanModesTabs->setCurrentIndex(settings.value("fanMode",0).toInt());

    ui->cb_plotsRightGap->setChecked(settings.value("graphOffset",true).toBool());
    ui->slider_timeRange->setValue(settings.value("graphRange",600).toInt());
    ui->cb_execSysEnv->setChecked(settings.value("appendSysEnv",true).toBool());
    ui->cb_eventsTracking->setChecked(settings.value("eventsTracking", false).toBool());

    ui->cb_enableOverclock->setChecked(settings.value("overclockEnabled",false).toBool());
    ui->cb_overclockAtLaunch->setChecked(settings.value("overclockAtLaunch",false).toBool());
    ui->slider_overclock->setValue(settings.value("overclockValue",0).toInt());

    ui->cb_daemonData->setChecked(settings.value("daemonData", false).toBool());

    globalStuff::globalConfig.daemonData = ui->cb_daemonData->isChecked();

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

    if (ui->cb_stats->isChecked())
        ui->tabs_systemInfo->setTabEnabled(3,true);
    else
        ui->tabs_systemInfo->setTabEnabled(3,false);

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

    plotManager.setRightGap(ui->cb_plotsRightGap->isChecked());
    hideEventControls(true);

    globalStuff::globalConfig.interval = ui->spin_timerInterval->value();
    globalStuff::globalConfig.daemonAutoRefresh = ui->cb_daemonAutoRefresh->isChecked();

    QFile f(auxStuffPath);
    if (f.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&f);
        while (!xml.atEnd()) {

            if (xml.readNextStartElement()) {
                if (xml.name().toString() == "rpevent") {
                    loadRpevent(xml);
                    continue;
                }

                if (xml.name().toString() == "execProfile") {
                    loadExecProfile(xml);
                    continue;
                }

                if (xml.name().toString() == "fanProfile") {
                    loadFanProfile(xml);
                    continue;
                }

                if (xml.name().toString() == "plot") {
                    loadPlotSchemas(xml);
                    continue;
                }
            }
        }
        f.close();
    }

    // legacy load
    loadExecProfiles();

    // legacy load
    loadFanProfiles();

    // create default if empty
    if (ui->combo_fanProfiles->count() == 0)
        createDefaultFanProfile();

    // create plots from xml config
    if (plotManager.schemas.count() > 0) {
        plotManager.recreatePlotsFromSchemas();
        createPlots();
    } else
        ui->stack_plots->setCurrentIndex(1);

    makeFanProfileListaAndGraph(fanProfiles.value(ui->combo_fanProfiles->currentText()));
}

void radeon_profile::loadRpevent(const QXmlStreamReader &xml) {
    RPEvent rpe;
    rpe.name = xml.attributes().value("name").toString();
    rpe.enabled = (xml.attributes().value("enabled") == "1");
    rpe.type = static_cast<rpeventType>(xml.attributes().value("tiggerType").toInt());
    rpe.activationBinary = xml.attributes().value("activationBinary").toString();
    rpe.activationTemperature = xml.attributes().value("activationTemperature").toInt();
    rpe.dpmProfileChange = static_cast<PowerProfiles>(xml.attributes().value("dpmProfileChange").toInt());
    rpe.powerLevelChange = static_cast<ForcePowerLevels>(xml.attributes().value("powerLevelChange").toInt());
    rpe.fixedFanSpeedChange = xml.attributes().value("fixedFanSpeedChange").toInt();
    rpe.fanProfileNameChange = xml.attributes().value("fanProfileNameChange").toString();
    rpe.fanComboIndex = xml.attributes().value("fanComboIndex").toInt();

    events.insert(rpe.name, rpe);

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(1, rpe.name);
    item->setCheckState(0,(rpe.enabled) ? Qt::Checked : Qt::Unchecked);
    ui->list_events->addTopLevelItem(item);
}

void radeon_profile::loadPlotAxisSchema(const QXmlStreamReader &xml, PlotAxisSchema &pas) {
    pas.unit = static_cast<ValueUnit>(xml.attributes().value("unit").toInt());
    pas.ticks = xml.attributes().value("ticks").toInt();
    pas.enabled = xml.attributes().value("enabled").toInt();

    pas.penGrid = QPen(QColor(xml.attributes().value("penColor").toString()),
                      xml.attributes().value("penWidth").toInt(),
                      static_cast<Qt::PenStyle>(xml.attributes().value("penStyle").toInt()));
}

void radeon_profile::loadPlotSchemas(QXmlStreamReader &xml) {
    PlotDefinitionSchema pds;
    pds.name = xml.attributes().value("name").toString();
    pds.enabled = xml.attributes().value("enabled").toInt();
    pds.background = QColor(xml.attributes().value("background").toString());

    while (xml.readNext()) {
        if (xml.name().toString() == "axis") {

            if (xml.attributes().value("align") == "left")
                loadPlotAxisSchema(xml, pds.left);
            else if (xml.attributes().value("align") == "right")
                loadPlotAxisSchema(xml, pds.right);

        }

        if (xml.name().toString() == "serie") {
            if (xml.attributes().value("align").toString() == "left")
                pds.left.dataList.insert(static_cast<ValueID>(xml.attributes().value("id").toInt()), QColor(xml.attributes().value("color").toString()));
            else if (xml.attributes().value("align").toString() == "right")
                pds.right.dataList.insert(static_cast<ValueID>(xml.attributes().value("id").toInt()), QColor(xml.attributes().value("color").toString()));

        }

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "plot") {
            break;
        }
    }

    plotManager.addSchema(pds);

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, pds.name);
    item->setCheckState(0,(pds.enabled) ? Qt::Checked : Qt::Unchecked);
    ui->list_plotDefinitions->addTopLevelItem(item);
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

void radeon_profile::loadFanProfile(QXmlStreamReader &xml) {
    QString fpName = xml.attributes().value("name").toString();

    fanProfileSteps fps;
    while (xml.readNext()) {
        if (xml.name().toString() == "step")
            fps.insert(xml.attributes().value("temperature").toString().toInt(),
                       xml.attributes().value("speed").toString().toInt());

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "fanProfile") {
            fanProfiles.insert(fpName, fps);
            ui->combo_fanProfiles->addItem(fpName);
            return;
        }
    }
}

// legacy load fan profiles
void radeon_profile::loadFanProfiles() {
    QFile fsPath(QDir::homePath() + "/.radeon-profile-fanSteps");

    if (fsPath.open(QIODevice::ReadOnly) && ui->combo_fanProfiles->count() == 0) {
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
        // remove old file
        fsPath.remove();
        fsPath.close();
    }
}

// legacy load
void radeon_profile::loadExecProfiles()
{
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

void radeon_profile::setupMinFanSpeedSetting(unsigned int speed) {
    minFanStepsSpeed = speed;
    ui->l_minFanSpeed->setText(QString::number(minFanStepsSpeed)+"%");
    ui->fanSpeedSlider->setMinimum(minFanStepsSpeed);
}
