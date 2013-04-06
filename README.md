Simple application to read current clocks of ATi Radeon cards (xf86-video-ati)


# Troubleshooting

* "Err" instead of values:
	Try start app as root. Need to read system files.
	
* "no values":
	Check `/sys/kernel/debug`. If it's empty, try `mount -t debugfs none /sys/kernel/debug`
