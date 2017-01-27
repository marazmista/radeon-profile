#include "ioctlHandler.hpp"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cstring> // memset()
#include <cstdio> // perror()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  include <libdrm/radeon_drm.h> // radeon ioctl codes and structs
#endif


radeonIoctlHandler::radeonIoctlHandler(unsigned cardIndex) : ioctlHandler(cardIndex){}

bool radeonIoctlHandler::getValue(void *data, unsigned dataSize, unsigned command) const {
    // https://cgit.freedesktop.org/mesa/drm/tree/include/drm/radeon_drm.h#n993
    // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/radeon_drm.h#n993
    // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/radeon_kms.c#n203

#ifdef DRM_IOCTL_RADEON_INFO
    struct drm_radeon_info buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.request = command;
    buffer.value = (uint64_t)data;
    bool success = !ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
    if(Q_UNLIKELY(!success))
        perror("ioctl");
    return success;
#else
    return false;
    Q_UNUSED(data);
    Q_UNUSED(command);
#endif

    Q_UNUSED(dataSize);
}


bool radeonIoctlHandler::isValid() const {
    if(fd < 0){
        qDebug() << "Ioctl: file descriptor not available";
        return false;
    }

#ifdef DRM_IOCTL_RADEON_INFO
    if(ioctl(fd, DRM_IOCTL_RADEON_INFO) && (errno == EACCES)){
        qDebug() << "Ioctl: drm render node not available and no root access";
        return false;
    }
#else
    return false;
#endif

    return true;
}


bool radeonIoctlHandler::getTemperature(int *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_TEMP // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_TEMP);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getCoreClock(unsigned *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_SCLK // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_SCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getMaxCoreClock(unsigned *data) const {
#ifdef RADEON_INFO_MAX_SCLK
    return getValue(data, sizeof(*data), RADEON_INFO_MAX_SCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getMemoryClock(unsigned *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_MCLK // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_MCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getClocks(unsigned *core, unsigned *memory) const {
    return ((core == NULL) || getCoreClock(core)) && ((memory == NULL) || getMemoryClock(memory));
}


bool radeonIoctlHandler::getVramUsage(unsigned long *data) const {
#ifdef RADEON_INFO_VRAM_USAGE
    return getValue(data, sizeof(*data), RADEON_INFO_VRAM_USAGE);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getVramSize(unsigned long *data) const {
    return false;
    Q_UNUSED(data);
}


bool radeonIoctlHandler::readRegistry(unsigned *data) const {
#ifdef RADEON_INFO_READ_REG // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_READ_REG);
#else
    return false;
    Q_UNUSED(data);
#endif
}
