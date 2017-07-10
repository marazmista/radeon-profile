#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H


#include <QString>


/**
 * @brief The ioctlHandler class is an interface to the kernel IOCTL system that allows to retrieve informations from the driver.
 */
class ioctlHandler
{
protected:
    /**
     * @brief File descriptor for the card device, used by all IOCTLs.
     */
    int fd;

    /**
     * @brief Execute an ioctl call to the driver.
     * @param data Points to the memory area to store input/output data.
     * @param dataSize Size of the memory area.
     * @param command Driver request identifier.
     * @return Success.
     */
    virtual bool getValue(void *data, unsigned dataSize, unsigned command) const = 0;

    /**
     * @brief Read the content of a GPU register.
     * @param data When called must contain the registry address. On success is filled with the content of the register.
     * @return Success.
     */
    virtual bool readRegistry(unsigned *data) const = 0;

    /**
     * @brief Check if the card is currently elaborating.
     * @param data On success is filled with the value (true=active, false=idle).
     * @return Success.
     */
    virtual bool isCardActive(bool *data) const = 0;

    /**
     * @brief Open a file descriptor to a file in the path $prefix$index
     * @param prefix Text prefix of the file to open (for example '/dev/dri/card' or '/dev/dri/renderD')
     * @param index Index to append to the prefix
     * @return The file descriptor
     */
    int openPath(const char *prefix, unsigned index) const;


    /**
     * @brief Open the communication with the device and initialize the ioctl handler.
     * @note You can check if it worked out with isValid().
     * @param card Index of the card to be opened (for example 'card0' --> 0).
     * @note You can find the list of available cards by running 'ls /dev/dri/ | grep card'.
     */
    ioctlHandler(unsigned card);

public:
    /**
     * @brief Close the communication with the device.
     */
    virtual ~ioctlHandler();

    /**
     * @brief Check if the ioctlHandler has been initialized correctly.
     * @return Validity.
     */
    bool isValid() const;

    /**
     * @brief Get GPU temperature.
     * @param data On success is filled with the value, in °mC (milliCelsius).
     * @return Success.
     */
    virtual bool getTemperature(int *data) const = 0;

    /**
     * @brief Get GPU current clock (sclk).
     * @param data On success is filled with the value, in MHz.
     * @return Success.
     */
    virtual bool getCoreClock(int *data) const = 0;

    /**
     * @brief Get GPU maximum clock.
     * @param data On success is filled with the value, in KHz.
     * @return Success.
     */
    virtual bool getMaxCoreClock(int *data) const = 0;

    /**
     * @brief Get memory maximum clock.
     * @param data On success is filled with the value, in KHz.
     * @return Success.
     */
    virtual bool getMaxMemoryClock(int *data) const = 0;

    /**
     * @brief Get both core and memory maximum clocks.
     * @param core On success is filled with the max core clock, in KHz.
     * @param memory On success is filled with the max memory clock, in KHz.
     * @note If one parameter is NULL it will not be filled.
     * @return Success.
     */
    virtual bool getMaxClocks(int *core, int *memory) const = 0;

    /**
     * @brief Get how busy the GPU is.
     * Sample the GPU status register N times and check how many of these samples have the GPU busy
     * @param data On success is filled with the value, as percentage of time (o%=idle, 100%=full).
     * @param time How much time to consider, in μS (microSeconds).
     * @warning This function blocks the process for the whole time indicated in 'time'.
     * @param frequency How frequently to check if the GPU is busy during the time, in Hz.
     * @note A frequency > 100 Hz is suggested (A low sampling frequency reduces precision).
     * @return Success.
     */
    virtual bool getGpuUsage(float *data, int time, int frequency) const;
    virtual bool getGpuUsage(float *data) const = 0;

    /**
     * @brief Get VRAM memory current clock (mclk).
     * @param data On success is filled with the value, in MHz.
     * @return Success.
     */
    virtual bool getMemoryClock(int *data) const = 0;

    /**
     * @brief Get VRAM memory current usage.
     * @param data On success is filled with the value, in bytes.
     * @return Success.
     */
    virtual bool getVramUsage(long *data) const = 0;

    /**
     * @brief Get the percentage of VRAM memory currently used.
     * @param data On success is filled with the value, as percentage.
     * @return Success.
     */
    bool getVramUsagePercentage(long *data) const;

    /**
     * @brief Get VRAM memory total size.
     * @param data On success is filled with the value, in bytes.
     * @return Success.
     */
    virtual bool getVramSize(float *data) const = 0;

    /**
     * @brief Get the name of driver
     * @return Driver name on success, empty on failure
     */
    QString getDriverName() const;

};



/**
 * @brief The radeonIoctlHandler class implements ioctlHandler for the radeon driver.
 * @note Define NO _IOCTL if libdrm headers can't be installed (all ioctl functions will fail).
 */
class radeonIoctlHandler : public ioctlHandler {
protected:
    bool getValue(void *data, unsigned dataSize, unsigned command) const;
    bool readRegistry(unsigned *data) const;
    bool isCardActive(bool *data) const;

public:
    /**
     * @brief Open the communication with the device and initialize the ioctl handler.
     * @note You can check if it worked out with isValid().
     * @param cardIndex Index of the card to be opened (for example 'card0' --> 0).
     * @note You can find the list of available cards by running 'ls /dev/dri/ | grep card'.
     */
    radeonIoctlHandler(unsigned cardIndex);
    bool getCoreClock(int *data) const;
    bool getMaxCoreClock(int *data) const;
    bool getMaxMemoryClock(int *data) const;
    bool getMaxClocks(int *core, int *memory) const;
    bool getMemoryClock(int *data) const;
    bool getTemperature(int *data) const;
    bool getVramSize(float *data) const;
    bool getVramUsage(long *data) const;
    bool getGpuUsage(float *data) const;
};



/**
 * @brief The amdgpuIoctlHandler class implements ioctlHandler for the amdgpu driver.
 * @note Define NO _IOCTL if libdrm headers can't be installed (all ioctl functions will fail).
 * @note Define NO_AMDGPU_IOCTL if only libdrm amdgpu headers are not available (amdgpu ioctl functions will fail).
 */
class amdgpuIoctlHandler : public ioctlHandler {
protected:
    bool getValue(void *data, unsigned dataSize, unsigned command) const;
    bool readRegistry(unsigned *data) const;
    bool isCardActive(bool *data) const;

private:
    bool getSensorValue(void* data, unsigned dataSize, unsigned sensor) const;

public:
    /**
     * @brief Open the communication with the device and initialize the ioctl handler.
     * @note You can check if it worked out with isValid().
     * @param cardIndex Index of the card to be opened (for example 'card0' --> 0).
     * @note You can find the list of available cards by running 'ls /dev/dri/ | grep card'.
     */
    amdgpuIoctlHandler(unsigned cardIndex);
    bool getCoreClock(int *data) const;
    bool getMaxCoreClock(int *data) const;
    bool getMaxMemoryClock(int *data) const;
    bool getMaxClocks(int *core, int *memory) const;
    bool getMemoryClock(int *data) const;
    bool getVceClocks(int *core, int *memory) const;
    bool getTemperature(int *data) const;
    bool getVramSize(float *data) const;
    bool getVramUsage(long *data) const;
    bool getGpuUsage(float *data) const;
};

#endif
