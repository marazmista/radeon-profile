#include "radeon_ioctl.h"


#ifdef NO_IOCTL


int openCardFD(const char * card){(void)card; return -1;}
void closeCardFD(int fd){(void)fd;}
int radeonTemperature(int fd, int *data){(void)fd;(void)data; return -1;}
int radeonCoreClock(int fd, unsigned *data){(void)fd;(void)data; return -1;}
int radeonMemoryClock(int fd, unsigned *data){(void)fd;(void)data; return -1;}
int radeonMaxCoreClock(int fd, unsigned *data){(void)fd;(void)data; return -1;}
int radeonVramUsage(int fd, unsigned long *data){(void)fd;(void)data; return -1;}
int radeonGttUsage(int fd, unsigned long *data){(void)fd;(void)data; return -1;}
int radeonReadRegistry(int fd, unsigned *data){(void)fd;(void)data; return -1;}
int radeonGpuUsage(int fd, float *data, unsigned time, unsigned frequency){(void)fd;(void)data;(void)time;(void)frequency; return -1;}
int amdgpuVramSize(int fd, unsigned long *data){(void)fd;(void)data; return -1;}
int amdgpuVramUsage(int fd, unsigned long *data){(void)fd;(void)data; return -1;}
int amdgpuGttUsage(int fd, unsigned long *data){(void)fd;(void)data; return -1;}
int amdgpuReadRegistry(int fd, unsigned *data){(void)fd;(void)data; return -1;}
int amdgpuGpuUsage(int fd, float *data, unsigned time, unsigned frequency){(void)fd;(void)data;(void)time;(void)frequency; return -1;}


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

void errorDebug(const char * title){
#ifdef QT_NO_DEBUG_OUTPUT
    (void)title;
#else
    perror(title);
#endif
}

int openCardFD(const char * card){
    int fd;
    char path[PATH_SIZE] = "/dev/dri/";

    strncat(path, card, PATH_SIZE-strlen(path)-1);
    fd= open(path,O_RDONLY);

    if(fd < 0)
        errorDebug("open");

    return fd;
}

void closeCardFD(int fd){
    if(close(fd))
        errorDebug("close");
}

int gpuUsage(int fd, float *data, unsigned time, unsigned frequency, int(*accessRegistry)(int fd, unsigned *data)){
#define REGISTRY_ADDRESS 0x8010 // http://support.amd.com/TechDocs/46142.pdf#page=246
#define REGISTRY_MASK 1<<31
#define ONE_SECOND 1000000
    const unsigned int sleep = ONE_SECOND/frequency;
    unsigned int remaining, activeCount = 0, totalCount = 0, buffer;
    int error;
    for(remaining = time; (remaining > 0) && (remaining <= time); remaining -= (sleep - usleep(sleep)), totalCount++){
        buffer = REGISTRY_ADDRESS;
        error = accessRegistry(fd, &buffer);

        if(error)
            return error;

        if(REGISTRY_MASK & buffer)
            activeCount++;
    }

    if(totalCount == 0)
        return -1;

    *data = (100.0f * activeCount) / totalCount;

#ifndef QT_NO_DEBUG_OUTPUT
    printf("%u active moments out of %u (%3.2f%%) in %umS (Sampling at %uHz)\n", activeCount, totalCount, *data, time/1000, frequency);
#endif

    return 0;
}

/********************  RADEON  ********************/
#include <drm/radeon_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/radeon_drm.h#L993

int radeonGetValue(int fd, void *data, uint32_t command){
    int error;
    struct drm_radeon_info buffer;
    CLEAN(buffer);
    buffer.request = command;
    buffer.value = (uint64_t)data;

    error = ioctl(fd, DRM_IOCTL_RADEON_INFO, &buffer);
    if(error)
        errorDebug("ioctl radeon");

    return error;
}

int radeonTemperature(int fd, int *data){
    return radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_TEMP); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L555
}

int radeonCoreClock(int fd, unsigned *data){
    return radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L562
}

int radeonMaxCoreClock(int fd, unsigned *data){
    return radeonGetValue(fd, data, RADEON_INFO_MAX_SCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L511
}

int radeonMemoryClock(int fd, unsigned *data){
    return radeonGetValue(fd, data, RADEON_INFO_CURRENT_GPU_MCLK); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L569
}

int radeonVramUsage(int fd, unsigned long *data){
    return radeonGetValue(fd, data, RADEON_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L529
}

int radeonGttUsage(int fd, unsigned long *data){
    return radeonGetValue(fd, data, RADEON_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L534
}

int radeonReadRegistry(int fd, unsigned *data){
    return radeonGetValue(fd, data, RADEON_INFO_READ_REG); // http://lxr.free-electrons.com/source/drivers/gpu/drm/radeon/radeon_kms.c#L576
}

int radeonGpuUsage(int fd, float *data, unsigned time, unsigned frequency){
    return gpuUsage(fd, data, time, frequency, radeonReadRegistry);
}


/********************  AMDGPU  ********************/
#include <drm/amdgpu_drm.h> // http://lxr.free-electrons.com/source/include/uapi/drm/amdgpu_drm.h#L441

int amdgpuVramSize(int fd, unsigned long *data){
    int error;
    struct drm_amdgpu_info_vram_gtt info;
    struct drm_amdgpu_info buffer;
    CLEAN(buffer);
    buffer.query = AMDGPU_INFO_VRAM_GTT; // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L404
    buffer.return_pointer = (uint64_t)&info;
    buffer.return_size = sizeof(info);

    error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    *data = info.vram_size;

    if(error)
        errorDebug("ioctl amdgpu");

    return error;
}

int amdgpuGetValue(int fd, void *data, uint32_t command){
    int error;
    struct drm_amdgpu_info buffer;
    CLEAN(buffer);
    buffer.query = command;
    buffer.return_pointer = (uint64_t)data;
    buffer.return_size = sizeof(unsigned long);

    error = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &buffer);
    if(error)
        errorDebug("ioctl amdgpu");

    return error;
}

int amdgpuVramUsage(int fd, unsigned long *data){
    return amdgpuGetValue(fd, data, AMDGPU_INFO_VRAM_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L381
}

int amdgpuGttUsage(int fd, unsigned long *data){
    return amdgpuGetValue(fd, data, AMDGPU_INFO_GTT_USAGE); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L387
}

int amdgpuReadRegistry(int fd, unsigned *data){
    return amdgpuGetValue(fd, data, AMDGPU_INFO_READ_MMR_REG); // http://lxr.free-electrons.com/source/drivers/gpu/drm/amd/amdgpu/amdgpu_kms.c#L416
}

int amdgpuGpuUsage(int fd, float *data, unsigned time, unsigned frequency){
    return gpuUsage(fd, data, time, frequency, amdgpuReadRegistry);
}

#endif
