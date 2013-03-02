#ifndef RADEON_PROFILE_H
#define RADEON_PROFILE_H

#include <QMainWindow>
#include <QString>

namespace Ui {
class radeon_profile;
}

class radeon_profile : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit radeon_profile(QWidget *parent = 0);
    ~radeon_profile();

    const QString profilePath = "/sys/class/drm/card0/device/power_profile";
    const QString clocksPath = "/sys/kernel/debug/dri/0/radeon_pm_info";

    const QString err = "Err";
    const QString noValues = "no values";

//    enum clockData { defGPU = 0, currGPU = 1, defMEM = 3, currMEM = 3, currVolts = 4 };
    
private slots:
    void on_chProfile_clicked();
    void timerEvent();

private:
    Ui::radeon_profile *ui;
};

#endif // RADEON_PROFILE_H
