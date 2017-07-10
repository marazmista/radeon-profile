#include "ioctlHandler.h"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cstdio> // perror()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  include <libdrm/radeon_drm.h> // radeon ioctl codes and structs
#endif


radeonIoctlHandler::radeonIoctlHandler(unsigned cardIndex) : ioctlHandler(cardIndex){}

/**
 * @see https://cgit.freedesktop.org/mesa/drm/tree/include/drm/radeon_drm.h#n993
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/radeon_drm.h#n993
 */
bool radeonIoctlHandler::getValue(void *data, unsigned dataSize, unsigned command) const {
#ifdef DRM_IOCTL_RADEON_INFO
    struct drm_radeon_info buffer = {};
    buffer.request = command;
    buffer.value = reinterpret_cast<uint64_t>(data);
    const bool success = !ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
    if(Q_UNLIKELY(!success))
        perror("DRM_IOCTL_RADEON_INFO");
    return success;
#else
    return false;
    Q_UNUSED(data);
    Q_UNUSED(command);
#endif

    Q_UNUSED(dataSize);
}


bool radeonIoctlHandler::getTemperature(int *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_TEMP // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_TEMP);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getCoreClock(int *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_SCLK // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_SCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::getMaxCoreClock(int *data) const {
#ifdef RADEON_INFO_MAX_SCLK
    return getValue(data, sizeof(*data), RADEON_INFO_MAX_SCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}

bool radeonIoctlHandler::getMaxMemoryClock(int *data) const {
    qDebug() << "radeonIoctlHandler::getMaxMemoryClock() is not available";
    return false;
    Q_UNUSED(data);
}


bool radeonIoctlHandler::getMaxClocks(int *core, int *memory) const {
    *memory = -1;
    return getMaxCoreClock(core);
}


bool radeonIoctlHandler::getMemoryClock(int *data) const {
#ifdef RADEON_INFO_CURRENT_GPU_MCLK // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_CURRENT_GPU_MCLK);
#else
    return false;
    Q_UNUSED(data);
#endif
}

bool radeonIoctlHandler::getVramUsage(long *data) const {
#ifdef RADEON_INFO_VRAM_USAGE
    return getValue(data, sizeof(*data), RADEON_INFO_VRAM_USAGE);
#else
    return false;
    Q_UNUSED(data);
#endif
}

bool radeonIoctlHandler::getGpuUsage(float *data) const {
    return ioctlHandler::getGpuUsage(data, 500000, 150);
}

bool radeonIoctlHandler::getVramSize(float *data) const {
#ifdef DRM_IOCTL_RADEON_GEM_INFO
    struct drm_radeon_gem_info buffer = {};
    const bool success = !ioctl(fd, DRM_IOCTL_RADEON_GEM_INFO, &buffer);
    if(Q_LIKELY(success))
        *data = buffer.vram_size;
    else
        perror("DRM_IOCTL_RADEON_GEM_INFO");
    return success;
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool radeonIoctlHandler::readRegistry(unsigned *data) const {
#ifdef RADEON_INFO_READ_REG // Linux >= 4.1
    return getValue(data, sizeof(*data), RADEON_INFO_READ_REG);
#else
    return false;
    Q_UNUSED(data);
#endif
}

/**
 * Register address 0x8010, 32nd bit
 * @see http://support.amd.com/TechDocs/46142.pdf#page=246
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/sid.h#n942
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/r600d.h#n291
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/evergreend.h#n842
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/rv770d.h#n403
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/nid.h#n238
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/radeon/cikd.h#n1036
 */
bool radeonIoctlHandler::isCardActive(bool *data) const {
    unsigned reg = 0x8010;
    bool success = readRegistry(&reg);
    if(success)
        *data = reg & (1 << 31); // Check if the activity bit is 1
    return success;
}

