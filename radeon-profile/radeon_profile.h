#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include <QMainWindow>
#include <QString>
#include "qcustomplot.h"
#include <QVector>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QTreeWidgetItem>
#include <QProcessEnvironment>

#define startClocksScaleL 100
#define startClocksScaleH 400
#define startVoltsScaleL 500
#define startVoltsScaleH 650

extern int ticksCounter;
extern QString powerMethodFilePath, profilePath, dpmStateFilePath, clocksPath, forcePowerLevelFilePath, sysfsHwmonPath, moduleParamsPath;
extern char selectedPowerMethod, selectedTempSensor, sensorsGPUtempIndex;
extern double maxT, minT, rangeX, current, tempSum;
extern QTimer timer;

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT

    enum powerMethod {
        DPM = 0,  // kernel >= 3.11
        PROFILE = 1,  // kernel <3.11 or dpm disabled
        PM_UNKNOWN = 2
    };

    enum tempSensor {
        SYSFS_HWMON = 0, // try to read temp from /sys/class/hwmonX/device/tempX_input
        CARD_HWMON, // try to read temp from /sys/class/drm/cardX/device/hwmon/hwmonX/temp1_input
        PCI_SENSOR,  // PCI Card, 'radeon-pci' label on sensors output
        MB_SENSOR,  // Card in motherboard, 'VGA' label on sensors output
        TS_UNKNOWN
    };

    // names in this enum equals indexes in Qtreewidged in ui for selecting clors
    enum graphColors {
        TEMP_BG = 0,
        CLOCKS_BG,
        VOLTS_BG,
        TEMP_LINE,
        GPU_CLOCK_LINE,
        MEM_CLOCK_LINE,
        UVD_VIDEO_LINE,
        UVD_DECODER_LINE,
        CORE_VOLTS_LINE,
        MEM_VOLTS_LINE
    };

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
    void gpuChanged();
    void closeEvent(QCloseEvent *);
    void closeFromTray();
    void on_spin_lineThick_valueChanged(int arg1);
    void on_spin_timerInterval_valueChanged(double arg1);
    void on_cb_graphs_clicked(bool checked);
    void on_cb_gpuData_clicked(bool checked);
    void refreshBtnClicked();
    void on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tabs_systemInfo_currentChanged(int index);

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
    QStringList grabSystemInfo(const QString cmd, const QProcessEnvironment env);
    void getCardConnectors();
    void detectCards();
    void figureOutGPUDataPaths(const QString gpuName);
    void saveConfig();
    void loadConfig();
    void setupGraphsStyle();
    QString findSysfsHwmonForGPU();
    void doTheStats(const double &coreClock,const double &memClock,const double &voltsGPU, const double &voltsMem);
    void updateStatsTable();
};


#endif // RADEON_PROFILE_H
