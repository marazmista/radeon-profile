// copyright marazmista @ 29.12.2013

// this file contains functions for creating some gui elements (menus, graphs)

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMenu>

//===================================
// === GUI setup functions === //
void radeon_profile::addPowerMethodToTrayMenu(const DriverFeatures &features)
{
    auto menu = icon_tray->contextMenu();

    if (features.currentPowerMethod == PowerMethod::DPM)
        menu->insertMenu(menu->actions()[1], createDpmMenu());
    else if (features.currentPowerMethod == PowerMethod::PROFILE) {
        QAction *changeProfile = new QAction(menu);
        changeProfile->setText(tr("Change standard profile"));
        connect(changeProfile,SIGNAL(triggered()),this,SLOT(on_chProfile_clicked()));

        menu->insertAction(menu->actions()[1], changeProfile);
    }
}

void radeon_profile::setupTrayIcon() {
    QMenu *menu_tray = new QMenu(this);
    setWindowState(Qt::WindowMinimized);
    //close //
    QAction *closeApp = new QAction(menu_tray);
    closeApp->setText(tr("Quit"));
    connect(closeApp,SIGNAL(triggered()),this,SLOT(closeFromTray()));

    // refresh when hidden
    refreshWhenHidden->setText(tr("Keep refreshing when hidden"));

    // add stuff to menu //
    menu_tray->addAction(refreshWhenHidden);

    menu_tray->addSeparator();
    menu_tray->addAction(closeApp);

    // setup icon finally //
    QIcon appicon = QIcon::fromTheme("radeon-profile-tray", QIcon(":/icon/extra/radeon-profile.png"));
    icon_tray = new QSystemTrayIcon(appicon,this);
    icon_tray->show();
    icon_tray->setContextMenu(menu_tray);
    connect(icon_tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

QMenu* radeon_profile::createGeneralMenu() {
    QMenu *menu_general = new QMenu(this);

    QAction *pause = new QAction(menu_general);
    pause->setCheckable(true);
    pause->setText(tr("Pause refresh temporarily"));
    pause->setIcon(QIcon(":/icon/symbols/pause.png"));
    connect(pause, SIGNAL(toggled(bool)), this,SLOT(pauseRefresh(bool)));

    QAction *resetTemp = new QAction(menu_general);
    resetTemp->setText(tr("Reset min and max temperatures"));
    connect(resetTemp,SIGNAL(triggered()), this, SLOT(resetMinMax()));

    menu_general->addAction(pause);
    menu_general->addSeparator();
    menu_general->addAction(resetTemp);

    return menu_general;
}

QMenu* radeon_profile::createDpmMenu() {
    QMenu *menu_dpm = new QMenu(this);
    menu_dpm->setTitle(tr("DPM"));

    QMenu *menu_forcePower = new QMenu(this);
    menu_forcePower->setTitle(tr("Force power level"));

    QAction *dpmSetBattery = new QAction(menu_dpm);
    QAction *dpmSetBalanced = new QAction(menu_dpm);
    QAction *dpmSetPerformance = new QAction(menu_dpm);

    dpmSetBattery->setText(tr("Battery"));
    dpmSetBattery->setIcon(QIcon(":/icon/symbols/arrow1.png"));
    dpmSetBalanced->setText(tr("Balanced"));
    dpmSetBalanced->setIcon(QIcon(":/icon/symbols/arrow2.png"));
    dpmSetPerformance->setText(tr("Performance"));
    dpmSetPerformance->setIcon(QIcon(":/icon/symbols/arrow3.png"));

    connect(dpmSetBattery,SIGNAL(triggered()),this,SLOT(setBattery()));
    connect(dpmSetBalanced,SIGNAL(triggered()),this, SLOT(setBalanced()));
    connect(dpmSetPerformance,SIGNAL(triggered()),this,SLOT(setPerformance()));


    QAction *forceAuto = new QAction(menu_forcePower);
    forceAuto->setText(tr("Auto"));

    QAction *forceLow = new QAction(menu_forcePower);
    forceLow->setText(tr("Low"));

    QAction *forceHigh = new QAction(menu_forcePower);
    forceHigh->setText(tr("High"));

    menu_forcePower->addAction(forceAuto);
    menu_forcePower->addSeparator();
    menu_forcePower->addAction(forceLow);
    menu_forcePower->addAction(forceHigh);

    connect(forceAuto,SIGNAL(triggered()),this,SLOT(forceAuto()));
    connect(forceLow,SIGNAL(triggered()),this,SLOT(forceLow()));
    connect(forceHigh,SIGNAL(triggered()),this,SLOT(forceHigh()));

    // add all items to menu
    menu_dpm->addAction(dpmSetBattery);
    menu_dpm->addAction(dpmSetBalanced);
    menu_dpm->addAction(dpmSetPerformance);
    menu_dpm->addSeparator();
    menu_dpm->addMenu(menu_forcePower);

    return menu_dpm;
}

void radeon_profile::addRuntmeWidgets() {
    QAction *copyToClipboard = new QAction(this);
    copyToClipboard->setText(tr("Copy to clipboard"));
    ui->list_glxinfo->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_glxinfo->addAction(copyToClipboard);
    connect(copyToClipboard, SIGNAL(triggered()),this,SLOT(copyGlxInfoToClipboard()));

    QAction *copyConnectors = new QAction(this);
    copyConnectors->setText(tr("Copy to clipboard"));
    ui->list_connectors->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_connectors->addAction(copyConnectors);
    connect(copyConnectors, SIGNAL(triggered()), this, SLOT(copyConnectorsToClipboard()));

    QAction *reset = new QAction(this);
    reset->setText(tr("Reset statistics"));
    ui->list_stats->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->list_stats->addAction(reset);
    connect(reset,SIGNAL(triggered()),this,SLOT(resetStats()));

    // add button for manual refresh glx info, connectors, mod params
    QPushButton *refreshBtn = new QPushButton(this);
    refreshBtn->setIcon(QIcon(":/icon/symbols/refresh.png"));
    ui->tw_systemInfo->setCornerWidget(refreshBtn);
    refreshBtn->setIconSize(QSize(20,20));
    refreshBtn->show();
    connect(refreshBtn,SIGNAL(clicked()),this,SLOT(refreshBtnClicked()));

    ui->label_version->setText(tr("version %n", NULL, appVersion));

    // version label
    QLabel *l = new QLabel("v. " +QString::number(appVersion),this);
    QFont f;
    f.setStretch(QFont::Unstretched);
    f.setWeight(QFont::Bold);
    f.setPointSize(8);
    l->setFont(f);
    ui->tw_main->setCornerWidget(l,Qt::BottomRightCorner);
    l->show();

    // button on exec pages
    QPushButton *btnBackProfiles = new QPushButton(this);
    btnBackProfiles->setText(tr("Back to profiles"));
    ui->tabs_execOutputs->setCornerWidget(btnBackProfiles);
    btnBackProfiles->show();
    connect(btnBackProfiles,SIGNAL(clicked()),this,SLOT(btnBackToProfilesClicked()));
}

void radeon_profile::createFanProfilesMenu(const bool rebuildMode) {
    if (rebuildMode && ui->btn_fanControl->menu() != nullptr)
        delete ui->btn_fanControl->menu();

    auto menu_fanProfiles = new QMenu(this);
    connect(menu_fanProfiles, SIGNAL(triggered(QAction*)), this, SLOT(fanProfileMenuActionClicked(QAction*)));
    QActionGroup *ag = new QActionGroup(menu_fanProfiles);

    QAction *fanAuto = new QAction(menu_fanProfiles);
    fanAuto->setText(tr("Auto"));
    fanAuto->setCheckable(true);
    fanAuto->setChecked(true);
    fanAuto->setActionGroup(ag);
    connect(fanAuto, SIGNAL(triggered()), this, SLOT(on_btn_pwmAuto_clicked()));

    QAction *fanFixed = new QAction(menu_fanProfiles);
    fanFixed->setText(tr("Fixed ") + ui->spin_fanFixedSpeed->text());
    fanFixed->setCheckable(true);
    fanFixed->setActionGroup(ag);
    connect(fanFixed, SIGNAL(triggered()), this, SLOT(on_btn_pwmFixed_clicked()));

    menu_fanProfiles->addAction(fanAuto);
    menu_fanProfiles->addAction(fanFixed);

    menu_fanProfiles->addSeparator();

    for (QString p : fanProfiles.keys()) {
        QAction *a = new QAction(menu_fanProfiles);
        a->setText(p);
        a->setCheckable(true);
        a->setActionGroup(ag);
        menu_fanProfiles->addAction(a);
    }

    ui->btn_fanControl->setMenu(menu_fanProfiles);
}

void radeon_profile::createOcProfilesMenu(const bool rebuildMode) {
    if (rebuildMode && ui->btn_ocProfileControl->menu() != nullptr)
        delete ui->btn_ocProfileControl->menu();

    auto menu = new QMenu(this);
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(ocProfilesMenuActionClicked(QAction*)));

    auto ag = new QActionGroup(menu);

    for (const auto &p : ocProfiles.keys()) {
        auto a = new QAction(menu);
        a->setText(p);
        a->setCheckable(true);
        a->setActionGroup(ag);
        menu->addAction(a);
    }

    ui->btn_ocProfileControl->setMenu(menu);
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

void setupAxis(QValueAxis* axis, const QColor color, const QString title, unsigned tickCount = 7) {
    axis->setGridLineColor(color);
    axis->setGridLineColor(color);
    axis->setLabelsColor(color);
    axis->setTitleText(title);
    axis->setTitleBrush(QBrush(color));
    axis->setLabelFormat("%d");
    axis->setTickCount(tickCount);
}

void setupSeries(QLineSeries *series, const QColor color, const QString name, QValueAxis *axisBottom, QValueAxis *axisSide) {
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
    chart->legend()->setLabelColor(Qt::white);
    chart->setBackgroundBrush(QBrush(Qt::darkGray));
}

void radeon_profile::addDpmButtons()
{
    group_Dpm.addButton(ui->btn_dpmBattery, PowerProfiles::BATTERY);
    group_Dpm.addButton(ui->btn_dpmBalanced, PowerProfiles::BALANCED);
    group_Dpm.addButton(ui->btn_dpmPerformance, PowerProfiles::PERFORMANCE);
}

void radeon_profile::addPowerProfileModesButons(const PowerProfileModes &modes) {
    for (const auto &ppm : modes) {
        QToolButton *btn_mode = new QToolButton(this);

        btn_mode->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        btn_mode->setText(ppm.name);
        btn_mode->setCheckable(true);
        btn_mode->setChecked(ppm.isActive);

        // TODO impement custom
        if (ppm.name == "CUSTOM")
            btn_mode->setVisible(false);

        group_ppm.addButton(btn_mode, ppm.id);
        ui->pageMode->layout()->addWidget(btn_mode);
    }

    ui->pageMode->layout()->addItem(new QSpacerItem(10, 40));
}

void radeon_profile::createFanProfileGraph()
{
    chartView_fan = new QChartView(this);
    QChart *chart_fan = new QChart();
    chartView_fan->setRenderHint(QPainter::Antialiasing);
    chartView_fan->setChart(chart_fan);

    setupChart(chart_fan, false);

    chart_fan->addSeries(new QLineSeries(chart_fan));

    QValueAxis *axis_temperature = new QValueAxis(chart_fan);
    QValueAxis *axis_speed = new QValueAxis(chart_fan);
    chart_fan->addAxis(axis_temperature,Qt::AlignBottom);
    chart_fan->addAxis(axis_speed, Qt::AlignLeft);
    axis_temperature->setRange(0, 100);
    axis_speed->setRange(0, 100);

    setupAxis(axis_speed, Qt::white, tr("Fan Speed [%]"), 11);
    setupAxis(axis_temperature, Qt::white, tr("Temperature [°C]"), 11);
    setupSeries(static_cast<QLineSeries*>(chart_fan->series()[0]) , Qt::yellow, "", axis_speed, axis_temperature);

    ui->verticalLayout_22->addWidget(chartView_fan);
}

void radeon_profile::createOcProfileGraph() {
    chartView_oc = new QChartView(this);
    QChart *chart_oc = new QChart();
    chartView_oc->setRenderHint(QPainter::Antialiasing);
    chartView_oc->setChart(chart_oc);

    setupChart(chart_oc, true);

    auto axis_state = new QValueAxis(chart_oc);
    auto axis_frequency = new QValueAxis(chart_oc);
    auto axis_volts = new QValueAxis(chart_oc);

    chart_oc->addAxis(axis_state,Qt::AlignBottom);
    chart_oc->addAxis(axis_frequency, Qt::AlignLeft);
    chart_oc->addAxis(axis_volts, Qt::AlignRight);

    setupAxis(axis_state, Qt::white, tr("State"));
    setupAxis(axis_frequency, Qt::white, tr("Frequency [MHz]"));
    setupAxis(axis_volts, Qt::white, tr("Voltage [mV]"));


    // keep the order as in OcSeriesType enum //
    auto series_ocClockFreq = new QLineSeries(chart_oc);
    auto series_ocCoreVolt = new QLineSeries(chart_oc);
    chart_oc->addSeries(series_ocClockFreq);
    chart_oc->addSeries(series_ocCoreVolt);

    setupSeries(series_ocClockFreq, Qt::yellow, tr("Core frequency [MHz]"), axis_state, axis_frequency);
    setupSeries(series_ocCoreVolt, Qt::green, tr("Core voltage [mV]"), axis_state, axis_volts);

    if (!device.getDriverFeatures().isVDDCCurveAvailable) {
        auto series_ocMemFreq = new QLineSeries(chart_oc);
        auto series_ocMemVolt = new QLineSeries(chart_oc);
        chart_oc->addSeries(series_ocMemFreq);
        chart_oc->addSeries(series_ocMemVolt);
        setupSeries(series_ocMemFreq, Qt::blue, tr("Memory frequency [MHz]"), axis_state, axis_frequency);
        setupSeries(series_ocMemVolt, Qt::cyan, tr("Memory voltage [mV]"), axis_state, axis_volts);
    }

    ui->verticalLayout_10->addWidget(chartView_oc);

    ui->list_coreStates->resizeColumnToContents(0);
    ui->list_memStates->resizeColumnToContents(0);
}
