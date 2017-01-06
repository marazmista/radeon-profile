#!/bin/sh

uevents='/sys/class/drm/*/device/uevent'

for device in $(ls $uevents) ; do
    id=$(grep 'PCI_ID' "$device")
    id=${id#'PCI_ID='}
    if [ "$id" ] ; then
	driver=$(grep 'DRIVER' "$device")
	driver=${driver#'DRIVER='}
	if [ "$driver" == "radeon" -o "$driver" == "amdgpu" ] ; then
	    card=${device#'/sys/class/drm/'}
	    card=${card%'/device/uevent'}
	    slot=$(grep 'PCI_SLOT_NAME' "$device")
	    slot=${slot#'PCI_SLOT_NAME='}
	    printf "$id \t $slot \t $driver \t $card\n"
	fi
    fi
done
