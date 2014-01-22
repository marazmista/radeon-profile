// copyright marazmista @ 29.12.2013

// this file contains functions for creating some gui elements (menus, graphs)

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMenu>

//===================================
// === GUI setup functions === //
void radeon_profile::setupGraphs()
{
    ui->plotColcks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);

    ui->plotTemp->xAxis->setLabel("time");
    ui->plotTemp->yAxis->setLabel("temperature");
    ui->plotColcks->xAxis->setLabel("time");
    ui->plotColcks->yAxis->setLabel("MHz");
    ui->plotVolts->xAxis->setLabel("time");
    ui->plotVolts->yAxis->setLabel("mV");
    ui->plotTemp->xAxis->setTickLabels(false);
    ui->plotColcks->xAxis->setTickLabels(false);
    ui->plotVolts->xAxis->setTickLabels(false);

    ui->plotTemp->addGraph(); // temp graph
    ui->plotColcks->addGraph(); // core clock graph
    ui->plotColcks->addGraph(); // mem clock graph
    ui->plotColcks->addGraph(); // uvd
    ui->plotColcks->addGraph(); // uvd
    ui->plotVolts->addGraph(); // volts gpu
    ui->plotVolts->addGraph(); // volts mem

    setupGraphsStyle();

    // legend clocks //
    ui->plotColcks->graph(0)->setName("GPU clock");
    ui->plotColcks->graph(1)->setName("Memory clock");
    ui->plotColcks->graph(2)->setName("Video core clock");
    ui->plotColcks->graph(3)->setName("Decoder clock");
    ui->plotColcks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotColcks->legend->setVisible(true);

    ui->plotVolts->graph(0)->setName("GPU voltage");
    ui->plotVolts->graph(1)->setName("Memory voltage");
    ui->plotVolts->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignLeft);
    ui->plotVolts->legend->setVisible(true);

    ui->plotColcks->replot();
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
    pen.setColor(ui->graphColorsList->topLevelItem(GPU_CLOCK_LINE)->backgroundColor(1));
    ui->plotColcks->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(MEM_CLOCK_LINE)->backgroundColor(1));
    ui->plotColcks->graph(1)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_VIDEO_LINE)->backgroundColor(1));
    ui->plotColcks->graph(2)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(UVD_DECODER_LINE)->backgroundColor(1));
    ui->plotColcks->graph(3)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(CORE_VOLTS_LINE)->backgroundColor(1));
    ui->plotVolts->graph(0)->setPen(pen);
    pen.setColor(ui->graphColorsList->topLevelItem(MEM_VOLTS_LINE)->backgroundColor(1));
    ui->plotVolts->graph(1)->setPen(pen);

    ui->plotTemp->setBackground(QBrush(ui->graphColorsList->topLevelItem(TEMP_BG)->backgroundColor(1)));
    ui->plotColcks->setBackground(QBrush(ui->graphColorsList->topLevelItem(CLOCKS_BG)->backgroundColor(1)));
    ui->plotVolts->setBackground(QBrush(ui->graphColorsList->topLevelItem(VOLTS_BG)->backgroundColor(1)));
}

void radeon_profile::setupTrayIcon() {
    trayMenu = new QMenu();
    setWindowState(Qt::WindowMinimized);
    //close //
    closeApp = new QAction(this);
    closeApp->setText("Quit");
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // standard profiles
    changeProfile = new QAction(this);
    changeProfile->setText("Change standard profile");
    connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

    // refresh when hidden
    refreshWhenHidden = new QAction(this);
    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(true);
    refreshWhenHidden->setText("Keep refreshing when hidden");

    // dpm menu //
    dpmMenu = new QMenu(this);
    dpmMenu->setTitle("DPM");

    dpmSetBattery = new QAction(this);
    dpmSetBalanced = new QAction(this);
    dpmSetPerformance = new QAction(this);

    dpmSetBattery->setText("Battery");
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText("Balanced");
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText("Performance");
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
    resetMinMax->setText("Reset min/max temperature");

    QAction *resetGraphs = new QAction(this);
    resetGraphs->setText("Reset graphs vertical scale");

    QAction *showLegend = new QAction(this);
    showLegend->setText("Show legend");
    showLegend->setCheckable(true);
    showLegend->setChecked(true);

    optionsMenu->addAction(showLegend);
    optionsMenu->addSeparator();
    optionsMenu->addAction(resetMinMax);
    optionsMenu->addAction(resetGraphs);

    connect(resetMinMax,SIGNAL(triggered()),this,SLOT(resetMinMax()));
    connect(resetGraphs,SIGNAL(triggered()),this,SLOT(resetGraphs()));
    connect(showLegend,SIGNAL(triggered(bool)),this,SLOT(showLegend(bool)));
}

void radeon_profile::setupForcePowerLevelMenu() {
    forcePowerMenu = new QMenu(this);

    QAction *forceAuto = new QAction(this);
    forceAuto->setText("Auto");

    QAction *forceLow = new QAction(this);
    forceLow->setText("Low");

    QAction *forceHigh = new QAction(this);
    forceHigh->setText("High");

    forcePowerMenu->setTitle("Force power level");
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
    copyToClipboard->setText("Copy to clipboard");

    ui->list_glxinfo->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_glxinfo->addAction(copyToClipboard);

    QAction *reset = new QAction(this);
    reset->setText("Reset statistics");

    ui->list_stats->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_stats->addAction(reset);

    connect(copyToClipboard, SIGNAL(triggered()),this,SLOT(copyGlxInfoToClipboard()));
    connect(reset,SIGNAL(triggered()),this,SLOT(resetStats()));
}
//========

