// copyright marazmista @ 29.12.2013

// this file contains functions for creating some gui elements (menus, graphs)

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMenu>

//===================================
// === GUI setup functions === //
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

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(setBattery()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(setBalanced()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(setPerformance()));

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

void radeon_profile::createGeneralMenu() {
    generalMenu = new QMenu(this);
    ui->btn_general->setMenu(generalMenu);

    QAction *pause = new QAction(generalMenu);
    pause->setCheckable(true);
    pause->setText(tr("Pause refresh temporairly"));
    pause->setIcon(QIcon(":/icon/symbols/pause.png"));
    connect(pause, SIGNAL(toggled(bool)), this,SLOT(pauseRefresh(bool)));

    QAction *resetTemp = new QAction(generalMenu);
    resetTemp->setText(tr("Reset min and max temperatures"));
    connect(resetTemp,SIGNAL(triggered()), this, SLOT(resetMinMax()));

    generalMenu->addAction(pause);
    generalMenu->addSeparator();
    generalMenu->addAction(resetTemp);
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
    fanFixed->setText(tr("Fixed ") + ui->labelFixedSpeed->text());
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

    ui->btn_fanControl->setMenu(fanProfilesMenu);
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

void setupAxis(QValueAxis* axis, QColor color, QString title) {
    axis->setGridLineColor(color);
    axis->setGridLineColor(color);
    axis->setLabelsColor(color);
    axis->setTitleText(title);
    axis->setTitleBrush(QBrush(color));
    axis->setLabelFormat("%d");
}

void setupSeries(QLineSeries *series, QColor color, QString name, QValueAxis *axisBottom, QValueAxis *axisSide) {
    series->setColor(color);
    series->setName(name);
    series->setPointsVisible(true);

    series->attachAxis(axisBottom);
    series->attachAxis(axisSide);
}

void setupChart(QChart *chart, bool legendVisable) {
    chart->setBackgroundRoundness(2);
    chart->setMargins(QMargins(5,0,0,0));
    chart->legend()->setVisible(legendVisable);

    chart->setBackgroundBrush(QBrush(Qt::darkGray));
}

void radeon_profile::addRuntimeWidgets() {
    // add button for manual refresh glx info, connectors, mod params
    QPushButton *refreshBtn = new QPushButton();
    refreshBtn->setIcon(QIcon(":/icon/symbols/refresh.png"));
    ui->tw_systemInfo->setCornerWidget(refreshBtn);
    refreshBtn->setIconSize(QSize(20,20));
    refreshBtn->show();
    connect(refreshBtn,SIGNAL(clicked()),this,SLOT(refreshBtnClicked()));

    ui->label_version->setText(tr("version %n", NULL, appVersion));

    // version label
    QLabel *l = new QLabel("v. " +QString().setNum(appVersion),this);
    QFont f;
    f.setStretch(QFont::Unstretched);
    f.setWeight(QFont::Bold);
    f.setPointSize(8);
    l->setFont(f);
    ui->tw_main->setCornerWidget(l,Qt::BottomRightCorner);
    l->show();

    // button on exec pages
    QPushButton *btnBackProfiles = new QPushButton();
    btnBackProfiles->setText(tr("Back to profiles"));
    ui->tabs_execOutputs->setCornerWidget(btnBackProfiles);
    btnBackProfiles->show();
    connect(btnBackProfiles,SIGNAL(clicked()),this,SLOT(btnBackToProfilesClicked()));

    // set pwm buttons in group
    pwmGroup.addButton(ui->btn_pwmAuto);
    pwmGroup.addButton(ui->btn_pwmFixed);
    pwmGroup.addButton(ui->btn_pwmProfile);

    dpmGroup.addButton(ui->btn_dpmBattery, PowerProfiles::BATTERY);
    dpmGroup.addButton(ui->btn_dpmBalanced, PowerProfiles::BALANCED);
    dpmGroup.addButton(ui->btn_dpmPerformance, PowerProfiles::PERFORMANCE);

    //setup fan profile graph
    fanProfileChart = new QChartView(this);
    QChart *fanChart = new QChart();
    fanProfileChart->setRenderHint(QPainter::Antialiasing);
    fanProfileChart->setChart(fanChart);

    setupChart(fanChart, false);

    fanSeries = new QLineSeries(fanChart);
    fanChart->addSeries(fanSeries);

    QValueAxis *axisTemperature = new QValueAxis(fanChart);
    QValueAxis *axisSpeed = new QValueAxis(fanChart);
    fanChart->addAxis(axisTemperature,Qt::AlignBottom);
    fanChart->addAxis(axisSpeed, Qt::AlignLeft);
    axisTemperature->setRange(0, 100);
    axisSpeed->setRange(0, 100);

    setupAxis(axisSpeed, Qt::white, tr("Fan Speed [%]"));
    setupAxis(axisTemperature, Qt::white, tr("Temperature [°C]"));
    setupSeries(fanSeries, Qt::yellow, "", axisSpeed, axisTemperature);

    ui->verticalLayout_22->addWidget(fanProfileChart);

    //setup oc table graph
    ocTableChart = new QChartView(this);
    QChart *ocChart = new QChart();
    ocTableChart->setRenderHint(QPainter::Antialiasing);
    ocTableChart->setChart(ocChart);

    setupChart(ocChart, true);

    ocClockFreqSeries = new QLineSeries(ocChart);
    ocMemFreqkSeries = new QLineSeries(ocChart);
    ocCoreVoltSeries = new QLineSeries(ocChart);
    ocMemVoltSeries = new QLineSeries(ocChart);

    ocChart->addSeries(ocClockFreqSeries);
    ocChart->addSeries(ocMemFreqkSeries);
    ocChart->addSeries(ocCoreVoltSeries);
    ocChart->addSeries(ocMemVoltSeries);

    axisState = new QValueAxis(ocChart);
    axisFrequency = new QValueAxis(ocChart);
    axisVolts = new QValueAxis(ocChart);

    ocChart->addAxis(axisState,Qt::AlignBottom);
    ocChart->addAxis(axisFrequency, Qt::AlignLeft);
    ocChart->addAxis(axisVolts, Qt::AlignRight);

    setupAxis(axisState, Qt::white, tr("State"));
    setupAxis(axisFrequency, Qt::white, tr("Frequency [MHz]"));
    setupAxis(axisVolts, Qt::white, tr("Voltage [mV]"));

    setupSeries(ocClockFreqSeries, Qt::yellow, tr("Core frequency [MHz]"), axisState, axisFrequency);
    setupSeries(ocMemFreqkSeries, Qt::blue, tr("Memory Voltage [MHz]"), axisState, axisFrequency);
    setupSeries(ocCoreVoltSeries, Qt::green, tr("Core Voltage [mV]"), axisState, axisVolts);
    setupSeries(ocMemVoltSeries, Qt::cyan, tr("Memory Voltage [mV]"), axisState, axisVolts);

    ui->verticalLayout_10->addWidget(ocTableChart);

    ui->list_coreStates->resizeColumnToContents(0);
    ui->list_memStates->resizeColumnToContents(0);
}
