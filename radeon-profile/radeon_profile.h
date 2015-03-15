#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include "gpu.h"
#include "daemonComm.h"
#include "execbin.h"

#include <QMainWindow>
#include <QString>
#include "qcustomplot.h"
#include <QVector>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QTreeWidgetItem>
#include <QProcessEnvironment>
#include <QList>
#include <QListWidgetItem>

#define startClocksScaleL 50
#define startClocksScaleH 150
#define startVoltsScaleL 500
#define startVoltsScaleH 650

extern int ticksCounter, statsTickCounter;
extern QString powerMethodFilePath, profilePath, dpmStateFilePath, clocksPath, forcePowerLevelFilePath, sysfsHwmonPath, moduleParamsPath;
extern char selectedPowerMethod, selectedTempSensor, sensorsGPUtempIndex;
extern double maxT, minT, rangeX, current, tempSum;
extern QTimer timer;

struct pmLevel {
    QString name;
    float time;
};
extern QList<pmLevel> pmStats;

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT

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

    enum itemValues {
        PROFILE_NAME,
        BINARY,
        BINARY_PARAMS,
        ENV_SETTINGS,
        LOG_FILE,
        LOG_FILE_DATE_APPEND
    };

    struct fanStepPair {
        int temperature, speed;

        fanStepPair(int _temperature, int _speed) {
            temperature = _temperature;
            speed = _speed;
        }
    };

public:
    explicit radeon_profile(QStringList, QWidget *parent = 0);
    ~radeon_profile();
    QString appHomePath;

    QSystemTrayIcon *trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance,*changeProfile, *refreshWhenHidden;
    QMenu *dpmMenu, *trayMenu, *optionsMenu, *forcePowerMenu;
    QTimer *timer;

private slots:
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
    void on_cb_stats_clicked(bool checked);
    void copyGlxInfoToClipboard();
    void resetStats();
    void on_cb_alternateRow_clicked(bool checked);
    void on_chProfile_clicked();
    void on_btn_reconfigureDaemon_clicked();
    void on_btn_cancel_clicked();
    void on_btn_addExecProfile_clicked();
    void on_list_vaules_itemClicked(QListWidgetItem *item);
    void on_list_variables_itemClicked(QListWidgetItem *item);
    void on_btn_modifyExecProfile_clicked();
    void on_btn_ok_clicked();
    void on_btn_removeExecProfile_clicked();
    void on_btn_selectBinary_clicked();
    void on_btn_selectLog_clicked();
    void on_cb_manualEdit_clicked(bool checked);
    void on_btn_runExecProfile_clicked();
    void on_btn_viewOutput_clicked();
    void on_btn_backToProfiles_clicked();
    void on_list_execProfiles_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tabs_execOutputs_tabCloseRequested(int index);
    void on_btn_pwmFixedApply_clicked();
    void on_btn_pwmFixed_clicked();
    void on_btn_pwmAuto_clicked();
    void on_btn_pwmProfile_clicked();
    void changeProfileFromCombo();
    void changePowerLevelFromCombo();
    void on_btn_fanInfo_clicked();
    void on_btn_addFanStep_clicked();
    void on_btn_removeFanStep_clicked();
    void on_list_fanSteps_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_fanSpeedSlider_valueChanged(int value);

private:
    gpu device;
    static const QString settingsPath;
    QList<execBin*> *execsRunning;
    QList<fanStepPair> fanSteps;

    Ui::radeon_profile *ui;
    void setupGraphs();
    void setupTrayIcon();
    void setupOptionsMenu();
    void refreshTooltip();
    void setupForcePowerLevelMenu();
    void changeEvent(QEvent *event);
    void saveConfig();
    void loadConfig();
    void setupGraphsStyle();
    void doTheStats();
    void updateStatsTable();
    void setupContextMenus();
    void refreshGpuData();
    void refreshGraphs();
    void setupUiEnabledFeatures(const globalStuff::driverFeatures &features);
    void loadVariables();
    void updateExecLogs();
    void addRuntimeWidgets();
    void loadFanProfiles();
    void saveFanProfiles();
    int askNumber(const int value, const int min, const int max, const QString label);
    void makeFanProfileGraph(const QList<fanStepPair> &steps);
    void refreshUI();
    void connectSignals();
    void adjustFanSpeed();

};

#endif // RADEON_PROFILE_H
