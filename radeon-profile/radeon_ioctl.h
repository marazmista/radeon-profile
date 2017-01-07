#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H



/**
 * @brief openCardFD Open the file descriptor needed for ioctls
 * @param card Name of the card to be opened (e.g. card0).
 * @warning You can find the list of available cards by running 'ls /dev/dri/'
 * @return File descriptor >=0 on success, error value <0 on failure
 */
int openCardFD(const char * card);


/**
 * @brief closeCardFD Close a file descriptor
 * @param fd The file descriptor to close
 */
void closeCardFD(int fd);


/**
 * @brief radeonTemperature Get GPU temperature [RADEON driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in °mC (milliCelsius)
 * @return 0 on success, error value !=0 on failure
 */
int radeonTemperature(int fd, int *data);


/**
 * @brief radeonCoreClock Get GPU current clock (sclk) [RADEON driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in MHz
 * @return 0 on success, error value !=0 on failure
 */
int radeonCoreClock(int fd, unsigned *data);


/**
 * @brief radeonMaxCoreClock Get GPU maximum clock [RADEON driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in KHz
 * @return 0 on success, error value !=0 on failure
 */
int radeonMaxCoreClock(int fd, unsigned *data);


/**
 * @brief radeonGpuUsage Get how busy the GPU is [RADEON driver]
 * @brief amdgpuGpuUsage Get how busy the GPU is [AMDGPU driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, as percentage of time (o%=idle, 100%=full)
 * @param time How much time to consider, in μS (microSeconds)
 * @warning This function blocks the application for the whole time indicated in 'time'
 * @param frequency How frequently to check if the GPU is busy during the time, in Hz
 * @warning A frequency > 100 Hz is suggested (A low sampling frequency reduces precision)
 * @return 0 on success, error value !=0 on failure
 */
int radeonGpuUsage(int fd, float *data, unsigned time, unsigned frequency);
int amdgpuGpuUsage(int fd, float *data, unsigned time, unsigned frequency);


/**
 * @brief radeonMemoryClock Get VRAM memory current clock (mclk) [RADEON driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in MHz
 * @return 0 on success, error value !=0 on failure
 */
int radeonMemoryClock(int fd, unsigned *data);


/**
 * @brief radeonVramUsage Get VRAM memory current usage [RADEON driver]
 * @brief amdgpuVramUsage Get VRAM memory current usage [AMDGPU driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in bytes
 * @return 0 on success, error value !=0 on failure
 */
int radeonVramUsage(int fd, unsigned long *data);
int amdgpuVramUsage(int fd, unsigned long *data);


/**
 * @brief amdgpuVramSize Get VRAM memory size [AMDGPU driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in bytes
 * @return 0 on success, error value !=0 on failure
 */
int amdgpuVramSize(int fd, unsigned long *data);


/**
 * @brief radeonGttUsage Get GTT memory current usage [RADEON driver]
 * @brief amdgpuGttUsage Get GTT memory current usage [AMDGPU driver]
 * @param fd File descriptor to use
 * @param data On success is filled with the value, in bytes
 * @return 0 on success, error value !=0 on failure
 */
int radeonGttUsage(int fd, unsigned long *data);
int amdgpuGttUsage(int fd, unsigned long *data);


/**
 * @brief radeonReadRegistry Read the content of a GPU register [RADEON driver]
 * @brief amdgpuReadRegistry Read the content of a GPU register [AMDGPU driver]
 * @param fd File descriptor to use
 * @param data When called must contain the registry address. On success is filled with the content of the register.
 * @return 0 on success, error value !=0 on failure
 */
int radeonReadRegistry(int fd, unsigned *data);
int amdgpuReadRegistry(int fd, unsigned *data);



#endif
