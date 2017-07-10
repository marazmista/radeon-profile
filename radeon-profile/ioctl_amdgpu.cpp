#include "ioctlHandler.h"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cstdio> // perror()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  ifndef NO_AMDGPU_IOCTL // Include libdrm amdgpu headers only if NO_AMDGPU_IOCTL is not declared
#    include <libdrm/amdgpu_drm.h> // amdgpu ioctl codes and structs
#  endif
#endif


amdgpuIoctlHandler::amdgpuIoctlHandler(unsigned cardIndex) : ioctlHandler(cardIndex) { }

bool amdgpuIoctlHandler::getSensorValue(void *data, unsigned dataSize, unsigned sensor) const {
#if defined(DRM_IOCTL_AMDGPU_INFO) && defined(AMDGPU_INFO_SENSOR)
    struct drm_amdgpu_info buffer = {};
    buffer.query = AMDGPU_INFO_SENSOR;
    buffer.return_pointer = reinterpret_cast<uint64_t>(data);
    buffer.return_size = dataSize;
    buffer.sensor_info.type = sensor;
    bool success = !ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    if (Q_UNLIKELY(!success))
        perror("DRM_IOCTL_AMDGPU_INFO");
    return success;
#else
    Q_UNUSED(data);
    Q_UNUSED(sensor);
    Q_UNUSED(dataSize);
    return false;
#endif
}


/**
 * @see https://cgit.freedesktop.org/mesa/drm/tree/include/drm/amdgpu_drm.h#n535
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

bool amdgpuIoctlHandler::getGpuUsage(float *data) const {
#ifdef AMDGPU_INFO_SENSOR_GPU_LOAD
    return getSensorValue(data, sizeof(data), AMDGPU_INFO_SENSOR_GPU_LOAD);
#else
    Q_UNUSED(data);
    return false;
#endif
}

bool amdgpuIoctlHandler::getTemperature(int *data) const {
#ifdef AMDGPU_INFO_SENSOR_GPU_TEMP
    return getSensorValue(data, sizeof(data), AMDGPU_INFO_SENSOR_GPU_TEMP);
#else
    Q_UNUSED(data);
    return false;
#endif
}

bool amdgpuIoctlHandler::getCoreClock(int *data) const {
#ifdef AMDGPU_INFO_SENSOR_GFX_SCLK
    return getSensorValue(data, sizeof(data), AMDGPU_INFO_SENSOR_GFX_SCLK);
#else
    Q_UNUSED(data);
    return false;
#endif
}

bool amdgpuIoctlHandler::getMaxCoreClock(int *data) const {
    return getMaxClocks(data, NULL);
}


bool amdgpuIoctlHandler::getMaxMemoryClock(int *data) const {
    return getMaxClocks(NULL, data);
}


bool amdgpuIoctlHandler::getMaxClocks(int *core, int *memory) const {
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


bool amdgpuIoctlHandler::getMemoryClock(int *data) const {
#ifdef  AMDGPU_INFO_SENSOR_GFX_MCLK
    return getSensorValue(data, sizeof(data), AMDGPU_INFO_SENSOR_GFX_MCLK);
#else
    Q_UNUSED(data);
    return false;
#endif
}


bool amdgpuIoctlHandler::getVceClocks(int *core, int *memory) const {
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


bool amdgpuIoctlHandler::getVramUsage(long *data) const {
#ifdef AMDGPU_INFO_VRAM_USAGE
    return getValue(data, sizeof(*data), AMDGPU_INFO_VRAM_USAGE);
#else
    return false;
    Q_UNUSED(data);
#endif
}


bool amdgpuIoctlHandler::getVramSize(float *data) const {
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
