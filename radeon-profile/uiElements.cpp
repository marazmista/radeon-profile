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

    ui->plotTemp->xAxis->setLabel(tr("Time (s)"));
    ui->plotTemp->yAxis->setLabel(tr("Temperature (°C)"));
    ui->plotClocks->xAxis->setLabel(tr("Time (s)"));
    ui->plotClocks->yAxis->setLabel(tr("Clock (MHz)"));
    ui->plotVolts->xAxis->setLabel(tr("Time (s)"));
    ui->plotVolts->yAxis->setLabel(tr("Voltage (mV)"));
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
    ui->plotFanProfile->xAxis->setLabel(tr("Temperature"));
    ui->plotFanProfile->yAxis->setLabel(tr("Fan speed"));

    setupGraphsStyle();

    // legend clocks //
    ui->plotClocks->graph(0)->setName(tr("GPU clock"));
    ui->plotClocks->graph(1)->setName(tr("Memory clock"));
    ui->plotClocks->graph(2)->setName(tr("UVD core clock (cclk)"));
    ui->plotClocks->graph(3)->setName(tr("UVD decoder clock (dclk)"));
    ui->plotClocks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotClocks->legend->setVisible(true);

    ui->plotVolts->graph(0)->setName(tr("GPU voltage (vddc)"));
    ui->plotVolts->graph(1)->setName(tr("I/O voltage (vddci)"));
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
    trayMenu = new QMenu(this);
    setWindowState(Qt::WindowMinimized);
    //close //
    closeApp = new QAction(trayMenu);
    closeApp->setText(tr("Quit"));
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // standard profiles
    changeProfile = new QAction(trayMenu);
    changeProfile->setText(tr("Change standard profile"));
    connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

    // refresh when hidden
    refreshWhenHidden = new QAction(trayMenu);
    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(true);
    refreshWhenHidden->setText(tr("Keep refreshing when hidden"));

    // dpm menu //
    dpmMenu = new QMenu(this);
    dpmMenu->setTitle(tr("DPM"));

    dpmSetBattery = new QAction(dpmMenu);
    dpmSetBalanced = new QAction(dpmMenu);
    dpmSetPerformance = new QAction(dpmMenu);

    dpmSetBattery->setText(tr("Battery"));
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText(tr("Balanced"));
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText(tr("Performance"));
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

    QAction *resetMinMax = new QAction(optionsMenu);
    resetMinMax->setText(tr("Reset min/max temperature"));

    QAction *resetGraphs = new QAction(optionsMenu);
    resetGraphs->setText(tr("Reset graphs vertical scale"));

    QAction *showLegend = new QAction(optionsMenu);
    showLegend->setText(tr("Show legend"));
    showLegend->setCheckable(true);
    showLegend->setChecked(true);

    QAction *graphOffset = new QAction(optionsMenu);
    graphOffset->setText(tr("Graph offset on right"));
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

    QAction *forceAuto = new QAction(forcePowerMenu);
    forceAuto->setText(tr("Auto"));

    QAction *forceLow = new QAction(forcePowerMenu);
    forceLow->setText(tr("Low"));

    QAction *forceHigh = new QAction(forcePowerMenu);
    forceHigh->setText(tr("High"));

    forcePowerMenu->setTitle(tr("Force power level"));
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
    copyToClipboard->setText(tr("Copy to clipboard"));
    ui->list_glxinfo->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_glxinfo->addAction(copyToClipboard);
    connect(copyToClipboard, SIGNAL(triggered()),this,SLOT(copyGlxInfoToClipboard()));

    QAction * copyConnectors = new QAction(this);
    copyConnectors->setText(tr("Copy to clipboard"));
    ui->list_connectors->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_connectors->addAction(copyConnectors);
    connect(copyConnectors, SIGNAL(triggered()), this, SLOT(copyConnectorsToClipboard()));

    QAction *reset = new QAction(this);
    reset->setText(tr("Reset statistics"));
    ui->list_stats->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_stats->addAction(reset);
    connect(reset,SIGNAL(triggered()),this,SLOT(resetStats()));
}


void radeon_profile::setupFanProfilesMenu(const bool rebuildMode) {
    if (rebuildMode)
        delete fanProfilesMenu;

    fanProfilesMenu = new QMenu(this);
    connect(fanProfilesMenu, SIGNAL(triggered(QAction*)), this, SLOT(fanProfileMenuActionClicked(QAction*)));
    QActionGroup *ag = new QActionGroup(fanProfilesMenu);

    QAction *fanAuto = new QAction(fanProfilesMenu);
    fanAuto->setText(tr("Auto"));
    fanAuto->setCheckable(true);
    fanAuto->setChecked(true);
    fanAuto->setActionGroup(ag);
    connect(fanAuto, SIGNAL(triggered()), this, SLOT(on_btn_pwmAuto_clicked()));

    QAction *fanFixed = new QAction(fanProfilesMenu);
    fanFixed->setText(tr("Fixed ") + ui->labelFixedSpeed->text()+"%");
    fanFixed->setCheckable(true);
    fanFixed->setActionGroup(ag);
    connect(fanFixed, SIGNAL(triggered()), this, SLOT(on_btn_pwmFixed_clicked()));

    fanProfilesMenu->addAction(fanAuto);
    fanProfilesMenu->addAction(fanFixed);

    fanProfilesMenu->addSeparator();

    for (QString p : fanProfiles.keys()) {
        QAction *a = new QAction(fanProfilesMenu);
        a->setText(p);
        a->setCheckable(true);
        a->setActionGroup(ag);
        fanProfilesMenu->addAction(a);
    }

    ui->l_fanSpeed->setMenu(fanProfilesMenu);
}

void radeon_profile::fillConnectors(){
    ui->list_connectors->clear();
    ui->list_connectors->addTopLevelItems(device.getCardConnectors());
    ui->list_connectors->expandToDepth(2);
    ui->list_connectors->header()->resizeSections(QHeaderView::ResizeToContents);
}

void radeon_profile::fillModInfo(){
    ui->list_modInfo->clear();
    ui->list_modInfo->addTopLevelItems(device.getModuleInfo());
    ui->list_modInfo->header()->resizeSections(QHeaderView::ResizeToContents);
}
