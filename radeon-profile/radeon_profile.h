#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include <QMainWindow>
#include <QString>
#include "qcustomplot.h"
#include <QVector>
#include <QSystemTrayIcon>
#include <QEvent>

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

    int i = 0;
    double maxT = 0.0,minT = 0.0,current,tempSum = 0; // for temps
    double rangeX = 180; // for graph scale

    const QString powerMethod = "/sys/class/drm/card0/device/power_method";
    const QString profilePath = "/sys/class/drm/card0/device/power_profile";
    const QString dpmState = "/sys/class/drm/card0/device/power_dpm_state";
    const QString clocksPath = "/sys/kernel/debug/dri/0/radeon_pm_info";
    const QString err = "Err";
    const QString noValues = "no values";

    QSystemTrayIcon *trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance,*changeProfile, *refreshWhenHidden;
    QMenu *dpmMenu, *trayMenu;

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

private:
    Ui::radeon_profile *ui;
    QString getPowerMethod();
    QStringList getClocks(const QString);
    QString getCurrentPowerProfile(const QString);
    void setProfile(const QString, const QStringList);
    void setProfile(const QString, const QString);
    QString getGPUTemp();
    QStringList fillGpuDataTable(const QString);
    QStringList getGLXInfo();
    void setupGraphs();
    void setupTrayIcon();
};

#endif // RADEON_PROFILE_H
