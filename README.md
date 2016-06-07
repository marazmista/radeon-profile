Simple application to read current clocks and temperatures, overclock and manage power settings of your graphic card.

# Supported drivers
* ## Radeon ( xf86-video-ati / xf86-video-amdgpu )
  The app reads clocks and power profiles from debugfs, which requires root access. Possible solutions (only one is needed):
  * Install and run radeon-profile-daemon (https://github.com/marazmista/radeon-profile-daemon).
  * Run the app with root privileges.
  * Add `username ALL = NOPASSWD: /usr/bin/radeon-profile` to your `/etc/sudoers`.
  * Change debugfs permissions (unsafe) http://bit.ly/1dvQMhS

  Can be forced by `--driver xorg` parameter.

* ## AMD Catalyst ( fglrx )
  Normal user can run it. Data is read from `ati-config` output. No support for power settings.

  Can be forced by `--driver fglrx` parameter.

* ## Intel ( xf86-video-intel )
  Normal user can run it. No support for power settings.

  Can be forced by `--driver intel` parameter.

# Dependencies
Crucial:
* Qt4 or Qt5  (see build below)
* libxrandr
* radeon card

For full functionality:
* glxinfo - info about OpenGL, mesa
* xdriinfo - driver info
* xrandr - connected displays
* lm_sensors - temperature (no need if hwmon for card is present in sysfs and if catalyst is installed)

# Links

* AUR package: https://aur.archlinux.org/packages/radeon-profile-git/
* System daemon: https://github.com/marazmista/radeon-profile-daemon
* System daemon AUR package: https://aur.archlinux.org/packages/radeon-profile-daemon-git/
* Sort of official thread: http://phoronix.com/forums/showthread.php?83602-radeon-profile-tool-for-changing-profiles-and-monitoring-some-GPU-parameters
* Graph widget: http://www.qcustomplot.com/
* Icon: http://proicons.deviantart.com/art/Graphics-Cards-Icons-H1-Pack-161178339

# Bulid
Qt4: `qmake-qt4 && make` in source directory.

Qt5: `qmake && make` in source directory.

# Troubleshooting
* __"no values":__ Check `/sys/kernel/debug`. If it's empty, try `# mount -t debugfs none /sys/kernel/debug`

# Screenshot
[Gallery](https://imgur.com/a/vWEIl)
