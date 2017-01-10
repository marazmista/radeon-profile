#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H


#include <QString>

/**
 * @brief The ioctlHandler class
 */
class ioctlHandler
{
private:
    int fd;
    struct ioctlCodes {
        uint32_t request,
            temperature,
            coreClock,
            maxCoreClock,
            memoryClock,
            vramUsage,
            vramSize,
            registry;
    } codes;

    /**
     * @brief getValue Execute an ioctl call to the driver
     * @param data Buffer memory area to store input/output data
     * @param dataSize Size of the memory area
     * @param command Driver request identifier
     * @return Success
     */
    bool getValue(void *data, unsigned dataSize, unsigned command);

    /**
     * @brief readRegistry Read the content of a GPU register
     * @param data When called must contain the registry address. On success is filled with the content of the register.
     * @return Success
     */
    bool readRegistry(unsigned *data);


public:
    /**
     * @brief ioctlHandler initializes the ioctl handler
     * @warning the class MUST be initializedor it won't work; you can check if it worked out with isValid()
     * @param card Name of the card to be opened (e.g. card0).
     * @warning You can find the list of available cards by running 'ls /dev/dri/'
     */
    ioctlHandler(QString card, QString driver);

    ~ioctlHandler();

    /**
     * @brief isValid Check if the ioctlHandler has been initialized correctly
     * @return Validity
     */
    bool isValid();


    /**
     * @brief getTemperature Get GPU temperature
     * @param data On success is filled with the value, in °mC (milliCelsius)
     * @return Success
     */
    bool getTemperature(int *data);


    /**
     * @brief getCoreClock Get GPU current clock (sclk)
     * @param fd File descriptor to use
     * @param data On success is filled with the value, in MHz
     * @return Success
     */
    bool getCoreClock(unsigned *data);


    /**
     * @brief getMaxCoreClock Get GPU maximum clock
     * @param fd File descriptor to use
     * @param data On success is filled with the value, in KHz
     * @return Success
     */
    bool getMaxCoreClock(unsigned *data);


    /**
     * @brief getGpuUsage Get how busy the GPU is
     * @param data On success is filled with the value, as percentage of time (o%=idle, 100%=full)
     * @param time How much time to consider, in μS (microSeconds)
     * @warning This function blocks the application for the whole time indicated in 'time'
     * @param frequency How frequently to check if the GPU is busy during the time, in Hz
     * @warning A frequency > 100 Hz is suggested (A low sampling frequency reduces precision)
     * @return Success
     */
    bool getGpuUsage(float *data, unsigned time, unsigned frequency);


    /**
     * @brief getMemoryClock Get VRAM memory current clock (mclk)
     * @param fd File descriptor to use
     * @param data On success is filled with the value, in MHz
     * @return Success
     */
    bool getMemoryClock(unsigned *data);


    /**
     * @brief getVramUsage Get VRAM memory current usage
     * @param fd File descriptor to use
     * @param data On success is filled with the value, in bytes
     * @return Success
     */
    bool getVramUsage(unsigned long *data);


    /**
     * @brief getVramSize Get VRAM memory size
     * @param fd File descriptor to use
     * @param data On success is filled with the value, in bytes
     * @return Success
     */
    bool getVramSize(unsigned long *data);

};



#endif
