#include "radeon_ioctl.h"


#ifdef NO_IOCTL


int openCardFD(const char * const card){(void)card; return -1;}
void closeCardFD(const int fd){(void)fd;}
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>

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

    return fd;
}

void closeCardFD(const int fd){
    if(close(fd))
        perror("close");
}

void radeonGetValue(int fd, struct buffer *data, uint32_t command){
    struct drm_radeon_info buffer;
    buffer.request = command;
    buffer.value = (uint64_t)&(data->value);
    buffer.pad = sizeof(data->value);

    data->error = ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
    if(data->error)
        perror("ioctl radeon");
}

void amdgpuGetValue(int fd, struct buffer *data, uint32_t command){
    struct drm_amdgpu_info buffer;
    buffer.query = command;

    switch(command){
    case AMDGPU_INFO_VRAM_GTT: {
        struct drm_amdgpu_info_vram_gtt info;
        buffer.return_pointer = (uint64_t)&info;
        buffer.return_size = sizeof(info);

        data->error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
        data->value.byte = info.vram_size;
        break;
    }

    default:
        buffer.return_pointer = (uint64_t)&(data->value);
        buffer.return_size = sizeof(data->value);

        data->error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
        break;
    }

    if(data->error)
        perror("ioctl amdgpu");
}

void radeonTemperature(int const fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_TEMP); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L555
}

void radeonCoreClock(int const fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L562
}

void radeonMaxCoreClock(const int fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_MAX_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L511
}

void radeonMemoryClock(int const fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_MCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L569
}

void radeonVramUsage(int const fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L529
}

void radeonGttUsage(int const fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L534
}

void amdgpuVramSize(const int fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_VRAM_GTT); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L404
}

void amdgpuVramUsage(int const fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L381
}

void amdgpuGttUsage(int const fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L387
}


#endif
