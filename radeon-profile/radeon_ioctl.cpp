#include "radeon_ioctl.h"


#ifdef NO_IOCTL

ioctlHandler::ioctlHandler(unsigned card, QString driver){Q_UNUSED(card);Q_UNUSED(driver);}
ioctlHandler::~ioctlHandler(){}
bool ioctlHandler::isValid(){return false;}
bool ioctlHandler::getTemperature(int *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getCoreClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getMemoryClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getMaxCoreClock(unsigned *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getVramUsage(unsigned long *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getVramSize(unsigned long *data){return false; Q_UNUSED(data);}
bool ioctlHandler::getGpuUsage(float *data, unsigned time, unsigned frequency){return false; Q_UNUSED(data);Q_UNUSED(time);Q_UNUSED(frequency);}

#else

#include <QFile>
#include <QString>
#include <QDebug>

#include <cstring>
#include <cstdio>
#include <cerrno>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <drm/radeon_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993
#include <drm/amdgpu_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441
}

#define DESCRIBE_ERROR(title) qWarning() << (title) << ':' << strerror(errno)
#define CLEAN(object) memset(&(object), 0, sizeof(object))
#define UNAVAILABLE 0
#define PATH_SIZE 20

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
        // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993
        // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L203
        codes.request = DRM_IOCTL_RADEON_INFO;
        codes.temperature = RADEON_INFO_CURRENT_GPU_TEMP;
        codes.coreClock = RADEON_INFO_CURRENT_GPU_SCLK;
        codes.maxCoreClock = RADEON_INFO_MAX_SCLK;
        codes.memoryClock = RADEON_INFO_CURRENT_GPU_MCLK;
        codes.vramUsage = RADEON_INFO_VRAM_USAGE;
        codes.vramSize = UNAVAILABLE;
        codes.registry = RADEON_INFO_READ_REG;
    } else if (driver == "amdgpu") {
        // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441
        // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L212
        codes.request = DRM_IOCTL_AMDGPU_INFO;
        codes.temperature = UNAVAILABLE;
        codes.coreClock = UNAVAILABLE;
        codes.maxCoreClock = UNAVAILABLE;
        codes.memoryClock = UNAVAILABLE;
        codes.vramUsage = AMDGPU_INFO_VRAM_USAGE;
        codes.vramSize = AMDGPU_INFO_VRAM_GTT;
        codes.registry = AMDGPU_INFO_READ_MMR_REG;
    } else {
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

    if((codes.request != DRM_IOCTL_RADEON_INFO) && (codes.request != DRM_IOCTL_AMDGPU_INFO)){
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

    case DRM_IOCTL_AMDGPU_INFO:{ // Amdgpu driver
        struct drm_amdgpu_info buffer;
        CLEAN(buffer);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)data;
        buffer.return_size = dataSize;
        success = !ioctl(fd, codes.request, &buffer);
        break;
    }

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
    return (codes.coreClock != UNAVAILABLE) && getValue(data, sizeof(*data), codes.coreClock);
}


bool ioctlHandler::getMaxCoreClock(unsigned *data){
    return (codes.maxCoreClock != UNAVAILABLE) && getValue(data, sizeof(*data), codes.maxCoreClock);
}


bool ioctlHandler::getMemoryClock(unsigned *data){
    return (codes.memoryClock != UNAVAILABLE) && getValue(data, sizeof(*data), codes.memoryClock);
}


bool ioctlHandler::getVramUsage(unsigned long *data){
    return (codes.vramUsage != UNAVAILABLE) && getValue(data, sizeof(*data), codes.vramUsage);
}


bool ioctlHandler::readRegistry(unsigned *data){
    return (codes.registry != UNAVAILABLE) && getValue(data, sizeof(*data), codes.registry);
}


bool ioctlHandler::getVramSize(unsigned long *data){
    bool success = false;

    if(codes.vramSize == AMDGPU_INFO_VRAM_GTT){
        struct drm_amdgpu_info_vram_gtt info;
        CLEAN(info);
        bool success = getValue(&info, sizeof(info), codes.vramSize);
        if(success)
            *data = info.vram_size;
    }

    return success;
}


#endif
