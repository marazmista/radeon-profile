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
void radeonRegistry(const int fd, struct buffer *data){(void)fd; data->error=1;}
void radeonGpuUsage(int const fd, struct buffer *data, const unsigned int time, const float frequency){(void)fd;(void)time;(void)frequency; data->error=1;}
void amdgpuVramSize(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuVramUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuGttUsage(int const fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuRegistry(const int fd, struct buffer *data){(void)fd; data->error=1;}
void amdgpuGpuUsage(int const fd, struct buffer *data, const unsigned int time, const float frequency){(void)fd;(void)time;(void)frequency; data->error=1;}


#else


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

void errorDebug(char const * const title){
#ifdef QT_NO_DEBUG_OUTPUT
    (void)title;
#else
    perror(title);
#endif
}

int openCardFD(const char * const card){
    int fd;
    char path[PATH_SIZE] = "/dev/dri/";

    strncat(path, card, PATH_SIZE);
    fd= open(path,O_RDONLY);

    if(fd < 0)
        errorDebug("open");

    return fd;
}

void closeCardFD(const int fd){
    if(close(fd))
        errorDebug("close");
}

void gpuUsage(int const fd, struct buffer *data, unsigned int const time, float const frequency, void(*accessRegistry)(int const fd, struct buffer *data)){
#define REGISTRY_ADDRESS 0x8010 // http://support.amd.com/TechDocs/46142.pdf#page=246
#define REGISTRY_MASK 1<<31
    const unsigned int sleep = time/frequency;
    unsigned int remaining, activeCount = 0, totalCount = 0;
    for(remaining = time ; remaining > 0 ; remaining -= (sleep - usleep(sleep)), totalCount++){
        // We need a struct buffer, we already have *data, we use it
        data->value.registry = REGISTRY_ADDRESS;
        accessRegistry(fd, data);

        if(data->error)
            return;

        if(REGISTRY_MASK & data->value.registry)
            activeCount++;
    }

    data->value.percentage = (100.0f * activeCount) / totalCount;

#ifndef QT_NO_DEBUG_OUTPUT
    printf("%u / %u (%3.2f%%) [%d mS, %3.1f Hz]\n", activeCount, totalCount, data->value.percentage, time/1000, frequency);
#endif
}

/********************  RADEON  ********************/
#include <drm/radeon_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993

void radeonGetValue(int fd, struct buffer *data, uint32_t command){
    struct drm_radeon_info buffer;
    buffer.request = command;
    buffer.value = (uint64_t)&(data->value);

    data->error = ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
    if(data->error)
        errorDebug("ioctl radeon");
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

void radeonReadRegistry(const int fd, struct buffer *data){
    radeonGetValue(fd, data, RADEON_INFO_READ_REG); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L576
}

void radeonGpuUsage(int const fd, struct buffer *data, unsigned int const time, float const frequency){
    gpuUsage(fd, data, time, frequency, radeonReadRegistry);
}


/********************  AMDGPU  ********************/
#include <drm/amdgpu_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441

void amdgpuVramSize(const int fd, struct buffer *data){
    struct drm_amdgpu_info_vram_gtt info;
    struct drm_amdgpu_info buffer;
    buffer.query = AMDGPU_INFO_VRAM_GTT; // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L404
    buffer.return_pointer = (uint64_t)&info;
    buffer.return_size = sizeof(info);

    data->error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    data->value.byte = info.vram_size;

    if(data->error)
        errorDebug("ioctl amdgpu");
}

void amdgpuGetValue(int fd, struct buffer *data, uint32_t command){
    struct drm_amdgpu_info buffer;
    buffer.query = command;
    buffer.return_pointer = (uint64_t)&(data->value);
    buffer.return_size = sizeof(data->value);

    data->error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    if(data->error)
        errorDebug("ioctl amdgpu");
}

void amdgpuVramUsage(int const fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L381
}

void amdgpuGttUsage(int const fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L387
}

void amdgpuReadRegistry(const int fd, struct buffer *data){
    amdgpuGetValue(fd, data, AMDGPU_INFO_READ_MMR_REG); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L416
}

void amdgpuGpuUsage(const int fd, struct buffer *data, const unsigned int time, const float frequency){
    gpuUsage(fd, data, time, frequency, amdgpuReadRegistry);
}

#endif
