#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include "gpu.h"
#include "daemonComm.h"
#include "execbin.h"
#include "rpevent.h"
#include "components/rpplot.h"
#include "components/pieprogressbar.h"
//#include "components/topbarcomponents.h"

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QTreeWidgetItem>
#include <QProcessEnvironment>
#include <QList>
#include <QListWidgetItem>
#include <QButtonGroup>
#include <QXmlStreamWriter>
#include <QtConcurrent/QtConcurrent>

#define minFanStepsTemp 0
#define maxFanStepsTemp 99

#define maxFanStepsSpeed 100

#define appVersion 20170629

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT

    enum itemValues {
        PROFILE_NAME,
        BINARY,
        BINARY_PARAMS,
        ENV_SETTINGS,
        LOG_FILE,
        LOG_FILE_DATE_APPEND
    };

public:
    explicit radeon_profile(QWidget *parent = 0);
    ~radeon_profile();
    QString appHomePath;

    QSystemTrayIcon *trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance,*changeProfile, *refreshWhenHidden;
    QMenu *dpmMenu, *trayMenu, *generalMenu, *forcePowerMenu, *fanProfilesMenu;
    QTimer *timer = nullptr;
    static unsigned int minFanStepsSpeed;

    typedef QMap<int, unsigned int> fanProfileSteps;
    
    
private slots:
    void timerEvent();
    void initFutureHandler();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void forceAuto();
    void forceLow();
    void forceHigh();
    void setBattery();
    void setBalanced();
    void setPerformance();
    void resetMinMax();
    void gpuChanged();
    void closeEvent(QCloseEvent *e);
    void closeFromTray();
    void on_spin_timerInterval_valueChanged(double arg1);
    void on_cb_gpuData_clicked(bool checked);
    void refreshBtnClicked();
    void on_cb_stats_clicked(bool checked);
    void copyGlxInfoToClipboard();
    void copyConnectorsToClipboard();
    void resetStats();
    void on_cb_alternateRow_clicked(bool checked);
    void on_chProfile_clicked();
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
    void btnBackToProfilesClicked();
    void on_list_execProfiles_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tabs_execOutputs_tabCloseRequested(int index);
    void on_btn_pwmFixedApply_clicked();
    void on_btn_pwmFixed_clicked();
    void on_btn_pwmAuto_clicked();
    void on_btn_pwmProfile_clicked();
    void setPowerLevelFromCombo();
    void on_btn_fanInfo_clicked();
    void on_btn_addFanStep_clicked();
    void on_btn_removeFanStep_clicked();
    void on_list_fanSteps_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_fanSpeedSlider_valueChanged(int value);
    void on_btn_applyOverclock_clicked();
    void on_slider_ocSclk_valueChanged(int value);
    void on_btn_activateFanProfile_clicked();
    void on_btn_removeFanProfile_clicked();
    void on_btn_saveFanProfile_clicked();
    void on_btn_saveAsFanProfile_clicked();
    void fanProfileMenuActionClicked(QAction *a);
    void on_cb_zeroPercentFanSpeed_clicked(bool checked);
    void on_combo_fanProfiles_currentIndexChanged(const QString &arg1);
    void on_btn_addEvent_clicked();
    void on_list_events_itemChanged(QTreeWidgetItem *item, int column);
    void on_btn_eventsInfo_clicked();
    void on_btn_modifyEvent_clicked();
    void on_btn_removeEvent_clicked();
    void on_btn_revokeEvent_clicked();
    void on_list_events_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_saveAll_clicked();
    void setPowerLevel(int level);
    void on_btn_configurePlots_clicked();
    void on_btn_applySavePlotsDefinitons_clicked();
    void on_btn_addPlotDefinition_clicked();
    void on_btn_removePlotDefinition_clicked();
    void on_btn_modifyPlotDefinition_clicked();
    void on_list_plotDefinitions_itemChanged(QTreeWidgetItem *item, int column);
    void on_list_plotDefinitions_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_slider_timeRange_valueChanged(int value);
    void on_cb_daemonData_clicked(bool checked);
    void pauseRefresh(bool checked);
    void on_btn_general_clicked();
    void on_slider_ocMclk_valueChanged(const int value);
    void on_group_oc_toggled(bool arg1);
    void on_slider_freqSclk_valueChanged(int value);
    void on_slider_freqMclk_valueChanged(int value);
    void on_group_freq_toggled(bool arg1);

private:
    struct currentStateInfo {
        PowerProfiles profile;
        ForcePowerLevels powerLevel;
        short fanIndex;
        QString fanProfileName;
    };

    gpu device;
    static const QString settingsPath;
    QList<execBin*> execsRunning;
    fanProfileSteps currentFanProfile;
    QMap<QString, fanProfileSteps> fanProfiles;
    QMap<QString, RPEvent> events;
    QMap<QString, unsigned int> pmStats;
    unsigned int ticksCounter, statsTickCounter;
    QButtonGroup pwmGroup, dpmGroup;
	currentStateInfo *savedState;
    QFutureWatcher<void> initFuture;
    PlotManager plotManager;
    QChartView *fanProfileChart;
    QLineSeries *fanSeries;
    QMap<int, PieProgressBar*> topBarPies;
    QMap<int, ValueID> keysInCurrentGpuList;

    Ui::radeon_profile *ui;
    void setupTrayIcon();
    void refreshTooltip();
    void setupForcePowerLevelMenu();
    void changeEvent(QEvent *event);
    void saveConfig();
    void loadConfig();
    void doTheStats();
    void updateStatsTable();
    void setupContextMenus();
    void refreshGpuData();
    void refreshGraphs();
    void setupUiEnabledFeatures(const DriverFeatures &features, const GPUDataContainer &data);
    void loadVariables();
    void updateExecLogs();
    void addRuntimeWidgets();
    void loadFanProfiles();
    int askNumber(const int value, const int min, const int max, const QString label);
    void makeFanProfileListaAndGraph(const fanProfileSteps &profile);
    void makeFanProfilePlot();
    void refreshUI();
    void connectSignals();
    void setCurrentFanProfile(const QString &profileName, const fanProfileSteps &profile);
    void adjustFanSpeed();
    fanProfileSteps stepsListToMap();
    void addChild(QTreeWidget * parent, const QString &leftColumn, const QString  &rightColumn);
    void setupFanProfilesMenu(const bool rebuildMode = false);
    int findCurrentFanProfileMenuIndex();
    void setupMinFanSpeedSetting(unsigned int speed);
    void markFanProfileUnsaved(bool unsaved);
    void checkEvents();
    void activateEvent(const RPEvent &rpe);
    void saveRpevents(QXmlStreamWriter &xml);
    void loadRpevent(const QXmlStreamReader &xml);
    void revokeEvent();
    void hideEventControls(bool hide);
    void saveExecProfiles(QXmlStreamWriter &xml);
    void loadExecProfile(const QXmlStreamReader &xml);
    void saveFanProfiles(QXmlStreamWriter &xml);
    void loadFanProfile(QXmlStreamReader &xml);
    void savePlotSchemas(QXmlStreamWriter &xml);
    void loadPlotSchemas(QXmlStreamReader &xml);
    void writePlotAxisSchemaToXml(QXmlStreamWriter &xml, const QString side, const PlotAxisSchema &pas);
    void loadPlotAxisSchema(const QXmlStreamReader &xml, PlotAxisSchema &pas);
    void createDefaultFanProfile();
    void loadExecProfiles();
    void setupUiElements();
    void addPlotsToLayout();
    void modifyPlotSchema(const QString &name);
    void createCurrentGpuDataListItems();
    void fillConnectors();
    void fillModInfo();
    bool askConfirmation(const QString title, const QString question);
    void createTopBar();
    void showWindow();
    bool isFanStepValid(unsigned int temperature, unsigned int fanSpeed);
    void addFanStep (int temperature, int fanSpeed);
    void createGeneralMenu();
    PlotInitialValues figureOutInitialScale(const PlotDefinitionSchema &pds);
    void applyOc();
};

#endif // RADEON_PROFILE_H
