// copyright marazmista @ 29.03.2014

// in here, we have class which represent graphic adapter in program //
// It chooses driver and communicate with proper class for selected driver //
// and retuns result to gui //

#ifndef GPU_H
#define GPU_H

#include "globalStuff.h"
#include "daemonComm.h"
#include <QTreeWidgetItem>
#include <QStringList>

/**
 * @brief The gpu class represents the wrapper which radeon_profile uses to interface with any driver.
 * It must be inherited by the driver classes
 * Also manges the eventual daemon communication through daemonComm
 * @see daemonComm
 */
class gpu : public QObject
{
    Q_OBJECT
public:


    gpuClocksStruct gpuClocksData;
    gpuClocksStructString gpuClocksDataString;
    gpuTemperatureStruct gpuTemeperatureData;
    gpuTemperatureStructString gpuTemeperatureDataString;
    QStringList gpuList;
    driverFeatures features;
    QString currentPowerProfile, currentPowerLevel;
    short maxCoreFreq, minCoreFreq; // MHz
    QString maxCoreFreqString, minCoreFreqString;

    /*  Common functions, already implemented in gpu.cpp (can be re-implemented/extended by inheriting classes)  */
    gpu();

    /**
     * @brief initialize Initialize the driver (detect available cards, select the current card, figure out available features)
     * @see detectCard()
     * @see changeGpu()
     * @see figureOutDriverFeatures()
     */
    virtual void initialize();

    /**
     * @brief getCardConnectors
     * @return List of video connectors and connected screens with various details
     */
    virtual QList<QTreeWidgetItem *> getCardConnectors() const;

    /**
     * @brief getGLXInfo
     * @param gpuName Name of the gpu to analyze
     * @return List of details related to OpenGL
     */
    virtual QStringList getGLXInfo(const QString & gpuName) const;

    /**
     * @brief getModuleInfo
     * @return List of parameters of the driver module
     */
    virtual QList<QTreeWidgetItem *> getModuleInfo() const;

    /**
     * @brief convertClocks translates data from gpuClocksData and fills gpuClocksDataString
     */
    virtual void convertClocks();

    /**
     * @brief convertTemperature translates data from gpuTemperatureData and fills gpuTemperatureDataString
     */
    virtual void convertTemperature();

    /**
     * @brief detectCards finds all available cards
     * The default implementation searchs for cards in "/sys/class/drm/" created by the current module
     */
    virtual QStringList detectCards() const;

    /*  Core functions, MUST be re-implemented by inheriting classes  */

    /**
     * @brief changeGPU switches the current gpu
     * @param gpuIndex position of the card to select in gpuList
     */
    virtual void changeGPU(ushort gpuIndex);

    /**
     * @brief figureOutDriverFeatures checks which features are available and fills the features struct
     */
    virtual driverFeatures figureOutDriverFeatures();

    /*  Optional functions, depending on driverFeatures  */
    // Already implemented functions
    /**
     * @brief updateClocksData fills gpuClocksData and gpuClocksDataString
     * Reimplement only if more data than gpuClocksData is available
     * @see getClocks()
     * @see convertClocks()
     */
    virtual void updateClocksData();

    /**
     * @brief updateTemperatureData fills gpuTemperatureData and gpuTemperatureDataString
     * Reimplement only if more data than gpuTemperatureData is available
     * @see getTemperature()
     * @see getPwmSpeed()
     * @see convertTemperature()
     */
    virtual void updateTemperatureData();

    /**
     * @brief refreshPowerLevel updates the current power profile and the current power level
     * Reimplement only if more data than power prifle and power level are available
     * @see getCurrentPowerProfile()
     * @see getCurrentPowerLevel()
     */
    virtual void refreshPowerLevel();

    virtual bool daemonConnected() const;

    // Unimplemented functions
    // If a feature is available (see the features struct), the class must reimplement the corrispondent function
    /**
     * @brief getClocks
     * @param forFeatures Wait one update cycle for the daemon to read the data.
     * @see features.coreClockAvailable
     * @see features.memClockAvailable
     */
    virtual gpuClocksStruct getClocks(bool forFeatures = false);

    /**
     * @brief getTemperature
     * @return current GPU temperature
     * @see features.temperatureAvailable
     */
    virtual float getTemperature() const;

    /**
     * @brief getPwmSpeed
     * @return Fan pwm speed (percentage over max pwm speed)
     * @see features.pwmAvailable
     */
    virtual ushort getPwmSpeed() const;

    virtual QString getCurrentPowerProfile() const;
    virtual QString getCurrentPowerLevel() const;
    virtual void setPowerProfile(powerProfiles newPowerProfile);
    virtual void setForcePowerLevel(forcePowerLevels newForcePowerLevel);

    virtual void setPwmManualControl(bool manual);

    /**
     * @brief setPwmValue
     * @param value Fan pwm speed (percentage of maxSpeed)
     */
    virtual void setPwmValue(ushort value);

    virtual void reconfigureDaemon();

    /**
     * @brief overclockGPU Overclock the GPU core clock by a percentage
     * @param value Percentage to overclock
     * @return True if the overclock is successful, false otherwise
     * @see features.GPUoverClockAvailable
     */
    virtual bool overclockGPU(int value);

    /**
     * @brief overclockMemory Overclock the GPU memory clock by a percentage
     * @param value Percentage to overclock
     * @return True if the overclock is successful, false otherwise
     * @see driverFeatures.memoryOverclockAvailable
     */
    virtual bool overclockMemory(int value);

protected:
    ushort currentGpuIndex;
    QString driverModule;

    // Already implemented utility functions
    /**
     * @brief setNewValue Writes newValue into filePath
     * @param filePath Destination path
     * @param newValue Value to write
     * @return success or failure
     */
    bool setNewValue(const QString & filePath, const QString & newValue) const;

    /**
     * @brief readFile read the content of a file
     * @param filePath path of the file to read
     * @return Content of the file
     */
    QString readFile(const QString & filePath) const;
};
#endif // GPU_H
