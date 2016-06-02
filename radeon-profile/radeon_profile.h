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
#include <QTimer>
#include <QMenu>
#include <QButtonGroup>

#define startClocksScaleL 50
#define startClocksScaleH 150
#define startVoltsScaleL 500
#define startVoltsScaleH 650

#define minFanStepsTemp 0
#define maxFanStepsTemp 99

#define minFanStepsSpeed 20
#define maxFanStepsSpeed 100

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT

    // names in this enum equals indexes in Qtreewidged in ui for selecting colors
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

    // Elements of any any exec profile
    enum itemValues {
        PROFILE_NAME,
        BINARY,
        BINARY_PARAMS,
        ENV_SETTINGS,
        LOG_FILE,
        LOG_FILE_DATE_APPEND
    };


public:
    explicit radeon_profile(QStringList, QWidget *parent = 0);
    ~radeon_profile();

private slots:
    void timerEvent();
    void on_btn_dpmBattery_clicked();
    void on_btn_dpmBalanced_clicked();
    void on_btn_dpmPerformance_clicked();
    void changeTimeRange();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_cb_showFreqGraph_toggled(const bool &checked);
    void on_cb_showTempsGraph_toggled(const bool &checked);
    void on_cb_showVoltsGraph_toggled(const bool &checked);
    void showLegend(const bool &checked);
    void setGraphOffset(const bool &checked);
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
    void on_cb_graphs_toggled(bool checked);
    void on_cb_gpuData_toggled(bool checked);
    void refreshBtnClicked();
    void on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_cb_stats_toggled(bool checked);
    void copyGlxInfoToClipboard();
    void copyConnectorsToClipboard();
    void resetStats();
    void on_cb_alternateRow_toggled(bool checked);
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
    void on_cb_manualEdit_toggled(bool checked);
    void on_btn_runExecProfile_clicked();
    void on_btn_viewOutput_clicked();
    void btnBackToProfilesClicked();
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
    void on_cb_enableOverclock_toggled(bool enable);
    void on_btn_applyOverclock_clicked();
    void on_slider_GPUoverclock_valueChanged(int value);
    void on_slider_memoryOverclock_valueChanged(int value);
    void on_cb_showAlwaysGpuSelector_toggled(bool checked);
    void on_cb_showCombo_toggled(bool checked);
    void on_mainTabs_currentChanged(int index);

private:
    gpu device;
    static const QString settingsPath;
    QList<execBin*> execsRunning;
    QMap<short, unsigned short> fanSteps;
    QMap<QString, unsigned short> pmStats;
    unsigned short rangeX = 180, ticksCounter = 0, statsTickCounter = 0;
    QSystemTrayIcon trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance, *changeProfile, *refreshWhenHidden;
    QMenu dpmMenu, trayMenu, optionsMenu, forcePowerMenu;
    QTimer timer;
    QButtonGroup pwmGroup;

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
    void replotGraphs();
    void setupUiEnabledFeatures(const driverFeatures &features);
    void loadVariables();
    void updateExecLogs();
    void addRuntimeWidgets();
    void loadFanProfiles();
    void saveFanProfiles();
    int askNumber(const int value, const int min, const int max, const QString label);
    void makeFanProfileGraph();
    void refreshUI();
    void connectSignals();
    void addChild(QTreeWidget * parent, const QString &leftColumn, const QString  &rightColumn);

    /**
     * @brief adjustFanSpeed Sets the PWM fan speed indicated for the actual temperature on the fan profile.
     * @param force Adjust the fan speed even if the temperature has not changed
     */
    void adjustFanSpeed(bool force = false);

    /**
     * @brief configureDaemonAutoRefresh Reconfigures the daemon with indicated auto-refresh settings.
     * @param enabled If true enables auto-refresh, otherwise disables it.
     * @param interval Seconds between each update.
     */
    void configureDaemonAutoRefresh(bool enabled = true, int interval = 1);

    /**
     * @brief showWindow reveals the main window, unless the "Start Minimized" setting is checked
     */
    void showWindow();

    /**
     * @brief fanStepIsValid Checks if the given parameters are a valid fan step.
     * @param temperature
     * @param fanSpeed
     * @return If the step is valid.
     */
    bool fanStepIsValid(int temperature, int fanSpeed);

    /**
     * @brief addFanStep Adds a single fan step to the custom curve steps.
     * If another step with the same temperature exists already it is overwritten.
     * @param temperature
     * @param fanSpeed
     * @param alsoToList Adds the step also to the ui->list_fanSteps list.
     * @param alsoToGraph Adds the step also to the fan steps graph (does NOT replot).
     * @param alsoAdjustSpeed Updates the fan speed after adding the step.
     * @return If the operation was successful.
     */
    bool addFanStep (int temperature, int fanSpeed, bool alsoToList = true, bool alsoToGraph = true, bool alsoAdjustSpeed = true);

    /**
     * @brief askQuestion Creates a dialog to ask a question
     * @param title Title of the dialog
     * @param question Question to ask
     * @return True if the user clicked "Yes", false otherwise
     */
    bool askConfirmation(QString title, QString question);
};

#endif // RADEON_PROFILE_H
