#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include <QMainWindow>
#include <QString>
#include "qcustomplot.h"
#include <QVector>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QTreeWidgetItem>

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit radeon_profile(QWidget *parent = 0);
    ~radeon_profile();
    QString appHomePath;

    QSystemTrayIcon *trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance,*changeProfile, *refreshWhenHidden;
    QMenu *dpmMenu, *trayMenu, *optionsMenu, *forcePowerMenu;
    QTimer *timer;

private slots:
    void on_chProfile_clicked();
    void timerEvent();
    void on_btn_dpmBattery_clicked();
    void on_btn_dpmBalanced_clicked();
    void on_btn_dpmPerformance_clicked();
    void changeTimeRange();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_cb_showFreqGraph_clicked(bool checked);
    void on_cb_showTempsGraph_clicked(bool checked);
    void on_cb_showVoltsGraph_clicked(bool checked);
    void showLegend(bool checked);
    void resetGraphs();
    void forceAuto();
    void forceLow();
    void forceHigh();
    void resetMinMax();
    void on_btn_forceAuto_clicked();
    void on_btn_forceHigh_clicked();
    void on_btn_forceLow_clicked();
    void on_combo_gpus_currentTextChanged(const QString &arg1);
    void closeEvent(QCloseEvent *);
    void closeFromTray();
    void on_spin_lineThick_valueChanged(int arg1);
    void on_spin_timerInterval_valueChanged(double arg1);
    void on_cb_graphs_clicked(bool checked);
    void on_cb_gpuData_clicked(bool checked);
    void refreshBtnClicked();

    void on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::radeon_profile *ui;
    void getPowerMethod();
    QStringList getClocks();
    QString getCurrentPowerProfile();
    void setValueToFile(const QString, const QStringList);
    void setValueToFile(const QString, const QString);
    QString getGPUTemp();
    QStringList fillGpuDataTable();
    void getGLXInfo();
    void setupGraphs();
    void setupTrayIcon();
    void setupOptionsMenu();
    void refreshTooltip();
    void setupForcePowerLevelMenu();
    void testSensor();
    void changeEvent(QEvent *event);
    void getModuleInfo();
    QStringList grabSystemInfo(const QString cmd);
    void getCardConnectors();
    void detectCards();
    void figureOutGPUDataPaths(const QString gpuName);
    void saveConfig();
    void loadConfig();
    void setupGraphsStyle();
};


#endif // RADEON_PROFILE_H
