Simple application to read current clocks of ATi Radeon cards (xf86-video-ati).

App need to be run with root privilages for read clocks and changing profiles. You can add `username ALL = NOPASSWD: /usr/bin/radeon-profile` to your `/etc/sudoers`
Here is tip for run app as normal user but involves change permissions to system files: http://bit.ly/1dvQMhS

Graph widget: http://www.qcustomplot.com/

Icon: http://proicons.deviantart.com/art/Graphics-Cards-Icons-H1-Pack-161178339

Sort of official thread: http://phoronix.com/forums/showthread.php?83602-radeon-profile-tool-for-changing-profiles-and-monitoring-some-GPU-parameters

# Dependencies
Crucial:
* qt4
* opensource radeon drivers

For full functionality:
* glxinfo - info about OpenGL, mesa
* xdriinfo - driver info
* xrandr - connected displays
* lm_sensors - temperature (no need if hwmon for card is present in sysfs)


# Troubleshooting


* __"no values":__ Check `/sys/kernel/debug`. If it's empty, try `# mount -t debugfs none /sys/kernel/debug`


# Bulid

Type: `qmake-qt4 && make` in source directory.


# Screenshot
[Main window](https://drive.google.com/file/d/0B7nxOyrvj2IiWVNabkRLenItajA/edit?usp=sharing)
[Graphs](https://drive.google.com/file/d/0B7nxOyrvj2IibU5vMDBGM2ZoUHc/edit?usp=sharing)
[Configuration](https://drive.google.com/file/d/0B7nxOyrvj2IiNDVMRlBuZWp0R0E/edit?usp=sharing)
