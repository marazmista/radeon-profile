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

QString getConfigPath() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/radeon-profile";
}

static const QString legacySettingsPath = QDir::homePath() + "/.radeon-profile-settings";
static const QString legacyAuxStuffPath = QDir::homePath() + "/.radeon-profile-auxstuff";

static const QString settingsPath = getConfigPath() + "/radeon-profile-settings";
static const QString auxStuffPath = getConfigPath() + "/radeon-profile-auxstuff";

static bool loadedFromLegacy = false;

void radeon_profile::saveConfig() {
    {
        // If settingsPath doesn't exist yet, running QSetting's destructor will create it.
        // It's important that happens before saving auxstuff later-on.
        QSettings settings(settingsPath,QSettings::IniFormat);

        settings.setValue("startMinimized",ui->cb_startMinimized->isChecked());
        settings.setValue("minimizeToTray",ui->cb_minimizeTray->isChecked());
        settings.setValue("closeToTray",ui->cb_closeTray->isChecked());
        settings.setValue("updateInterval",ui->spin_timerInterval->value());
        settings.setValue("updateGraphs",ui->cb_graphs->isChecked());
        settings.setValue("saveWindowGeometry",ui->cb_saveWindowGeometry->isChecked());
        settings.setValue("windowGeometry",this->geometry());
        settings.setValue("powerLevelStatistics", ui->cb_stats->isChecked());
        settings.setValue("aleternateRowColors",ui->cb_alternateRow->isChecked());

        settings.setValue("graphOffset", plotManager.generalConfig.graphOffset);
        settings.setValue("graphRange", ui->slider_timeRange->value());
        settings.setValue("showLegend", plotManager.generalConfig.showLegend);
        settings.setValue("plotsBackgroundColor", plotManager.generalConfig.plotsBackground);
        settings.setValue("setCommonPlotsBg", plotManager.generalConfig.commonPlotsBackground);
        settings.setValue("daemonAutoRefresh",ui->cb_daemonAutoRefresh->isChecked());
        settings.setValue("fanSpeedSlider",ui->slider_fanSpeed->value());
        settings.setValue("saveSelectedFanMode",ui->cb_saveFanMode->isChecked());
        settings.setValue("fanMode",ui->stack_fanModes->currentIndex());
        settings.setValue("fanProfileName",ui->l_currentFanProfile->text());

        settings.setValue("restorePercentOverclock", ui->cb_restorePercentOc->isChecked());
        settings.setValue("overclockValue", ui->slider_ocSclk->value());
        settings.setValue("overclockMemValue", ui->slider_ocMclk->value());
        settings.setValue("restoreFrequencyStates", ui->cb_restoreFrequencyStates->isChecked());
        settings.setValue("enabledFrequencyStates", enabledFrequencyStatesCore);
        settings.setValue("enabledFrequencyStatesMem", enabledFrequencyStatesMem);

        settings.setValue("ocProfileName", ui->l_currentOcProfile->text());
        settings.setValue("restoreOcProfile", ui->cb_restoreOcProfile->isChecked());

        settings.setValue("execDbcAction",ui->combo_execDbcAction->currentIndex());
        settings.setValue("appendSysEnv",ui->cb_execSysEnv->isChecked());
        settings.setValue("eventsTracking", ui->cb_eventsTracking->isChecked());
        settings.setValue("daemonData", ui->cb_daemonData->isChecked());
        settings.setValue("connConfirmMethod", ui->combo_connConfirmMethod->currentIndex());
        settings.setValue("refreshWhenHidden", refreshWhenHidden->isChecked());
    }

    QString xmlString;
    QXmlStreamWriter xml(&xmlString);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("auxStuff");

    saveRpevents(xml);
    saveExecProfiles(xml);
    saveFanProfiles(xml);
    savePlotSchemas(xml);
    saveTopbarItemsSchemas(xml);
    saveOcProfiles(xml);

    xml.writeEndElement();
    xml.writeEndDocument();

    QFile f(auxStuffPath);
    if (f.open(QIODevice::WriteOnly))  {
        f.write(xmlString.toLatin1());
        f.close();
    }

    if (loadedFromLegacy) {
        QFile::remove(legacySettingsPath);
        QFile::remove(legacyAuxStuffPath);
        loadedFromLegacy = false;
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
        xml.writeAttribute("sensorInstance", QString::number(rpe.sensorInstance));
        xml.writeAttribute("powerProfileChange", rpe.powerProfileChange);
        xml.writeAttribute("powerLevelChange", rpe.powerLevelChange);
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

        FanProfile fps = fanProfiles.value(k);

        xml.writeAttribute("hysteresis", QString::number(fps.hysteresis));
        xml.writeAttribute("sensorInstance", QString::number(fps.sensor));

        for (auto ks : fps.steps.keys()) {
            xml.writeStartElement("step");
            xml.writeAttribute("temperature", QString::number(ks));
            xml.writeAttribute("speed", QString::number(fps.steps.value(ks)));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

void radeon_profile::saveOcProfiles(QXmlStreamWriter &xml) {
    xml.writeStartElement("OcProfiles");

    for (QString k : ocProfiles.keys()) {
        xml.writeStartElement("ocProfile");
        xml.writeAttribute("name", k);

        auto oct = ocProfiles.value(k);
        xml.writeAttribute("powerCap", QString::number(oct.powerCap));

        for (auto tk : oct.tables.keys()) {
            const FVTable &fvt = oct.tables.value(tk);

            xml.writeStartElement("table");
            xml.writeAttribute("tableName", tk);

            for (auto fvtk : fvt.keys()) {
                const FreqVoltPair &fvp = fvt.value(fvtk);

                xml.writeStartElement("state");
                xml.writeAttribute("enabled", QString::number(true));
                xml.writeAttribute("stateNumber", QString::number(fvtk));
                xml.writeAttribute("frequency", QString::number(fvp.frequency));
                xml.writeAttribute("voltage", QString::number(fvp.voltage));

                xml.writeEndElement();
            }

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
        xml.writeAttribute("id", QString::number(sk.key()));
        xml.writeAttribute("color", pas.dataList.value(sk).name());
        xml.writeEndElement();
    }
}

void radeon_profile::savePlotSchemas(QXmlStreamWriter &xml) {
    xml.writeStartElement("Plots");

    for (int i = 0; i < plotManager.schemas.count(); ++i) {
        const PlotDefinitionSchema &pds = plotManager.schemas.at(i);

        xml.writeStartElement("plot");
        xml.writeAttribute("name", pds.name);
        xml.writeAttribute("enabled", QString::number(pds.enabled));
        xml.writeAttribute("background", pds.background.name());

        writePlotAxisSchemaToXml(xml, "left", pds.left);
        writePlotAxisSchemaToXml(xml, "right", pds.right);

        xml.writeEndElement();
    }

    xml.writeEndElement();
}

void radeon_profile::saveTopbarItemsSchemas(QXmlStreamWriter &xml) {
    xml.writeStartElement("TopbarItems");

    for (const TopbarItemDefinitionSchema &tis : topbarManager.schemas) {
        xml.writeStartElement("topbarItem");

        xml.writeAttribute("type", QString::number(tis.type));
        xml.writeAttribute("primaryValueId", QString::number(tis.primaryValueId.key()));
        xml.writeAttribute("primaryColor", tis.primaryColor.name());
        xml.writeAttribute("secondaryValueIdEnabled", QString::number(tis.secondaryValueIdEnabled));
        xml.writeAttribute("secondaryValueId", QString::number(tis.secondaryValueId.key()));
        xml.writeAttribute("secondaryColor", tis.secondaryColor.name());
        xml.writeAttribute("pieMaxValue", QString::number(tis.pieMaxValue));

        xml.writeEndElement();
    }
    xml.writeEndElement();
}

void radeon_profile::loadConfig() {
    qDebug() << "Loading configuration";

    // Try to load from config first, fallback to old file path if not found.
    loadedFromLegacy = !QFileInfo::exists(settingsPath);
    const auto configPath = loadedFromLegacy ? legacySettingsPath : settingsPath;
    QSettings settings(configPath,QSettings::IniFormat);

    ui->cb_startMinimized->setChecked(settings.value("startMinimized",false).toBool());
    ui->cb_minimizeTray->setChecked(settings.value("minimizeToTray",false).toBool());
    ui->cb_closeTray->setChecked(settings.value("closeToTray",false).toBool());
    ui->spin_timerInterval->setValue(settings.value("updateInterval",1).toDouble());
    ui->cb_graphs->setChecked(settings.value("updateGraphs",true).toBool());
    ui->cb_saveWindowGeometry->setChecked(settings.value("saveWindowGeometry").toBool());
    ui->cb_stats->setChecked(settings.value("powerLevelStatistics",true).toBool());
    ui->cb_alternateRow->setChecked(settings.value("aleternateRowColors",true).toBool());
    ui->cb_daemonAutoRefresh->setChecked(settings.value("daemonAutoRefresh",true).toBool());
    ui->combo_execDbcAction->setCurrentIndex(settings.value("execDbcAction",0).toInt());
    ui->slider_fanSpeed->setValue(settings.value("fanSpeedSlider",20).toInt());
    ui->cb_saveFanMode->setChecked(settings.value("saveSelectedFanMode",false).toBool());
    ui->l_currentFanProfile->setText(settings.value("fanProfileName","default").toString());
    if (ui->cb_saveFanMode->isChecked())
        ui->stack_fanModes->setCurrentIndex(settings.value("fanMode",0).toInt());

    plotManager.generalConfig.graphOffset = settings.value("graphOffset",true).toBool();
    ui->slider_timeRange->setValue(settings.value("graphRange",600).toInt());
    plotManager.generalConfig.showLegend = settings.value("showLegend",false).toBool();
    ui->cb_execSysEnv->setChecked(settings.value("appendSysEnv",true).toBool());
    ui->cb_eventsTracking->setChecked(settings.value("eventsTracking", false).toBool());

    ui->cb_restorePercentOc->setChecked(settings.value("restorePercentOverclock", false).toBool());
    ui->group_oc->setChecked(ui->cb_restorePercentOc->isChecked());
    ui->cb_restoreFrequencyStates->setChecked(settings.value("restoreFrequencyStates", false).toBool());
    enabledFrequencyStatesCore = settings.value("enabledFrequencyStates", "0 1 2 3 4 5 6 7").toString();
    enabledFrequencyStatesMem = settings.value("enabledFrequencyStatesMem", "0 1 2 3").toString();
    ui->group_freq->setChecked(ui->cb_restoreFrequencyStates->isChecked());
    ui->slider_ocSclk->setValue(settings.value("overclockValue",0).toInt());
    ui->slider_ocMclk->setValue(settings.value("overclockMemValue",0).toInt());
    ui->cb_daemonData->setChecked(settings.value("daemonData", false).toBool());
    ui->combo_connConfirmMethod->setCurrentIndex(settings.value("connConfirmMethod", 1).toInt());
    // old config files without per FanProfile hysteresis value may still have
    // the global value. This will be applied in loadFanProfile().
    const int legacy_hysteresis = settings.value("temperatureHysteresis", 0).toInt();

    plotManager.generalConfig.plotsBackground = QColor(settings.value("plotsBackgroundColor","#808080").toString());
    plotManager.generalConfig.commonPlotsBackground = settings.value("setCommonPlotsBg", false).toBool();

    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(settings.value("refreshWhenHidden", true).toBool());

    ui->cb_restoreOcProfile->setChecked(settings.value("restoreOcProfile", false).toBool());
    if (ui->cb_restoreOcProfile->isChecked())
        ui->l_currentOcProfile->setText(settings.value("ocProfileName", "default").toString());

    if (!ui->cb_daemonData->isChecked())
        ui->cb_daemonAutoRefresh->setEnabled(false);

    dcomm.setConnectionConfirmationMethod(static_cast<DaemonComm::ConfirmationMehtod>(ui->combo_connConfirmMethod->currentIndex()));

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

    timer->setInterval(ui->spin_timerInterval->value() * 1000);

    if (ui->cb_stats->isChecked())
        ui->tw_systemInfo->setTabEnabled(3,true);
    else
        ui->tw_systemInfo->setTabEnabled(3,false);

    on_cb_alternateRow_clicked(ui->cb_alternateRow->isChecked());

    plotManager.setRightGap();
    hideEventControls(true);

    const auto auxFilePath = loadedFromLegacy ? legacyAuxStuffPath : auxStuffPath;
    QFile f(auxFilePath);
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
                    loadFanProfile(xml, legacy_hysteresis);
                    continue;
                }

                if (xml.name().toString() == "ocProfile") {
                    loadOcProfile(xml);
                    continue;
                }

                if (xml.name().toString() == "plot") {
                    loadPlotSchemas(xml);
                    continue;
                }

                if (xml.name().toString() == "topbarItem") {
                    loadTopbarItemsSchemas(xml);
                    continue;
                }
            }
        }

        f.close();
    }

    // create default if empty
    if (fanProfiles.count() == 0)
        createDefaultFanProfile();

    topbarManager.setDefaultForeground(QWidget::palette().foreground().color());
}

void radeon_profile::loadRpevent(const QXmlStreamReader &xml) {
    RPEvent rpe;
    rpe.name = xml.attributes().value("name").toString();
    rpe.enabled = (xml.attributes().value("enabled") == "1");
    rpe.type = static_cast<RPEventType>(xml.attributes().value("tiggerType").toInt());
    rpe.activationBinary = xml.attributes().value("activationBinary").toString();
    rpe.activationTemperature = xml.attributes().value("activationTemperature").toInt();
    rpe.sensorInstance = xml.attributes().value("sensorInstance").toUInt();
    rpe.powerProfileChange = xml.attributes().value("powerProfileChange").toString();
    rpe.powerLevelChange = xml.attributes().value("powerLevelChange").toString();
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
                pds.left.dataList.insert(ValueID::fromKey(xml.attributes().value("id").toULong()), QColor(xml.attributes().value("color").toString()));
            else if (xml.attributes().value("align").toString() == "right")
                pds.right.dataList.insert(ValueID::fromKey(xml.attributes().value("id").toULong()), QColor(xml.attributes().value("color").toString()));

        }

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "plot") {
            break;
        }
    }

    plotManager.addSchema(pds);
}

void radeon_profile::loadTopbarItemsSchemas(const QXmlStreamReader &xml) {
    TopbarItemDefinitionSchema tis(ValueID::fromKey(xml.attributes().value("primaryValueId").toULong()),
                                   static_cast<TopbarItemType>(xml.attributes().value("type").toInt()),
                                   QColor(xml.attributes().value("primaryColor").toString()));

    tis.setPieMaxValue(xml.attributes().value("pieMaxValue").toInt());

    if (xml.attributes().value("secondaryValueIdEnabled").toInt() == 1) {
        tis.setSecondaryColor(QColor(xml.attributes().value("secondaryColor").toString()));
        tis.setSecondaryValueId(ValueID::fromKey(xml.attributes().value("secondaryValueId").toULong()));
    }

    topbarManager.addSchema(tis);
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

void radeon_profile::loadFanProfile(QXmlStreamReader &xml, const int default_hysteresis) {
    QString fpName = xml.attributes().value("name").toString();

    FanProfile fps;
    fps.hysteresis = xml.attributes().hasAttribute("hysteresis")
                   ? xml.attributes().value("hysteresis").toInt()
                   : default_hysteresis;
    fps.sensor = xml.attributes().hasAttribute("sensorInstance")
               ? xml.attributes().value("sensorInstance").toInt()
               : ValueID::T_EDGE;
    while (xml.readNext()) {
        if (xml.name().toString() == "step" && !xml.attributes().value("temperature").isEmpty())
            fps.steps.insert(xml.attributes().value("temperature").toString().toInt(),
                             xml.attributes().value("speed").toString().toInt());

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "fanProfile") {
            fanProfiles.insert(fpName, fps);
            return;
        }
    }
}

void radeon_profile::loadOcProfile(QXmlStreamReader &xml) {
    const QString name = xml.attributes().value("name").toString();

    OCProfile ocp;
    ocp.powerCap = xml.attributes().value("powerCap").toInt();


    FVTable table;
    QString tableName;
    while(xml.readNext()) {
        if (xml.name().toString() == "table" && !xml.attributes().value("tableName").isEmpty())
            tableName = xml.attributes().value("tableName").toString();

        if (xml.name().toString() == "state" && !xml.attributes().value("stateNumber").isEmpty()) {
            table.insert(xml.attributes().value("stateNumber").toUInt(),
                         FreqVoltPair(xml.attributes().value("frequency").toUInt(),
                                      xml.attributes().value("voltage").toUInt()));
        }

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "table") {
            ocp.tables.insert(tableName, table);
            table.clear();
            tableName = "";
        }

        if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "ocProfile") {
            ocProfiles.insert(name, ocp);
            return;
        }
    }
}
