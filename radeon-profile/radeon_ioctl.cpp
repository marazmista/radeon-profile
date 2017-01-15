#include "radeon_ioctl.h"


#ifdef NO_IOCTL // Do not compile ioctl functions (useful if kernel headers can't be installed)

ioctlHandler::ioctlHandler(unsigned card, QString driver){Q_UNUSED(card);Q_UNUSED(driver);}
ioctlHandler::~ioctlHandler(){}
bool ioctlHandler::isValid(){return false;}
bool ioctlHandler::getTemperature(int *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getCoreClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getMemoryClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getMaxCoreClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getVramUsage(unsigned long *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getVramSize(unsigned long *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getGpuUsage(float *data, unsigned time, unsigned frequency){
    return false;
    Q_UNUSED(data);
    Q_UNUSED(time);
    Q_UNUSED(frequency);
}

#else

// NO_IOCTL is not defined, drm headers must be available for compilation

#ifdef __has_include // If the compiler supports the __has_include macro (clang or gcc>=5.0)
#  if !__has_include(<drm/radeon_drm.h>)
#    error radeon_drm.h is not available! Install kernel headers or compile with the flag -DNO_IOCTL
#  endif
#  ifndef NO_AMDGPU_IOCTL
#    if !__has_include(<drm/amdgpu_drm.h>) // Available only in Linux 4.2 and above
#      error amdgpu_drm.h is not available! Install kernel headers or compile with the flag -DNO_AMDGPU_IOCTL
#    endif
#  endif
#endif

// Includes

#include <QString>
#include <QDebug>

#include <cstring> // memset(), strerror()
#include <cstdio> // snprintf()
#include <cerrno> // errno
#include <unistd.h> // close(), usleep()
#include <fcntl.h> // open()
#include <sys/ioctl.h> // ioctl()

#include <drm/radeon_drm.h> // radeon ioctl codes and structs
#ifndef NO_AMDGPU_IOCTL
#  include <drm/amdgpu_drm.h> // amdgpu ioctl codes and structs
#endif

// Macros

#define DESCRIBE_ERROR(title) qWarning() << (title) << ':' << strerror(errno)
#define CLEAN(object) memset(&(object), 0, sizeof(object))
#define UNAVAILABLE 0
#define PATH_SIZE 20

// Class functions

ioctlHandler::ioctlHandler(unsigned card, QString driver){
    if(driver.isEmpty())
        return;

    /* Open file descriptor to card device
     * Information ioctls can be accessed in two ways:
     * The kernel generates for the card with index N the files /dev/dri/card<N> and /dev/dri/renderD<128+N>
     * Both can be opened without root access
     * Executing ioctls on /dev/dri/card<N> requires either root access or being DRM Master
     * /dev/dri/renderD<128+N> does not require these permissions, but legacy kernels do not support it
     * For more details:
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#Render_nodes
     * https://www.x.org/wiki/Events/XDC2013/XDC2013DavidHerrmannDRMSecurity/slides.pdf#page=15
     */
     // Try /dev/dri/renderD<128+N>
    char path[PATH_SIZE];
    snprintf(path, PATH_SIZE, "/dev/dri/renderD%u", 128+card);
    qDebug() << "Opening" << path << "for IOCTLs with driver" << driver;
    fd = open(path, O_RDONLY);
    if(fd < 0){
        DESCRIBE_ERROR("fd open");

        // /dev/dri/renderD<128+N> not available, try /dev/dri/card<N>
        snprintf(path, PATH_SIZE, "/dev/dri/card%u", card);
        qDebug() << "Opening" << path << "for IOCTLs with driver" << driver;
        fd = open(path, O_RDONLY);
        if(fd < 0){
            DESCRIBE_ERROR("fd open");
            // /dev/dri/card<N> not available, initialization failed
            return;
        }

        /* Try drm authorization to card device, to allow non-root ioctls if the process is DRM master
         * Won't work in radeon-profile since as graphical app the DRM master will be the display server
         * But it might work in radeon-profile-daemon
        drm_auth_t auth;
        if(ioctl(fd, DRM_IOCTL_GET_MAGIC, &auth) || ioctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth))
            DESCRIBE_ERROR("auth_magic");
        */
    }

    // Initialize ioctl codes
    if(driver=="radeon"){
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/radeon_drm.h#n993
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/radeon_kms.c#n203
        codes.request = DRM_IOCTL_RADEON_INFO;
        codes.temperature = RADEON_INFO_CURRENT_GPU_TEMP;
        codes.coreClock = RADEON_INFO_CURRENT_GPU_SCLK;
        codes.maxCoreClock = RADEON_INFO_MAX_SCLK;
        codes.memoryClock = RADEON_INFO_CURRENT_GPU_MCLK;
        codes.vramUsage = RADEON_INFO_VRAM_USAGE;
        codes.vramSize = UNAVAILABLE;
        codes.registry = RADEON_INFO_READ_REG;
    }
#ifdef __AMDGPU_DRM_H__
    else if (driver == "amdgpu") {
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/amdgpu_drm.h#n471
        // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#n212
        codes.request = DRM_IOCTL_AMDGPU_INFO;
        codes.temperature = UNAVAILABLE;

#  ifdef AMDGPU_INFO_VCE_CLOCK_TABLE // Available only in Linux 4.10 and above
        codes.coreClock = AMDGPU_INFO_VCE_CLOCK_TABLE;
        codes.memoryClock = AMDGPU_INFO_VCE_CLOCK_TABLE;
#  else
        codes.coreClock = UNAVAILABLE;
        codes.memoryClock = UNAVAILABLE;
#  endif

        codes.maxCoreClock = UNAVAILABLE;
        codes.vramUsage = AMDGPU_INFO_VRAM_USAGE;
        codes.vramSize = AMDGPU_INFO_VRAM_GTT;
        codes.registry = AMDGPU_INFO_READ_MMR_REG;
    }
#endif
       else {
        codes = {
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE,
            UNAVAILABLE
        };
    }
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
        qDebug() << "Ioctl: drm authentication failed and no root access";
        return false;
    }

    return true;
}


ioctlHandler::~ioctlHandler(){
    if(close(fd))
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
    bool success = false;

    switch(codes.request){
    case DRM_IOCTL_RADEON_INFO:{ // Radeon driver
        struct drm_radeon_info buffer;
        CLEAN(buffer);
        buffer.request = command;
        buffer.value = (uint64_t)data;
        success = !ioctl(fd, codes.request, &buffer);
        break;
    }

#ifdef __AMDGPU_DRM_H__
    case DRM_IOCTL_AMDGPU_INFO:{ // Amdgpu driver
        struct drm_amdgpu_info buffer;
        CLEAN(buffer);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)data;
        buffer.return_size = dataSize;
        success = !ioctl(fd, codes.request, &buffer);
        break;
    }
#else
        Q_UNUSED(dataSize)
#endif

    default: // Unknown driver or not initialized
        qWarning("ioctlHandler not initialized correctly");
        return false;
    }

    if(Q_UNLIKELY(!success))
        DESCRIBE_ERROR("ioctl");

    return success;
}


bool ioctlHandler::getTemperature(int *data){
    return (codes.temperature != UNAVAILABLE) && getValue(data, sizeof(*data), codes.temperature);
}


bool ioctlHandler::getCoreClock(unsigned *data){
    switch(codes.coreClock){
#ifdef AMDGPU_INFO_VCE_CLOCK_TABLE // Implicit: ifdef __AMDGPU_DRM_H__
    case AMDGPU_INFO_VCE_CLOCK_TABLE:{
        drm_amdgpu_info_vce_clock_table table;
        bool success = getValue(&table, sizeof(table), codes.coreClock);
        if(success && table.num_valid_entries > 0)
            *data = table.entries[0].sclk;
        return success;
    }
#endif

    case UNAVAILABLE:
        return false;

    default:
        return getValue(data, sizeof(*data), codes.coreClock);
    }
}


bool ioctlHandler::getMaxCoreClock(unsigned *data){
    return (codes.maxCoreClock != UNAVAILABLE) && getValue(data, sizeof(*data), codes.maxCoreClock);
}


bool ioctlHandler::getMemoryClock(unsigned *data){
    switch(codes.memoryClock){
#ifdef AMDGPU_INFO_VCE_CLOCK_TABLE // Implicit: ifdef __AMDGPU_DRM_H__
    case AMDGPU_INFO_VCE_CLOCK_TABLE:{
        drm_amdgpu_info_vce_clock_table table;
        bool success = getValue(&table, sizeof(table), codes.memoryClock);
        if(success && table.num_valid_entries > 0)
            *data = table.entries[0].mclk;
        return success;
    }
#endif

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


#endif