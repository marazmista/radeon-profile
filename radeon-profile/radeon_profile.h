#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include "gpu.h"
#include "daemonComm.h"
#include "execbin.h"
#include "rpevent.h"
#include "components/rpplot.h"
#include "components/pieprogressbar.h"
#include "components/topbarcomponents.h"

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

#define appVersion 20181028

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT

    enum ItemValues {
        PROFILE_NAME,
        BINARY,
        BINARY_PARAMS,
        ENV_SETTINGS,
        LOG_FILE,
        LOG_FILE_DATE_APPEND
    };

public:
    static unsigned int minFanStepsSpeed;

    explicit radeon_profile(QWidget *parent = 0);
    ~radeon_profile();
    
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
    void on_btn_configureTopbar_clicked();
    void on_btn_setPlotsBackground_clicked();

private:
    struct CurrentStateInfo {
        PowerProfiles profile;
        ForcePowerLevels powerLevel;
        short fanIndex;
        QString fanProfileName;
    };

    QSystemTrayIcon *trayIcon;
    QAction *closeApp, *dpmSetBattery, *dpmSetBalanced, *dpmSetPerformance,*changeProfile, *refreshWhenHidden;
    QMenu *dpmMenu, *trayMenu, *generalMenu, *forcePowerMenu, *fanProfilesMenu;
    QTimer *timer = nullptr;

    gpu device;
    QList<ExecBin*> execsRunning;
    FanProfileSteps currentFanProfile;
    QMap<QString, FanProfileSteps> fanProfiles;
    QMap<QString, RPEvent> events;
    QMap<QString, unsigned int> pmStats;
    unsigned int ticksCounter, statsTickCounter;
    short hysteresisRelativeTepmerature = 0;
    QButtonGroup pwmGroup, dpmGroup;
    CurrentStateInfo *savedState;
    QFutureWatcher<void> *initFuture;
    PlotManager plotManager;
    TopbarManager topbarManager;
    QChartView *fanProfileChart;
    QLineSeries *fanSeries;
    QList<TopbarItem*> topBarItems;
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
    void makeFanProfileListaAndGraph(const FanProfileSteps &profile);
    void makeFanProfilePlot();
    void refreshUI();
    void connectSignals();
    void setCurrentFanProfile(const QString &profileName, const FanProfileSteps &profile);
    void adjustFanSpeed();
    FanProfileSteps stepsListToMap();
    void addTreeWidgetItem(QTreeWidget * parent, const QString &leftColumn, const QString  &rightColumn);
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
    void saveTopbarItemsSchemas(QXmlStreamWriter &xml);
    void loadTopbarItemsSchemas(const QXmlStreamReader &xml);
    void writePlotAxisSchemaToXml(QXmlStreamWriter &xml, const QString side, const PlotAxisSchema &pas);
    void loadPlotAxisSchema(const QXmlStreamReader &xml, PlotAxisSchema &pas);
    void createDefaultFanProfile();
    void loadExecProfiles();
    void setupUiElements();
    void createPlots();
    void modifyPlotSchema(const QString &name);
    void createCurrentGpuDataListItems();
    void fillConnectors();
    void fillModInfo();
    bool askConfirmation(const QString title, const QString question);
    void createDefaultTopbar();
    void showWindow();
    bool isFanStepValid(unsigned int temperature, unsigned int fanSpeed);
    void addFanStep (int temperature, int fanSpeed);
    void createGeneralMenu();
    PlotInitialValues figureOutInitialScale(const PlotDefinitionSchema &pds);
    void applyOc();
    void setupPlot(const PlotDefinitionSchema &pds);
};

#endif // RADEON_PROFILE_H
