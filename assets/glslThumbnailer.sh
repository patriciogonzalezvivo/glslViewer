#!/bin/bash

 in_file="$1"
    size="$2"
out_file="$3"

Xvfb :100 -ac &
PID1=$!
export DISPLAY=:100
glslViewer "$in_file" --noncurses --headless --msaa -s "$size" -e floor,off -E screenshot,"$out_file" >> /tmp/glslViewer-error.log 2>> /tmp/glslViewer-error.txt

rm -f /tmp/.X100-lock
rm -rf /tmp/.X11-unix

kill -9 $PID1