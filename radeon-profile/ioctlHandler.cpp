#include "ioctlHandler.h"

#include <QDebug>
#include <sys/ioctl.h> // ioctl()
#include <cerrno>
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

/**
 * Open file descriptor to card device.<br>
 * The kernel generates for the card with index N the files /dev/dri/card<N> and /dev/dri/renderD<128+N>.<br>
 * Executing ioctls on /dev/dri/card<N> requires either root access or being DRM Master.<br>
 * /dev/dri/renderD<128+N> does not require these permissions, but legacy kernels (Linux < 3.15) do not support it.<br>
 * This constructor tries to open both (render has the precedence).
 * @see https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
 * @see https://en.wikipedia.org/wiki/Direct_Rendering_Manager#Render_nodes
 * @see https://www.x.org/wiki/Events/XDC2013/XDC2013DavidHerrmannDRMSecurity/slides.pdf#page=14
 */
ioctlHandler::ioctlHandler(unsigned card){
#ifdef NO_IOCTL
    qWarning() << "radeon profile was compiled with NO_IOCTL, ioctls won't work";
    fd = -1;
    Q_UNUSED(card);
#else
    fd = openPath("/dev/dri/renderD", 128+card); // Try /dev/dri/renderD<128+N>
    if(fd < 0) // /dev/dri/renderD<128+N> not available
        fd = openPath("/dev/dri/card", card); // Try /dev/dri/card<N>
#endif
}


bool ioctlHandler::isValid() const {
    int temp;

    if(fd < 0) {
        qDebug() << "ioctlHandler: file descriptor not valid";
        return false;
    } else if(!getValue(&temp, sizeof(temp), 0) && (errno == EACCES)){
        qDebug() << "ioctlHandler: drm render node not available and no root access";
        return false;
    } else {
        qDebug() << "ioctlHandler: everything ok";
        return true;
    }
}


QString ioctlHandler::getDriverName() const {
    if(fd < 0)
        return "";

    char driver[NAME_SIZE] = "";

#ifdef DRM_IOCTL_VERSION
    drm_version_t v = {};
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


bool ioctlHandler::getGpuUsage(float *data, int time, int frequency) const {
#define ONE_SECOND 1000000
    const unsigned int sleep = ONE_SECOND/frequency;
    unsigned int activeCount = 0, totalCount = 0;
    for(int slept = 0; slept < time; usleep(sleep), slept+=sleep, totalCount++){
        bool active;
        if(!isCardActive(&active))
            return false; // Read failed, exit
        else if (active)
            activeCount++; // Read succeeded and the card is active, increase the counter
    }

    *data = (100.0f * activeCount) / totalCount;

    // qDebug() << activeCount << "active moments out of" << totalCount << "(" << *data << "%) in" << time/1000 << "mS (Sampling at" << frequency << "Hz)";
    return true;
}


bool ioctlHandler::getVramUsagePercentage(long *data) const {
    float total = 0;
    long usage = 0;
    bool success = getVramUsage(&usage) && getVramSize(&total);
    if(Q_LIKELY(success))
        *data = (100.0f * usage) / total;
    return success;
}

