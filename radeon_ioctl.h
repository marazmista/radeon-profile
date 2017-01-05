//#ifdef ENABLE_IOCTL

#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H



int openCardFD(char const * const card);
void closeCardFD(int const fd);

struct buffer {
    int error; // Data is valid only if error == 0
    union {
        unsigned int MegaHertz; // https://lists.freedesktop.org/archives/dri-devel/2014-October/069412.html
        int milliCelsius; // https://lists.freedesktop.org/archives/dri-devel/2013-June/040499.html
        unsigned long byte; // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L529
    } value;
};

struct buffer radeonTemperature(int const fd); // fills milliCelsius
struct buffer radeonCoreClock(int const fd); // fills MegaHertz
struct buffer radeonMemoryClock(int const fd); // fills MegaHertz
struct buffer radeonVramUsage(int const fd); // fills byte
struct buffer radeonGttUsage(int const fd); // fills byte
struct buffer amdgpuVramUsage(int const fd); // fills byte
struct buffer amdgpuGttUsage(int const fd); // fills byte



#endif

//#endif
