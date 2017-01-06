#include "radeon_ioctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>

#define CLEAN(object) memset(&(object), 0, sizeof(object))
#define PATH_SIZE 20

int openCardFD(const char * const card){
    int fd;
    char path[PATH_SIZE] = "/dev/dri/";

    strncat(path, card, PATH_SIZE);
    fd= open(path,O_RDONLY);

    if(fd < 0)
        perror("open");

    return fd;
}

void closeCardFD(const int fd){
    if(close(fd))
        perror("close");
}


#ifdef NO_IOCTL


void radeonTemperature(int const fd, struct buffer *data){(void)fd; data->error=1;}
void radeonCoreClock(int const fd, struct buffer *data){(void)fd; data->error=1;}
void radeonMemoryClock(int const fd, struct buffer *data){(void)fd; data->error=1;}
void radeonMaxCoreClock(int const fd, struct buffer *data){(void)fd; data->error=1;}
void radeonVramUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}
void radeonGttUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuVramSize(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuVramUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuGttUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}


#else


#include <drm/radeon_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993
#include <drm/amdgpu_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441

#define UNKNOWN_COMMAND INT32_MIN

void getValue(int fd, struct buffer *data, uint32_t command){
    uint32_t request;

    switch(command){
    //RADEON
    case RADEON_INFO_CURRENT_GPU_MCLK:
    case RADEON_INFO_CURRENT_GPU_SCLK:
    case RADEON_INFO_CURRENT_GPU_TEMP:
    case RADEON_INFO_GTT_USAGE:
    case RADEON_INFO_MAX_SCLK:
    case RADEON_INFO_VRAM_USAGE:
    {
        struct drm_radeon_info buffer;
        CLEAN(buffer);
        buffer.request = command;
        buffer.value = (uint64_t)&(data->value);
        buffer.pad = sizeof(data->value);
        request = DRM_IOCTL_RADEON_INFO;

        data->error = ioctl(fd, request, &buffer);
        break;
    }

    // AMDGPU
    case AMDGPU_INFO_GTT_USAGE:
    case AMDGPU_INFO_VRAM_USAGE:
    {
        struct drm_amdgpu_info buffer;
        CLEAN(buffer);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)&(data->value);
        buffer.return_size = sizeof(data->value);
        request = DRM_IOCTL_AMDGPU_INFO;

        data->error = ioctl(fd, request, &buffer);
        break;
    }

    case AMDGPU_INFO_VRAM_GTT:
    {
        struct drm_amdgpu_info buffer;
        struct drm_amdgpu_info_vram_gtt info;
        CLEAN(buffer);
        CLEAN(info);
        buffer.query = command;
        buffer.return_pointer = (uint64_t)&info;
        buffer.return_size = sizeof(info);
        request = DRM_IOCTL_AMDGPU_INFO;

        data->error = ioctl(fd, request, &buffer);
        data->value.byte = info.vram_size;
        break;
    }

    // UNKNOWN
    default:
        data->error = UNKNOWN_COMMAND;
        printf("Unknown ioctl command: %#x", command);
        break;
    }


    if(data->error && (data->error != UNKNOWN_COMMAND)){
        char errStr[30];
        sprintf(errStr, "ioctl (fd=%d, request=%#x, command=%#x)", fd, request, command);
        perror(errStr);
    }
}

void radeonTemperature(int const fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_CURRENT_GPU_TEMP); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L555
}

void radeonCoreClock(int const fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_CURRENT_GPU_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L562
}

void radeonMaxCoreClock(const int fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_MAX_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L511
}

void radeonMemoryClock(int const fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_CURRENT_GPU_MCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L569
}

void radeonVramUsage(int const fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L529
}

void radeonGttUsage(int const fd, struct buffer *data){
    getValue(fd, data, RADEON_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L534
}

void amdgpuVramSize(const int fd, struct buffer *data){
    getValue(fd, data, AMDGPU_INFO_VRAM_GTT); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L404
}

void amdgpuVramUsage(int const fd, struct buffer *data){
    getValue(fd, data, AMDGPU_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L381
}

void amdgpuGttUsage(int const fd, struct buffer *data){
    getValue(fd, data, AMDGPU_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L387
}


#endif
