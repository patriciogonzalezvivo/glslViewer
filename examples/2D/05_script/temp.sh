#!/bin/bash

while true; do
    awk '{print "u_temp,"$1/1000}' /sys/class/thermal/thermal_zone0/temp
    sleep 1
done
