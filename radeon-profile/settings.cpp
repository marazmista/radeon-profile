// copyright marazmista @ 30.03.2014

// there is a settings handling, save and load from ini //

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include <QSettings>
#include <QMenu>
#include <QDir>

const QString radeon_profile::settingsPath = QDir::homePath() + "/.radeon-profile-settings";

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
    settings.setValue("graphRange",ui->timeSlider->value());

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

    optionsMenu->actions().at(0)->setChecked(settings.value("showLegend",true).toBool());
    ui->timeSlider->setValue(settings.value("graphRange",180).toInt());
    // Graphs settings
    ui->spin_lineThick->setValue(settings.value("graphLineThickness",2).toInt());
    // detalis: http://qt-project.org/doc/qt-4.8/qvariant.html#a-note-on-gui-types
    //ok, color is saved as QVariant, and read and convertsion it to QColor is below
    ui->graphColorsList->topLevelItem(TEMP_BG)->setBackgroundColor(1,settings.value("graphTempBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(CLOCKS_BG)->setBackgroundColor(1,settings.value("graphClocksBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(VOLTS_BG)->setBackgroundColor(1,settings.value("graphVoltsBackground",Qt::darkGray).value<QColor>());
    ui->graphColorsList->topLevelItem(TEMP_LINE)->setBackgroundColor(1,settings.value("graphTempLine",Qt::yellow).value<QColor>());
    ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphGPUClockLine",Qt::black).value<QColor>());
    ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->setBackgroundColor(1,settings.value("graphMemClockLine",Qt::cyan).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->setBackgroundColor(1,settings.value("graphUVDVideoLine",Qt::red).value<QColor>());
    ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->setBackgroundColor(1,settings.value("graphUVDDecoderLine",Qt::green).value<QColor>());
    ui->graphColorsList->topLevelItem(CORE_VOLTS_LINE)->setBackgroundColor(1,settings.value("graphVoltsLine",Qt::blue).value<QColor>());
    ui->graphColorsList->topLevelItem(MEM_VOLTS_LINE)->setBackgroundColor(1,settings.value("graphMemVoltsLine",Qt::cyan).value<QColor>());
    setupGraphsStyle();

    ui->cb_showTempsGraph->setChecked(settings.value("showTempGraphOnStart",true).toBool());
    ui->cb_showFreqGraph->setChecked(settings.value("showFreqGraphOnStart",true).toBool());
    ui->cb_showVoltsGraph->setChecked(settings.value("showVoltsGraphOnStart",false).toBool());

    // apply some settings to ui on start //
    if (ui->cb_saveWindowGeometry->isChecked())
        this->setGeometry(settings.value("windowGeometry").toRect());

    if (ui->cb_startMinimized->isChecked())
        this->window()->hide();
    else
        showNormal();

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

    showLegend(optionsMenu->actions().at(0)->isChecked());
    changeTimeRange();
    on_cb_showTempsGraph_clicked(ui->cb_showTempsGraph->isChecked());
    on_cb_showFreqGraph_clicked(ui->cb_showFreqGraph->isChecked());
    on_cb_showVoltsGraph_clicked(ui->cb_showVoltsGraph->isChecked());
}
