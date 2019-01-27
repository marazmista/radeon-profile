Simple application to read current clocks of ATi Radeon cards (xf86-video-ati, xf86-video-amdgpu).

# xf86-video-ati and xf86-video-amdgpu  driver
Install and run radeon-profile-daemon (https://github.com/marazmista/radeon-profile-daemon) for using this app as normal user. Otherwise app need to be run with root privilages for changing power profiles (and clocks readings sometimes). You can add `username ALL = NOPASSWD: /usr/bin/radeon-profile` to your `/etc/sudoers`. Here is tip for run app as normal user but involves change permissions to system files: http://bit.ly/1dvQMhS

# Functionality

* Monitoring of basic GPU parameters (frequencies, voltages, usage, temperature, fan speed)
* DPM profiles and power levels
* Fan control (HD 7000+), definition of multiple custom curves or fixed speed
* Overclocking (amdgpu) (Wattman, Overdrive, PowerPlay etc)
* Event derinitions (i.e. change fan profile when temp above defined or set DPM to high when selected binary executed)
* Define binaries to run with set of envirnoment variablees (i.e. GALLIUM_HUD, MESA, LIBGL etc)

# Dependencies

* Qt 5.6<= (qt5-base and qt5-charts) (qt5-defaults for Ubuntu/Debian)
* libxrandr
* libdrm (for amdgpu, 2.4.79<=, more recent, the better)
* recent kernel (for amdgpu 4.12<=, more recent, the better)
* radeon card

For full functionality:
* glxinfo - info about OpenGL, mesa
* xdriinfo - driver info
* xrandr - connected displays


# Build

```
git clone https://github.com/marazmista/radeon-profile.git
cd radeon-profile/radeon-profile
qmake
make 
```

For Ubuntu 17.04, qt5-charts isn't available:
* Use `qtchooser -l` to list available profiles
* Use `qmake -qt=[profile from qtchooser]` to specify Qt root or download and install a Qt bundle from https://www.qt.io/download-open-source/#section-2
* Make a `qt5opt.conf` in `/usr/lib/x86_64-linux-gnu/qtchooser/` containing:

```
/opt/Qt5.9.1/5.9.1/gcc_64/bin
/opt/Qt5.9.1/5.9.1
```


# Links

* AUR package: https://aur.archlinux.org/packages/radeon-profile-git/
* System daemon: https://github.com/marazmista/radeon-profile-daemon
* System daemon AUR package: https://aur.archlinux.org/packages/radeon-profile-daemon-git/
* Sort of official thread: http://phoronix.com/forums/showthread.php?83602-radeon-profile-tool-for-changing-profiles-and-monitoring-some-GPU-parameters
* Icon: http://proicons.deviantart.com/art/Graphics-Cards-Icons-H1-Pack-161178339

# Screenshot

Main screen
![Main screen](https://i.imgur.com/Z880p47.png)

[More screenshots](http://imgur.com/a/DMRr9)
