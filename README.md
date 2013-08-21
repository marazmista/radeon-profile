Simple application to read current clocks of ATi Radeon cards (xf86-video-ati)
Install lm_sensors for gpu temperature.

App need to be run with root privilages for read clocks and changing profiles. You can add `username ALL = NOPASSWD: /usr/bin/radeon-profile` to your `/etc/sudoers`

Graph widget: http://www.qcustomplot.com/

Icon: http://proicons.deviantart.com/art/Graphics-Cards-Icons-H1-Pack-161178339


# Troubleshooting


* __"Err" instead of values:__ Try start app as root. Need to read system files.
	
* __"no values":__ Check `/sys/kernel/debug`. If it's empty, try `# mount -t debugfs none /sys/kernel/debug`


# Bulid

Type: `qmake-qt4 && make`


# Screenshot
[Main window](https://docs.google.com/file/d/0B7nxOyrvj2IiSWlTeE5GdGdDejQ/edit?usp=sharing)
[Graphs](https://docs.google.com/file/d/0B7nxOyrvj2IiZ0JfdGFRM3RPUjA/edit?usp=sharing)
