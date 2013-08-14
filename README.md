Simple application to read current clocks of ATi Radeon cards (xf86-video-ati)
Install lm_sensors for gpu temperature.

App need to be run with root privilages for read clocks and changing profiles. You can add `username ALL = NOPASSWD: /usr/bin/radeon-profile` to your `/etc/sudoers`

Graph widget: http://www.qcustomplot.com/

# Troubleshooting


* __"Err" instead of values:__ Try start app as root. Need to read system files.
	
* __"no values":__ Check `/sys/kernel/debug`. If it's empty, try `# mount -t debugfs none /sys/kernel/debug`


# Screenshot
[Main window](https://docs.google.com/file/d/0B7nxOyrvj2IiZ2otNWxKMVJmakU/edit?usp=sharing)
[Graphs](https://docs.google.com/file/d/0B7nxOyrvj2IiTmdGQ3AyUS1hQ28/edit?usp=sharing)
