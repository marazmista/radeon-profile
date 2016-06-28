// copyright marazmista @ 29.12.2013

// this file contains functions for creating some gui elements (menus, graphs)

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMenu>
#include <QDir>

//===================================
// === GUI setup functions === //
void radeon_profile::setupGraphs()
{
    qDebug() << "Setting up graphs";
    ui->plotClocks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);

    ui->plotTemp->xAxis->setLabel(tr("Time (s)"));
    ui->plotTemp->yAxis->setLabel(tr("Temperature") + celsius);
    ui->plotClocks->xAxis->setLabel(tr("Time (s)"));
    ui->plotClocks->yAxis->setLabel(tr("Clock (MHz)"));
    ui->plotVolts->xAxis->setLabel(tr("Time (s)"));
    ui->plotVolts->yAxis->setLabel(tr("Voltage (mV)"));
    ui->plotTemp->xAxis->setTickLabels(false);
    ui->plotClocks->xAxis->setTickLabels(false);
    ui->plotVolts->xAxis->setTickLabels(false);

    QCP::PlottingHints fastReplot = QCP::phCacheLabels | QCP::phFastPolylines;
    ui->plotTemp->setPlottingHints(fastReplot);
    ui->plotClocks->setPlottingHints(fastReplot);
    ui->plotVolts->setPlottingHints(fastReplot);

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
    ui->plotFanProfile->xAxis->setLabel(tr("Temperature") + celsius);
    ui->plotFanProfile->yAxis->setLabel(tr("Fan speed"));

    setupGraphsStyle();

    // legend clocks //
    ui->plotClocks->graph(0)->setName(tr("GPU clock"));
    ui->plotClocks->graph(1)->setName(tr("Memory clock"));
    ui->plotClocks->graph(2)->setName(tr("UVD core clock"));
    ui->plotClocks->graph(3)->setName(tr("UVD decoder clock"));
    ui->plotClocks->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    ui->plotClocks->legend->setVisible(true);

    ui->plotVolts->graph(0)->setName(tr("GPU voltage"));
    ui->plotVolts->graph(1)->setName(tr("I/O voltage"));
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

    ui->plotTemp->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->plotClocks->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plotClocks->graph(1)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plotClocks->graph(2)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plotClocks->graph(3)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plotVolts->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plotVolts->graph(1)->setLineStyle(QCPGraph::lsStepCenter);
}

void radeon_profile::setupTrayIcon() {
    qDebug() << "Setting up tray icon";

    // Unity on Ubuntu 14.04 and earlier does not support tray icon
    const QRegExp filter = QRegExp("VERSION_ID=\"(\\d+).\\d+\"");
    const QStringList release = globalStuff::grabSystemInfo("cat /etc/os-release"),
            versionID = release.filter(filter),
            ps = globalStuff::grabSystemInfo("ps -A").filter(QRegExp(".*unity.*"));
    if( ! ps.isEmpty() // Unity is the desktop enviroinment
            && release.contains("ID=ubuntu") // This check works only for ubuntu
            && ! versionID.isEmpty()){ // Release version is available
            filter.indexIn(versionID[0]);
            ushort version = filter.cap(1).toUShort();
            trayIconAvailable = version > 14;
            qDebug() << "Ubuntu " << version << ", trayIconAvailable: " << trayIconAvailable;
    } else
        trayIconAvailable = true;

    ui->cb_minimizeTray->setEnabled(trayIconAvailable);
    ui->cb_closeTray->setEnabled(trayIconAvailable);
    if( ! trayIconAvailable){
        qWarning() << "Tray icon is not supported on this system";
        return;
    }

    // maximize
    trayBtn_show = new QAction(this);
    trayBtn_show->setText(tr("Show"));
    connect(trayBtn_show, SIGNAL(triggered()),this,SLOT(show()));

    //close //
    closeApp = new QAction(this);
    closeApp->setText(tr("Quit"));
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // standard profiles
    changeProfile = new QAction(this);
    changeProfile->setText(tr("Change standard profile"));
    connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

    // refresh when hidden
    refreshWhenHidden = new QAction(this);
    refreshWhenHidden->setCheckable(true);
    refreshWhenHidden->setChecked(true);
    refreshWhenHidden->setText(tr("Keep refreshing when hidden"));

    // dpm menu //
    dpmMenu.setTitle(tr("DPM"));

    dpmSetBattery = new QAction(this);
    dpmSetBalanced = new QAction(this);
    dpmSetPerformance = new QAction(this);

    dpmSetBattery->setText(tr("Battery"));
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText(tr("Balanced"));
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText(tr("Performance"));
    dpmSetPerformance->setIcon(QIcon(":/icon/symbols/arrow3.png"));

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(on_btn_dpmBattery_clicked()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(on_btn_dpmBalanced_clicked()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(on_btn_dpmPerformance_clicked()));

    dpmMenu.addAction(dpmSetBattery);
    dpmMenu.addAction(dpmSetBalanced);
    dpmMenu.addAction(dpmSetPerformance);
    dpmMenu.addSeparator();
    dpmMenu.addMenu(&forcePowerMenu);

    // add stuff above to menu //
    trayMenu.addAction(refreshWhenHidden);
    trayMenu.addSeparator();
    trayMenu.addAction(changeProfile);
    trayMenu.addMenu(&dpmMenu);
    trayMenu.addSeparator();
    trayMenu.addAction(trayBtn_show);
    trayMenu.addAction(closeApp);

    // setup icon finally //
    trayIcon.setIcon(QIcon(":/icon/extra/radeon-profile.png"));
    trayIcon.show();
    trayIcon.setContextMenu(&trayMenu);
    connect(&trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

void radeon_profile::setupExportDialog(){
    qDebug() << "Setting up exportDialog";
    exportUi.setupUi(&exportDialog);
    exportUi.directory->setText(QDir::homePath());
    connect(&exportDialog, SIGNAL(accepted()), this, SLOT(exportGraphs()));

    exportFormat.addButton(exportUi.rb_pdf);
    exportFormat.addButton(exportUi.rb_png);
    exportFormat.addButton(exportUi.rb_jpg);
    exportFormat.addButton(exportUi.rb_bmp);
    exportFormat.setExclusive(true);

    connect(exportUi.btn_changeDirectory, SIGNAL(clicked()), this, SLOT(changeExportDirectory()));
}

void radeon_profile::setupOptionsMenu()
{
    qDebug() << "Setting up options menu";
    ui->btn_options->setMenu(&optionsMenu);

    QAction *resetMinMax = new QAction(this);
    resetMinMax->setText(tr("Reset min/max temperature"));

    QAction *resetGraphs = new QAction(this);
    resetGraphs->setText(tr("Reset graphs vertical scale"));

    QAction *showLegend = new QAction(this);
    showLegend->setText(tr("Show legend"));
    showLegend->setCheckable(true);
    showLegend->setChecked(true);

    QAction *graphOffset = new QAction(this);
    graphOffset->setText(tr("Graph offset on right"));
    graphOffset->setCheckable(true);
    graphOffset->setChecked(true);

    optionsMenu.addAction(showLegend);
    optionsMenu.addAction(graphOffset);
    optionsMenu.addSeparator();
    optionsMenu.addAction(resetMinMax);
    optionsMenu.addAction(resetGraphs);

    connect(resetMinMax,SIGNAL(triggered()),this,SLOT(resetMinMax()));
    connect(resetGraphs,SIGNAL(triggered()),this,SLOT(resetGraphs()));
    connect(showLegend,SIGNAL(triggered(bool)),this,SLOT(showLegend(bool)));
    connect(graphOffset,SIGNAL(triggered(bool)),this,SLOT(setGraphOffset(bool)));
}

void radeon_profile::setupForcePowerLevelMenu() {

    qDebug() << "Setting up force power level menu";
    QAction *forceAuto = new QAction(this);
    forceAuto->setText(tr("Auto"));

    QAction *forceLow = new QAction(this);
    forceLow->setText(tr("Low"));

    QAction *forceHigh = new QAction(this);
    forceHigh->setText(tr("High"));

    forcePowerMenu.setTitle(tr("Force power level"));
    forcePowerMenu.addAction(forceAuto);
    forcePowerMenu.addSeparator();
    forcePowerMenu.addAction(forceLow);
    forcePowerMenu.addAction(forceHigh);

    connect(forceAuto,SIGNAL(triggered()),this,SLOT(forceAuto()));
    connect(forceLow,SIGNAL(triggered()),this,SLOT(forceLow()));
    connect(forceHigh,SIGNAL(triggered()),this,SLOT(forceHigh()));
}

void radeon_profile::setupContextMenus() {
    qDebug() << "Setting up context menus";
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
//========

