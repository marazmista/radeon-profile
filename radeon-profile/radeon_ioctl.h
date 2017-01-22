#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H


#include <QString>

/**
 * @brief The ioctlHandler class is an interface to the kernel IOCTL system that allows to retrieve informations from the driver.
 */
class ioctlHandler
{
private:
    /**
     * @brief File descriptor for the card device, used by all IOCTLs.
     */
    int fd;

    /**
     * @brief The ioctlCodes struct contains all query codes needed to execute the ioctls.
     */
    struct ioctlCodes{
#define UNAVAILABLE 0
        unsigned request, /**< Identifies the driver to query (radeon/amdgpu). */
            temperature, /**< Temperature ioctl request. */
            coreClock, /**< SCLK ioctl request. */
            maxCoreClock, /**< Max SCLK ioctl request. */
            memoryClock, /**< MCLK ioctl request. */
            vramUsage, /**< VRAM usage ioctl request. */
            vramSize, /**< VRAM total size ioctl request. */
            registry; /**< Registry read ioctl request .*/

        /** @brief Create an empty struct (all codes initialized to UNAVAILABLE). */
        ioctlCodes();
    } codes; /**< Contains available query codes. */


    /**
     * @brief Execute an ioctl call to the driver.
     * @param data Points to the memory area to store input/output data.
     * @param dataSize Size of the memory area.
     * @param command Driver request identifier.
     * @return Success.
     */
    bool getValue(void *data, unsigned dataSize, unsigned command);

    /**
     * @brief Read the content of a GPU register.
     * @param data When called must contain the registry address. On success is filled with the content of the register.
     * @return Success.
     */
    bool readRegistry(unsigned *data);

    /**
     * @brief Open a file descriptor to a file in the path $prefix$index
     * @param prefix Text prefix of the file to open (for example '/dev/dri/card' or '/dev/dri/renderD')
     * @param index Index to append to the prefix
     * @return The file descriptor
     */
    int openPath(const char *prefix, unsigned index);


public:
    /**
     * @brief Open the communication with the device and initialize the ioctl handler.
     * @note You can check if it worked out with isValid().
     * @param card Index of the card to be opened (for example 'card0' --> 0).
     * @note You can find the list of available cards by running 'ls /dev/dri/ | grep card'.
     */
    ioctlHandler(unsigned card);

    /**
     * @brief Close the communication with the device.
     */
    ~ioctlHandler();

    /**
     * @brief Check if the ioctlHandler has been initialized correctly.
     * @return Validity.
     */
    bool isValid();

    /**
     * @brief Get GPU temperature.
     * @param data On success is filled with the value, in °mC (milliCelsius).
     * @return Success.
     */
    bool getTemperature(int *data);

    /**
     * @brief Get GPU current clock (sclk).
     * @param data On success is filled with the value, in MHz.
     * @return Success.
     */
    bool getCoreClock(unsigned *data);

    /**
     * @brief Get GPU maximum clock.
     * @param data On success is filled with the value, in KHz.
     * @return Success.
     */
    bool getMaxCoreClock(unsigned *data);

    /**
     * @brief Get how busy the GPU is.
     * @param data On success is filled with the value, as percentage of time (o%=idle, 100%=full).
     * @param time How much time to consider, in μS (microSeconds).
     * @warning This function blocks the process for the whole time indicated in 'time'.
     * @param frequency How frequently to check if the GPU is busy during the time, in Hz.
     * @note A frequency > 100 Hz is suggested (A low sampling frequency reduces precision).
     * @return Success.
     */
    bool getGpuUsage(float *data, unsigned time, unsigned frequency);

    /**
     * @brief Get VRAM memory current clock (mclk).
     * @param data On success is filled with the value, in MHz.
     * @return Success.
     */
    bool getMemoryClock(unsigned *data);

    /**
     * @brief Get VRAM memory current usage.
     * @param data On success is filled with the value, in bytes.
     * @return Success.
     */
    bool getVramUsage(unsigned long *data);

    /**
     * @brief Get VRAM memory total size.
     * @param data On success is filled with the value, in bytes.
     * @return Success.
     */
    bool getVramSize(unsigned long *data);

    /**
     * @brief Get the name of driver
     * @return Driver name on success, empty on failure
     */
    QString getDriverName();

};



#endif
