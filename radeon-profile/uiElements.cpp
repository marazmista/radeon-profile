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
