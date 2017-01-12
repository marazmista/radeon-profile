#include "radeon_ioctl.h"


#ifdef NO_IOCTL

ioctlHandler::ioctlHandler(QString card, QString driver){Q_UNUSED(card);Q_UNUSED(driver);}
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

ioctlHandler::ioctlHandler(QString card, QString driver){
    if(card.isEmpty() || driver.isEmpty())
        return;

    // Open file descriptor to card device
    const char * path = ("/dev/dri/" + card).toStdString().c_str();
    qDebug() << "Opening" << path << "for IOCTLs with driver" << driver;
    fd = open(path, O_RDONLY);
    if(fd < 0){
        DESCRIBE_ERROR("fd open");
        return;
    }

    // Initialize ioctl codes
    if(driver=="radeon"){
        // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993
        // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L203
        codes = {
            request: DRM_IOCTL_RADEON_INFO,
            temperature: RADEON_INFO_CURRENT_GPU_TEMP,
            coreClock: RADEON_INFO_CURRENT_GPU_SCLK,
            maxCoreClock: RADEON_INFO_MAX_SCLK,
            memoryClock: RADEON_INFO_CURRENT_GPU_MCLK,
            vramUsage: RADEON_INFO_VRAM_USAGE,
            vramSize: UNAVAILABLE,
            registry:RADEON_INFO_READ_REG
        };
    } else if (driver == "amdgpu") {
        // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441
        // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L212
        codes = {
            request: DRM_IOCTL_AMDGPU_INFO,
            temperature: UNAVAILABLE,
            coreClock: UNAVAILABLE,
            maxCoreClock: UNAVAILABLE,
            memoryClock: UNAVAILABLE,
            vramUsage: AMDGPU_INFO_VRAM_USAGE,
            vramSize: AMDGPU_INFO_VRAM_GTT,
            registry: AMDGPU_INFO_READ_MMR_REG
        };
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

    // Try drm authorization to card device, to allow non-root ioctls
    // https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
    // https://www.x.org/wiki/Events/XDC2013/XDC2013DavidHerrmannDRMSecurity/slides.pdf#page=15
    // Works only if DRM master, never true since this is a graphical application and the DRM master will be the display server
    // It might work in radeon-profile-daemon
    drm_auth_t auth;
    if(ioctl(fd, DRM_IOCTL_GET_MAGIC, &auth) || ioctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth))
        DESCRIBE_ERROR("auth_magic");
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
#define REGISTRY_ADDRESS 0x8010 // http://support.amd.com/TechDocs/46142.pdf#page=246
#define REGISTRY_MASK 1<<31
#define ONE_SECOND 1000000
    const unsigned int sleep = ONE_SECOND/frequency;
    unsigned int remaining, activeCount = 0, totalCount = 0, buffer;
    for(remaining = time; (remaining > 0) && (remaining <= time); remaining -= (sleep - usleep(sleep)), totalCount++){
        buffer = REGISTRY_ADDRESS;
        bool success = readRegistry(&buffer);

        if(!success)
            return false;

        if(REGISTRY_MASK & buffer)
            activeCount++;
    }

    if(totalCount == 0)
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

    if(!success)
        DESCRIBE_ERROR("ioctl");

    return success;
}


bool ioctlHandler::getTemperature(int *data){
    return (codes.temperature != UNAVAILABLE) && getValue(data, sizeof(data), codes.temperature);
}


bool ioctlHandler::getCoreClock(unsigned *data){
    return (codes.coreClock != UNAVAILABLE) && getValue(data, sizeof(data), codes.coreClock);
}


bool ioctlHandler::getMaxCoreClock(unsigned *data){
    return (codes.maxCoreClock != UNAVAILABLE) && getValue(data, sizeof(data), codes.maxCoreClock);
}


bool ioctlHandler::getMemoryClock(unsigned *data){
    return (codes.memoryClock != UNAVAILABLE) && getValue(data, sizeof(data), codes.memoryClock);
}


bool ioctlHandler::getVramUsage(unsigned long *data){
    return (codes.vramUsage != UNAVAILABLE) && getValue(data, sizeof(data), codes.vramUsage);
}


bool ioctlHandler::readRegistry(unsigned *data){
    return (codes.registry != UNAVAILABLE) && getValue(data, sizeof(data), codes.registry);
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
