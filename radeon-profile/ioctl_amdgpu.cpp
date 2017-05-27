#include "ioctlHandler.hpp"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cstdio> // perror()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  ifndef NO_AMDGPU_IOCTL // Include libdrm amdgpu headers only if NO_AMDGPU_IOCTL is not declared
#    include <libdrm/amdgpu_drm.h> // amdgpu ioctl codes and structs
#  endif
#endif


amdgpuIoctlHandler::amdgpuIoctlHandler(unsigned cardIndex) : ioctlHandler(cardIndex){}

/**
 * @see https://cgit.freedesktop.org/mesa/drm/tree/include/drm/amdgpu_drm.h#n437
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/drm/amdgpu_drm.h#n471
 */
bool amdgpuIoctlHandler::getValue(void *data, unsigned dataSize, unsigned command) const {
#ifdef DRM_IOCTL_AMDGPU_INFO
    struct drm_amdgpu_info buffer = {};
    buffer.query = command;
    buffer.return_pointer = reinterpret_cast<uint64_t>(data);
    buffer.return_size = dataSize;
    bool success = !ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    if(Q_UNLIKELY(!success))
        perror("DRM_IOCTL_AMDGPU_INFO");
    return success;
#else
    Q_UNUSED(data);
    Q_UNUSED(command);
    Q_UNUSED(dataSize);
    return false;
#endif
}


bool amdgpuIoctlHandler::getTemperature(int *data) const {
#ifdef AMDGPU_INFO_SENSOR // Linux >= 4.12
    struct drm_amdgpu_info buffer = {};
    buffer.query = AMDGPU_INFO_SENSOR;
    buffer.return_pointer = reinterpret_cast<uint64_t>(data);
    buffer.return_size = sizeof(*data);
    buffer.sensor_info.type = AMDGPU_INFO_SENSOR_GPU_TEMP;
    bool success = !ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    if(Q_UNLIKELY(!success))
        perror("DRM_IOCTL_AMDGPU_INFO AMDGPU_INFO_SENSOR");
    return success;
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool amdgpuIoctlHandler::getCoreClock(unsigned *data) const {
    return getClocks(data, NULL);
}


bool amdgpuIoctlHandler::getMaxCoreClock(unsigned *data) const {
    return getMaxClocks(data, NULL);
}


bool amdgpuIoctlHandler::getMaxMemoryClock(unsigned *data) const {
    return getMaxClocks(NULL, data);
}


bool amdgpuIoctlHandler::getMaxClocks(unsigned *core, unsigned *memory) const {
#ifdef AMDGPU_INFO_DEV_INFO
    struct drm_amdgpu_info_device info = {};
    bool success = getValue(&info, sizeof(info), AMDGPU_INFO_DEV_INFO);
    if(success){
        if(core != NULL)
            *core = info.max_engine_clock;
        if(memory != NULL)
            *memory = info.max_memory_clock;
    }
    return success;
#else
    return false;
    Q_UNUSED(core);
    Q_UNUSED(memory);
#endif
}


bool amdgpuIoctlHandler::getMemoryClock(unsigned *data) const {
    return getClocks(NULL, data);
}


bool amdgpuIoctlHandler::getClocks(unsigned *core, unsigned *memory) const {
#ifdef AMDGPU_INFO_VCE_CLOCK_TABLE // Linux >= 4.10
    struct drm_amdgpu_info_vce_clock_table table = {};
    bool success = getValue(&table, sizeof(table), AMDGPU_INFO_VCE_CLOCK_TABLE) && (table.num_valid_entries > 0);
    if(success){
        if(core != NULL)
            *core = table.entries[0].sclk;
        if(memory != NULL)
            *memory = table.entries[0].mclk;
    }
    return success;
#else
    return false;
    Q_UNUSED(core);
    Q_UNUSED(memory)
#endif
}


bool amdgpuIoctlHandler::getVramUsage(unsigned long *data) const {
#ifdef AMDGPU_INFO_VRAM_USAGE
    return getValue(data, sizeof(*data), AMDGPU_INFO_VRAM_USAGE);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool amdgpuIoctlHandler::getVramSize(unsigned long *data) const {
#ifdef AMDGPU_INFO_VRAM_GTT
    struct drm_amdgpu_info_vram_gtt info = {};
    bool success = getValue(&info, sizeof(info), AMDGPU_INFO_VRAM_GTT);
    if(success)
        *data = info.vram_size;
    return success;
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool amdgpuIoctlHandler::readRegistry(unsigned *data) const {
#ifdef AMDGPU_INFO_READ_MMR_REG
    return getValue(data, sizeof(*data), AMDGPU_INFO_READ_MMR_REG);
#else
    return false;
    Q_UNUSED(data);
#endif
}

/**
 * Register address 0x2004, 32nd bit
 * @see https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/amd/include/asic_reg/si/sid.h#n941
 */
bool amdgpuIoctlHandler::isCardActive(bool *data) const {
    unsigned reg = 0x2004;
    bool success = readRegistry(&reg);
    if(success)
        *data = reg & (1 << 31);
    return success;
}
