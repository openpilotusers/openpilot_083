#!/usr/bin/bash

ALIAS_CHECK=$(/usr/bin/grep gitpull /system/comma/home/.bash_profile)

if [ "$ALIAS_CHECK" == "" ]; then
    sleep 3
    mount -o remount,rw /system
    echo "alias gi='/data/openpilot/gitpull.sh'" >> /system/comma/home/.bash_profile
    mount -o remount,r /system
fi

if [ ! -f "/data/KRSet" ]; then
    setprop persist.sys.country KR
    setprop persist.sys.timezone Asia/Seoul
    /usr/bin/touch /data/KRSet
fi

export PASSIVE="0"
exec ./launch_chffrplus.sh

