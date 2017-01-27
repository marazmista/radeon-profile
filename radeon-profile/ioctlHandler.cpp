#include "ioctlHandler.hpp"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cstring> // memset()
#include <cstdio> // snprintf()
#include <unistd.h> // close(), usleep()
#include <fcntl.h> // open()

#ifndef NO_IOCTL // Include libdrm headers only if NO_IOCTL is not defined
#  include <libdrm/drm.h>
#endif


#define PATH_SIZE 20
#define NAME_SIZE 10


int ioctlHandler::openPath(const char *prefix, unsigned index) const {
    char path[PATH_SIZE];
    snprintf(path, PATH_SIZE, "%s%u", prefix, index);

    int res = open(path, O_RDONLY);
    if(res < 0) // Open failed
        perror(path);
    else
        qDebug() << "Opened" << path << ", fd number:" << res;

    return res;
}

ioctlHandler::ioctlHandler(unsigned card){
#ifdef NO_IOCTL
    qWarning() << "radeon profile was compiled with NO_IOCTL, ioctls won't work";
    fd = -1;
    Q_UNUSED(card);
#else
    /* Open file descriptor to card device
     * Information ioctls can be accessed in two ways:
     * The kernel generates for the card with index N the files /dev/dri/card<N> and /dev/dri/renderD<128+N>
     * Both can be opened without root access
     * Executing ioctls on /dev/dri/card<N> requires either root access or being DRM Master
     * /dev/dri/renderD<128+N> does not require these permissions, but legacy kernels (Linux < 3.15) do not support it
     * For more details:
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
     * https://en.wikipedia.org/wiki/Direct_Rendering_Manager#Render_nodes
     * https://www.x.org/wiki/Events/XDC2013/XDC2013DavidHerrmannDRMSecurity/slides.pdf#page=15
     */
    fd = openPath("/dev/dri/renderD", 128+card); // Try /dev/dri/renderD<128+N>
    if(fd < 0) // /dev/dri/renderD<128+N> not available
        fd = openPath("/dev/dri/card", card); // Try /dev/dri/card<N>
#endif
}


QString ioctlHandler::getDriverName() const {
    if(fd < 0)
        return "";

    char driver[NAME_SIZE] = "";

#ifdef DRM_IOCTL_VERSION
    drm_version_t v;
    memset(&v, 0, sizeof(v));
    v.name = driver;
    v.name_len = NAME_SIZE;
    if(ioctl(fd, DRM_IOCTL_VERSION, &v)){
        perror("DRM_IOCTL_VERSION");
        return "";
    }
#endif

    return QString(driver);
}


ioctlHandler::~ioctlHandler(){
    qDebug() << "Closing ioctl fd, number:" << fd;
    if((fd >= 0) && close(fd))
        perror("fd close");
}


bool ioctlHandler::getGpuUsage(float *data, unsigned time, unsigned frequency) const {
    /* Sample the GPU status register N times and check how many of these samples have the GPU busy
     * Register documentation:
     * http://support.amd.com/TechDocs/46142.pdf#page=246   (address 0x8010, 31st bit)
     */
#define REGISTRY_ADDRESS 0x8010
#define REGISTRY_MASK 1<<31
#define ONE_SECOND 1000000
    const unsigned int sleep = ONE_SECOND/frequency;
    unsigned int remaining, activeCount = 0, totalCount = 0, buffer;
    for(remaining = time; (remaining > 0) && (remaining <= time); // Cycle until the time has finished
        remaining -= (sleep - usleep(sleep)), totalCount++){ // On each cycle sleep and subtract the slept time from the remaining
        buffer = REGISTRY_ADDRESS;
        bool success = readRegistry(&buffer);

        if(Q_UNLIKELY(!success)) // Read failed
            return false;

        if(REGISTRY_MASK & buffer) // Check if the activity bit is 1
            activeCount++;
    }

    if(Q_UNLIKELY(totalCount == 0))
        return false;

    *data = (100.0f * activeCount) / totalCount;

    // qDebug() << activeCount << "active moments out of" << totalCount << "(" << *data << "%) in" << time/1000 << "mS (Sampling at" << frequency << "Hz)";
    return true;
}



