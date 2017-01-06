#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H



/***** All functions return 0 on success, !=0 on failure *****/
int openCardFD(const char * card);
void closeCardFD(int fd);
int radeonTemperature(int fd, int *data); // milliCelsius
int radeonCoreClock(int fd, unsigned *data); // MegaHertz
int radeonMemoryClock(int fd, unsigned *data); // MegaHertz
int radeonMaxCoreClock(int fd, unsigned *data); // KiloHertz
int radeonReadRegistry(int fd, unsigned *data); // raw; when called, *data must contain the registry address
int radeonGpuUsage(int fd, float *data, unsigned time, unsigned frequency); // percentage; blocks for $time microseconds
int radeonVramUsage(int fd, unsigned long *data); // byte
int radeonGttUsage(int fd, unsigned long *data); // byte
int amdgpuVramSize(int fd, unsigned long *data); // byte
int amdgpuVramUsage(int fd, unsigned long *data); // byte
int amdgpuGttUsage(int fd, unsigned long *data); // byte
int amdgpuReadRegistry(int fd, unsigned *data); // raw; when called, *data must contain the registry address
int amdgpuGpuUsage(int fd, float *data, unsigned time, unsigned frequency); // percentage; blocks for $time microseconds



#endif
