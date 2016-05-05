// copyright marazmista @ 29.12.2013

// this file contains functions for creating some gui elements (menus, graphs)

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMenu>

//===================================
// === GUI setup functions === //
void radeon_profile::setupGraphs()
{
    ui->plotClocks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);

    ui->plotTemp->xAxis->setLabel(label_timeAxis);
    ui->plotTemp->yAxis->setLabel(label_temperatureAxis);
    ui->plotClocks->xAxis->setLabel(label_timeAxis);
    ui->plotClocks->yAxis->setLabel(label_clockAxis);
    ui->plotVolts->xAxis->setLabel(label_timeAxis);
    ui->plotVolts->yAxis->setLabel(label_voltageAxis);
    ui->plotTemp->xAxis->setTickLabels(false);
    ui->plotClocks->xAxis->setTickLabels(false);
    ui->plotVolts->xAxis->setTickLabels(false);

    ui->plotTemp->addGraph(); // temp graph
    ui->plotClocks->addGraph(); // core clock graph
    ui->plotClocks->addGraph(); // mem clock graph
    ui->plotClocks->addGraph(); // uvd
    ui->plotClocks->addGraph(); // uvd
    ui->plotVolts->addGraph(); // volts gpu
    ui->plotVolts->addGraph(); // volts mem

    ui->plotFanProfile->addGraph();
    ui->plotFanProfile->yAxis->setRange(0,100);
    ui->plotFanProfile->xAxis->setRange(0,110);
    ui->plotFanProfile->xAxis->setLabel(label_temperature);
    ui->plotFanProfile->yAxis->setLabel(label_fanSpeed);

    setupGraphsStyle();

    // legend clocks //
    ui->plotClocks->graph(0)->setName(label_currentGPUClock);
    ui->plotClocks->graph(1)->setName(label_currentMemClock);
    ui->plotClocks->graph(2)->setName(label_uvdVideoCoreClock);
    ui->plotClocks->graph(3)->setName(label_uvdDecoderClock);
    ui->plotClocks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotClocks->legend->setVisible(true);

    ui->plotVolts->graph(0)->setName(label_vddc);
    ui->plotVolts->graph(1)->setName(label_vddci);
    ui->plotVolts->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignLeft);
    ui->plotVolts->legend->setVisible(true);

    ui->plotClocks->replot();
    ui->plotTemp->replot();
    ui->plotVolts->replot();
}

void radeon_profile::setupGraphsStyle()
{
    QPen pen;
    pen.setWidth(ui->spin_lineThick->value());
    pen.setCapStyle(Qt::SquareCap);
    pen.setColor(ui->graphColorsList->topLevelItem(TEMP_LINE)->backgroundColor(1));
    ui->plotTemp->graph(0)->setPen(pen);
    ui->plotFanProfile->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->backgroundColor(1));
    ui->plotClocks->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->backgroundColor(1));
    ui->plotClocks->graph(1)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->backgroundColor(1));
    ui->plotClocks->graph(2)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->backgroundColor(1));
    ui->plotClocks->graph(3)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(CORE_VOLTS_LINE)->backgroundColor(1));
    ui->plotVolts->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(MEM_VOLTS_LINE)->backgroundColor(1));
    ui->plotVolts->graph(1)->setPen(pen);

    ui->plotTemp->setBackground(QBrush(ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1)));
    ui->plotClocks->setBackground(QBrush(ui->graphColorsList->topLevelItem(CLOCKS_BG)->backgroundColor(1)));
    ui->plotVolts->setBackground(QBrush(ui->graphColorsList->topLevelItem(VOLTS_BG)->backgroundColor(1)));
    ui->plotFanProfile->setBackground(QBrush(ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1)));
}

void radeon_profile::setupTrayIcon() {
    trayMenu = new QMenu();
    setWindowState(Qt::WindowMinimized);
    //close //
    closeApp = new QAction(this);
    closeApp->setText(label_quit);
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // standard profiles
    changeProfile = new QAction(this);
    changeProfile->setText(label_changeProfile);
    connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

    // refresh when hidden
    refreshWhenHidden = new QAction(this);
    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(true);
    refreshWhenHidden->setText(label_refreshWhenHidden);

    // dpm menu //
    dpmMenu = new QMenu(this);
    dpmMenu->setTitle(label_dpm);

    dpmSetBattery = new QAction(this);
    dpmSetBalanced = new QAction(this);
    dpmSetPerformance = new QAction(this);

    dpmSetBattery->setText(label_battery);
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText(label_performance);
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText(label_performance);
    dpmSetPerformance->setIcon(QIcon(":/icon/symbols/arrow3.png"));

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(on_btn_dpmBattery_clicked()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(on_btn_dpmBalanced_clicked()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(on_btn_dpmPerformance_clicked()));

    dpmMenu->addAction(dpmSetBattery);
    dpmMenu->addAction(dpmSetBalanced);
    dpmMenu->addAction(dpmSetPerformance);
    dpmMenu->addSeparator();
    dpmMenu->addMenu(forcePowerMenu);

    // add stuff above to menu //
    trayMenu->addAction(refreshWhenHidden);
    trayMenu->addSeparator();
    trayMenu->addAction(changeProfile);
    trayMenu->addMenu(dpmMenu);
    trayMenu->addSeparator();
    trayMenu->addAction(closeApp);

    // setup icon finally //
    QIcon appicon(":/icon/extra/radeon-profile.png");
    trayIcon = new QSystemTrayIcon(appicon,this);
    trayIcon->show();
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

void radeon_profile::setupOptionsMenu()
{
    optionsMenu = new QMenu(this);
    ui->btn_options->setMenu(optionsMenu);

    QAction *resetMinMax = new QAction(this);
    resetMinMax->setText(label_resetMinMax);

    QAction *resetGraphs = new QAction(this);
    resetGraphs->setText(label_resetGraphs);

    QAction *showLegend = new QAction(this);
    showLegend->setText(label_showLegend);
    showLegend->setCheckable(true);
    showLegend->setChecked(true);

    QAction *graphOffset = new QAction(this);
    graphOffset->setText(label_graphOffset);
    graphOffset->setCheckable(true);
    graphOffset->setChecked(true);

    optionsMenu->addAction(showLegend);
    optionsMenu->addAction(graphOffset);
    optionsMenu->addSeparator();
    optionsMenu->addAction(resetMinMax);
    optionsMenu->addAction(resetGraphs);

    connect(resetMinMax,SIGNAL(triggered()),this,SLOT(resetMinMax()));
    connect(resetGraphs,SIGNAL(triggered()),this,SLOT(resetGraphs()));
    connect(showLegend,SIGNAL(triggered(bool)),this,SLOT(showLegend(bool)));
    connect(graphOffset,SIGNAL(triggered(bool)),this,SLOT(setGraphOffset(bool)));
}

void radeon_profile::setupForcePowerLevelMenu() {
    forcePowerMenu = new QMenu(this);

    QAction *forceAuto = new QAction(this);
    forceAuto->setText(label_auto);

    QAction *forceLow = new QAction(this);
    forceLow->setText(label_low);

    QAction *forceHigh = new QAction(this);
    forceHigh->setText(label_high);

    forcePowerMenu->setTitle(label_forcePowerLevel);
    forcePowerMenu->addAction(forceAuto);
    forcePowerMenu->addSeparator();
    forcePowerMenu->addAction(forceLow);
    forcePowerMenu->addAction(forceHigh);

    connect(forceAuto,SIGNAL(triggered()),this,SLOT(forceAuto()));
    connect(forceLow,SIGNAL(triggered()),this,SLOT(forceLow()));
    connect(forceHigh,SIGNAL(triggered()),this,SLOT(forceHigh()));
}

void radeon_profile::setupContextMenus() {
    QAction *copyToClipboard = new QAction(this);
    copyToClipboard->setText(label_copyToClipboard);
    ui->list_glxinfo->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_glxinfo->addAction(copyToClipboard);
    connect(copyToClipboard, SIGNAL(triggered()),this,SLOT(copyGlxInfoToClipboard()));

    QAction * copyConnectors = new QAction(this);
    copyConnectors->setText(label_copyToClipboard);
    ui->list_connectors->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_connectors->addAction(copyConnectors);
    connect(copyConnectors, SIGNAL(triggered()), this, SLOT(copyConnectorsToClipboard()));

    QAction *reset = new QAction(this);
    reset->setText(label_resetStatistics);
    ui->list_stats->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_stats->addAction(reset);
    connect(reset,SIGNAL(triggered()),this,SLOT(resetStats()));
}
//========

