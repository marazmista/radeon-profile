#ifdef ENABLE_IOCTL

#include "radeon_ioctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>

#include <drm/drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/drm.h#L709
#include <drm/radeon_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993
#include <drm/amdgpu_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441

#define CLEAN(object) memset(&(object), 0, sizeof(object))
#define PATH_SIZE 20

int openCardFD(const char * const card){
    int fd;
    char path[PATH_SIZE] = "/dev/dri/";

    strncat(path, card, PATH_SIZE);
    fd= open(path,O_RDONLY);

    if(fd < 0)
        perror("open");
    else
        printf("%s open on fd %d\n", path, fd);

    return fd;
}

void closeCardFD(const int fd){
    if(close(fd))
        perror("close");
    else
        printf("Closed fd %d\n", fd);
}

struct buffer getValue(int fd, uint32_t command){
    struct buffer data;
    char errStr[30];

    CLEAN(data);

    switch(command){
    //RADEON
    case RADEON_INFO_CURRENT_GPU_MCLK:
    case RADEON_INFO_CURRENT_GPU_SCLK:
    case RADEON_INFO_CURRENT_GPU_TEMP:
    case RADEON_INFO_GTT_USAGE:
    case RADEON_INFO_VRAM_USAGE:
    {
        struct drm_radeon_info buffer;
        CLEAN(buffer);
        buffer.request = command;
        buffer.value = (uint64_t)&(data.value);
        buffer.pad = sizeof(data.value);

        data.error = ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
        if(data.error)
            sprintf(errStr, "ioctl (fd=%d, request=%#lx, command=%#x)", fd, DRM_IOCTL_RADEON_INFO, command);
        break;
    }

    // AMDGPU
    case AMDGPU_INFO_GTT_USAGE:
    case AMDGPU_INFO_VRAM_USAGE:
    {
        struct drm_amdgpu_info buffer;
        CLEAN(buffer);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)&(data.value);
        buffer.return_size = sizeof(data.value);

        data.error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
        if(data.error)
            sprintf(errStr, "ioctl (fd=%d, request=%#lx, command=%#x)", fd, DRM_IOCTL_AMDGPU_INFO, command);
        break;
    }


    // UNKNOWN
    default:
        data.error = 1;
        sprintf(errStr, "Unknown ioctl command: %#x", command);
    }

    if(data.error)
        perror(errStr);

    return data;
}

struct buffer radeonTemperature(int const fd){
    return getValue(fd, RADEON_INFO_CURRENT_GPU_TEMP);
}

struct buffer radeonCoreClock(int const fd){
    return getValue(fd, RADEON_INFO_CURRENT_GPU_SCLK);
}

struct buffer radeonMemoryClock(int const fd){
    return getValue(fd, RADEON_INFO_CURRENT_GPU_MCLK);
}

struct buffer radeonVramUsage(int const fd){
    return getValue(fd, RADEON_INFO_VRAM_USAGE);
}

struct buffer radeonGttUsage(int const fd){
    return getValue(fd, RADEON_INFO_GTT_USAGE);
}

struct buffer amdgpuVramUsage(int const fd){
    return getValue(fd, AMDGPU_INFO_VRAM_USAGE);
}

struct buffer amdgpuGttUsage(int const fd){
    return getValue(fd, AMDGPU_INFO_GTT_USAGE);
}

#endif
