// Define NO _IOCTL if libdrm headers can't be installed (all ioctl functions will return failure)
// Define NO_AMDGPU_IOCTL if libdrm headers  are installed but amdgpu is not available (Linux < 4.2)

#ifndef NO_IOCTL // Check if libdrm headers are available only if NO_IOCTL is not defined ...
#  ifdef __has_include // ... and the compiler supports the __has_include macro (clang or gcc>=5.0)
#    if !__has_include(<libdrm/radeon_drm.h>)
#      error radeon_drm.h is not available! Install libdrm headers or compile with the flag -DNO_IOCTL
#    endif // radeon_drm.h
#    ifndef NO_AMDGPU_IOCTL // Search the amdgpu drm header only if NO_AMDGPU_IOCTL is not defined
#      if !__has_include(<libdrm/amdgpu_drm.h>)
#        error amdgpu_drm.h is not available! Install libdrm headers or compile with the flag -DNO_AMDGPU_IOCTL
#      endif // amdgpu_drm.h
#    endif // NO_AMDGPU_IOCTL
#  endif // __has_include
#endif // NO_IOCTL


#include "radeon_ioctl.h"
#include <QString>
#include <QDebug>
#include <cstring> // memset(), strerror()
#include <cstdio> // snprintf()
#include <cerrno> // errno
#include <unistd.h> // close(), usleep()
#include <fcntl.h> // open()
#include <sys/ioctl.h> // ioctl()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  include <libdrm/radeon_drm.h> // radeon ioctl codes and structs
#  ifndef NO_AMDGPU_IOCTL
#    include <libdrm/amdgpu_drm.h> // amdgpu ioctl codes and structs
#  endif
#endif


#define DESCRIBE_ERROR(title) qWarning() << (title) << ':' << strerror(errno)
#define CLEAN(object) memset(&(object), 0, sizeof(object))


ioctlHandler::ioctlCodes::ioctlCodes(){
    request = UNAVAILABLE;
    temperature = UNAVAILABLE;
    coreClock = UNAVAILABLE;
    maxCoreClock = UNAVAILABLE;
    memoryClock = UNAVAILABLE;
    vramUsage = UNAVAILABLE;
    vramSize = UNAVAILABLE;
    registry = UNAVAILABLE;
}

int ioctlHandler::openPath(const char *prefix, unsigned index){
#define PATH_SIZE 20
    char path[PATH_SIZE];
    snprintf(path, PATH_SIZE, "%s%u", prefix, index);
    qDebug() << "Opening" << path << "for ioctls";

    int res = open(path, O_RDONLY);
    if(res < 0) // Open failed
        DESCRIBE_ERROR(path);

    return res;
}

ioctlHandler::ioctlHandler(unsigned card){
    codes = ioctlCodes();

#ifdef NO_IOCTL
    qWarning() << "radeon profile was compiled with NO_IOCTL, ioctls won't work";
    fd = -1;
    return;
#endif // NO_IOCTL


    /* Open file descriptor to card device
     * Information ioctls can be accessed in two ways:
     * The kernel generates for the card with index N the files /dev/dri/card<N> and /dev/dri/renderD<128+N>
     * Both can be opened without root access
     * Executing ioctls on /dev/dri/card<N> requires either root access or being DRM Master
     * /dev/dri/renderD<128+N> does not require these permissions, but legacy kernels (Linux < 3.15) do not support it
     * For more details:
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#Render_nodes
     * https://www.x.org/wiki/Events/XDC2013/XDC2013DavidHerrmannDRMSecurity/slides.pdf#page=15
     */
    fd = openPath("/dev/dri/renderD", 128+card); // Try /dev/dri/renderD<128+N>
    if(fd < 0){ // /dev/dri/renderD<128+N> not available
        fd = openPath("/dev/dri/card", card); // Try /dev/dri/card<N>
        if(fd < 0) // /dev/dri/card<N> not available
            return; // Initialization failed
    }

    // Initialize ioctl codes
    QString driver = getDriverName();
    qDebug() << "Initializing ioctl codes for driver " << driver;

#ifdef __RADEON_DRM_H__
    if(driver=="radeon"){
        // https://cgit.freedesktop.org/mesa/drm/tree/include/drm/radeon_drm.h#n993
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/radeon_drm.h#n993
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/radeon_kms.c#n203
        codes.request = DRM_IOCTL_RADEON_INFO;
#ifdef RADEON_INFO_CURRENT_GPU_TEMP // Available only in Linux 4.1 and above
        codes.temperature = RADEON_INFO_CURRENT_GPU_TEMP;
        codes.coreClock = RADEON_INFO_CURRENT_GPU_SCLK;
        codes.memoryClock = RADEON_INFO_CURRENT_GPU_MCLK;
        codes.registry = RADEON_INFO_READ_REG;
#endif // RADEON_INFO_CURRENT_GPU_TEMP
        codes.maxCoreClock = RADEON_INFO_MAX_SCLK;
        codes.vramUsage = RADEON_INFO_VRAM_USAGE;
    }
#endif // __RADEON_DRM_H__

#ifdef __AMDGPU_DRM_H__
    if (driver == "amdgpu") {
        // https://cgit.freedesktop.org/mesa/drm/tree/include/drm/amdgpu_drm.h#n437
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/amdgpu_drm.h#n471
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#n212
        codes.request = DRM_IOCTL_AMDGPU_INFO;
#  ifdef AMDGPU_INFO_VCE_CLOCK_TABLE // Available only in Linux 4.10 and above
        codes.coreClock = AMDGPU_INFO_VCE_CLOCK_TABLE;
        codes.memoryClock = AMDGPU_INFO_VCE_CLOCK_TABLE;
#  endif // AMDGPU_INFO_VCE_CLOCK_TABLE
        codes.maxCoreClock = AMDGPU_INFO_DEV_INFO;
        codes.vramUsage = AMDGPU_INFO_VRAM_USAGE;
        codes.vramSize = AMDGPU_INFO_VRAM_GTT;
        codes.registry = AMDGPU_INFO_READ_MMR_REG;
    }
#endif // __AMDGPU_DRM_H__
}


QString ioctlHandler::getDriverName(){
    if(fd < 0)
        return "";

#define NAME_SIZE 10
    char driver[NAME_SIZE] = "";

#ifdef _DRM_H_
    drm_version_t v;
    CLEAN(v);
    v.name = driver;
    v.name_len = NAME_SIZE;
    if(ioctl(fd, DRM_IOCTL_VERSION, &v)){
        DESCRIBE_ERROR("ioctl version");
        return "";
    }
#endif // _DRM_H_

    return QString(driver);
}


bool ioctlHandler::isValid(){
    if(fd < 0){
        qDebug() << "Ioctl: file descriptor not available";
        return false;
    }

    if(codes.request == UNAVAILABLE){
        qDebug() << "Ioctl: request code not available";
        return false;
    }

    if(ioctl(fd, codes.request) && (errno == EACCES)){
        qDebug() << "Ioctl: drm render node not available and no root access";
        return false;
    }

    return true;
}


ioctlHandler::~ioctlHandler(){
    if((fd >= 0) && close(fd))
        DESCRIBE_ERROR("fd close");
}


bool ioctlHandler::getGpuUsage(float *data, unsigned time, unsigned frequency){
    /* Sample the GPU status register N times and check how many of these samples have the GPU busy
     * Register documentation:
     * http://support.amd.com/TechDocs/46142.pdf#page=246   (address 0x8010, 31st bit)
     */
#define REGISTRY_ADDRESS 0x8010
#define REGISTRY_MASK 1<<31
#define ONE_SECOND 1000000
    const unsigned int sleep = ONE_SECOND/frequency;
    unsigned int remaining, activeCount = 0, totalCount = 0, buffer;
    for(remaining = time; (remaining > 0) && (remaining <= time); // Cycle until the time has finished
            remaining -= (sleep - usleep(sleep)), totalCount++){ // On each cycle sleep and subtract the slept time from the remaining
        buffer = REGISTRY_ADDRESS;
        bool success = readRegistry(&buffer);

        if(Q_UNLIKELY(!success)) // Read failed
            return false;

        if(REGISTRY_MASK & buffer) // Check if the activity bit is 1
            activeCount++;
    }

    if(Q_UNLIKELY(totalCount == 0))
        return false;

    *data = (100.0f * activeCount) / totalCount;

    // qDebug() << activeCount << "active moments out of" << totalCount << "(" << *data << "%) in" << time/1000 << "mS (Sampling at" << frequency << "Hz)";
    return true;
}


bool ioctlHandler::getValue(void *data, unsigned dataSize, unsigned command){
    switch(codes.request){
#ifdef __RADEON_DRM_H__
    case DRM_IOCTL_RADEON_INFO:{ // Radeon driver
        struct drm_radeon_info buffer;
        CLEAN(buffer);
        buffer.request = command;
        buffer.value = (uint64_t)data;
        bool success = !ioctl(fd, codes.request, &buffer);
        if(Q_UNLIKELY(!success))
            DESCRIBE_ERROR("ioctl");
        return success;
    }
#endif // __RADEON_DRM_H__

#ifdef __AMDGPU_DRM_H__
    case DRM_IOCTL_AMDGPU_INFO:{ // Amdgpu driver
        struct drm_amdgpu_info buffer;
        CLEAN(buffer);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)data;
        buffer.return_size = dataSize;
        bool success = !ioctl(fd, codes.request, &buffer);
        if(Q_UNLIKELY(!success))
            DESCRIBE_ERROR("ioctl");
        return success;
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(command)
    Q_UNUSED(dataSize)
#endif // __AMDGPU_DRM_H__

    default: // Unknown driver or not initialized
        qWarning("ioctlHandler not initialized correctly");
        return false;
    }
}


bool ioctlHandler::getTemperature(int *data){
    return (codes.temperature != UNAVAILABLE) && getValue(data, sizeof(*data), codes.temperature);
}


bool ioctlHandler::getCoreClock(unsigned *data){
    switch(codes.coreClock){
#ifdef AMDGPU_INFO_VCE_CLOCK_TABLE
    case AMDGPU_INFO_VCE_CLOCK_TABLE:{
        struct drm_amdgpu_info_vce_clock_table table;
        CLEAN(table);
        bool success = getValue(&table, sizeof(table), codes.coreClock);
        if(success && table.num_valid_entries > 0)
            *data = table.entries[0].sclk;
        return success;
    }
#endif // AMDGPU_INFO_VCE_CLOCK_TABLE

    case UNAVAILABLE:
        return false;

    default:
        return getValue(data, sizeof(*data), codes.coreClock);
    }
}


bool ioctlHandler::getMaxCoreClock(unsigned *data){
    switch (codes.maxCoreClock) {
#ifdef __AMDGPU_DRM_H__
    case AMDGPU_INFO_DEV_INFO:{
        struct drm_amdgpu_info_device info;
        CLEAN(info);
        bool success = getValue(&info, sizeof(info), codes.maxCoreClock);
        if(success)
            *data = info.max_engine_clock;
        return success;
    }
#endif // __AMDGPU_DRM_H__

    case UNAVAILABLE:
        return false;

    default:
        return getValue(data, sizeof(*data), codes.maxCoreClock);
    }
}


bool ioctlHandler::getMemoryClock(unsigned *data){
    switch(codes.memoryClock){
#ifdef AMDGPU_INFO_VCE_CLOCK_TABLE
    case AMDGPU_INFO_VCE_CLOCK_TABLE:{
        struct drm_amdgpu_info_vce_clock_table table;
        CLEAN(table);
        bool success = getValue(&table, sizeof(table), codes.memoryClock);
        if(success && table.num_valid_entries > 0)
            *data = table.entries[0].mclk;
        return success;
    }
#endif // AMDGPU_INFO_VCE_CLOCK_TABLE

    case UNAVAILABLE:
        return false;

    default:
        return getValue(data, sizeof(*data), codes.memoryClock);
    }
}


bool ioctlHandler::getVramUsage(unsigned long *data){
    return (codes.vramUsage != UNAVAILABLE) && getValue(data, sizeof(*data), codes.vramUsage);
}


bool ioctlHandler::readRegistry(unsigned *data){
    return (codes.registry != UNAVAILABLE) && getValue(data, sizeof(*data), codes.registry);
}


bool ioctlHandler::getVramSize(unsigned long *data){
    switch(codes.vramSize){
#ifdef __AMDGPU_DRM_H__
    case AMDGPU_INFO_VRAM_GTT:{
        struct drm_amdgpu_info_vram_gtt info;
        CLEAN(info);
        bool success = getValue(&info, sizeof(info), codes.vramSize);
        if(success)
            *data = info.vram_size;
        return success;
    }
#endif

    case UNAVAILABLE:
        return false;

    default:
        return getValue(data, sizeof(*data), codes.vramSize);
    }
}

