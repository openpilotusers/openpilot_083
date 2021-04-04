#!/usr/bin/bash

export LD_LIBRARY_PATH=/data/data/com.termux/files/usr/lib
export HOME=/data/data/com.termux/files/home
export PATH=/usr/local/bin:/data/data/com.termux/files/usr/bin:/data/data/com.termux/files/usr/sbin:/data/data/com.termux/files/usr/bin/applets:/bin:/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin:/data/data/com.termux/files/usr/bin/python
export PYTHONPATH=/data/openpilot

cd /data/openpilot
RT_DELTA=`cat /data/params/d/RTDelta`
sed -i "2s/.*/const int HYUNDAI_MAX_RT_DELTA \= ${RT_DELTA}\;          \/\/ max delta torque allowed for real time checks/g" /data/openpilot/panda/board/safety/safety_hyundai.h
reboot