#!/bin/bash

 in_file="$1"
    size="$2"
out_file="$3"

/usr/bin/glslViewer "$in_file" --noncurses --headless --msaa -s "$size" -e floor,off -E screenshot,"$out_file"