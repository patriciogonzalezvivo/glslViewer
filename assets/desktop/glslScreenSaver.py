#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import yaml
import random
import subprocess

CONFIG_FOLDER = "/usr/share/glslViewer/" 
if not os.path.exists( CONFIG_FOLDER ):
    CONFIG_FOLDER = "/usr/local/share/glslViewer/"
CONFIG_FILE = os.path.join(CONFIG_FOLDER, "glslScreenSaver.yaml")

USER_CONFIG_FILE = os.path.join(os.path.expanduser("~"),".glslScreenSaver.yaml")
if os.path.exists( USER_CONFIG_FILE ):
    CONFIG_FILE = USER_CONFIG_FILE;

OPTION = "run"

TMP_IMAGE = "/tmp/glslScreenSaver.png"

# CMD_SCREENSHOT = "gnome-screenshot -f"
CMD_SCREENSHOT = "xwd -silent -root | convert xwd:- "

def screenshot():
    cmd = CMD_SCREENSHOT + " " + TMP_IMAGE
    subprocess.call(cmd, shell=True)

def getIdle():
    output = subprocess.Popen(["xprintidle"],stdout=subprocess.PIPE)
    response = output.communicate()
    return float(response[0]) * 0.001

if __name__ == '__main__':    
    if len(sys.argv) > 1:
        if sys.argv[1]:
            OPTION = sys.argv[1]

    with open( CONFIG_FILE ) as f:
        config = yaml.load(f, Loader=yaml.FullLoader)

        cmd = "glslViewer"
        if (OPTION == "daemon"):
            wait = int(config["wait"])
            while True:
                time.sleep(wait*0.5);
                idle = getIdle()
                if (idle > wait):
                    cmd = "glslScreenSaver run"
                    subprocess.call(cmd, shell=True)

        elif (OPTION == "dev"):
            cmd += " -l"                # always on top

        elif (OPTION == "run"):
            cmd += " -e cursor,off"     # hide cursor
            cmd += " -ss"               # turn on screensaver mode

        cmd += " " + TMP_IMAGE

        choise = config["screensaver"]

        if (isinstance(choise, list)):
            choise = random.choice(choise)

        cmd += " " + config["shaders"][choise]["fragment"]
        if "vertex" in config["shaders"][choise]:
            cmd += " " + config["shaders"][choise]["vertex"]
        if "geometry" in config["shaders"][choise]:
            cmd += " " + config["shaders"][choise]["geometry"]
        if "extra" in config["shaders"][choise]:
            cmd += " " + config["shaders"][choise]["extra"]

        print(cmd)
        screenshot()
        subprocess.call(cmd, cwd=CONFIG_FOLDER, shell=True)