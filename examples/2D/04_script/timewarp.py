#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from glslviewer import GlslViewer

import sys, os
import subprocess
import time
import cv2
import numpy as np

WIDTH = 0
HEIGHT = 0
shader = None

def getFrames(folder):
    files = [] #list of image filenames
    dirFiles = os.listdir(folder) #list of directory files
    dirFiles.sort() #good initial sort but doesnt sort numerically very well
    sorted(dirFiles) #sort numerically in ascending order

    for filename in dirFiles: #filter out all non jpgs
        if '.png' in filename:
            files.append(filename)
    
    return files

def normalize( A, div=None ):
    if not div:
        div = max(np.min(A), np.max(A))
        print('Div', div)
        
    B = A / div
    B = B * 0.5
    B = B + 0.5
    return np.clip(B, 0.0, 1.0)

def opticalFlow( img1, img2, frame, flow=None, levels=5, iterations=5, winsize=11):
    flow = cv2.calcOpticalFlowFarneback(    img1, img2, flow=flow,
                                            pyr_scale=0.5, levels=levels, winsize=winsize,
                                            iterations=iterations,
                                            poly_n=5, poly_sigma=1.1, flags=0 )

    w,h = img1.shape[:2]
    rgb = np.zeros((w, h, 3))

    div_x = max(abs(np.min(flow[..., 0])), np.max(flow[..., 0]))
    div_y = max(abs(np.min(flow[..., 1])), np.max(flow[..., 1]))
    div = max(div_x, div_y)

    print("Norm divider: ", div)
    frame['div'] = div

    rgb[:,:,0] = normalize(flow[..., 0], div)
    rgb[:,:,1] = normalize(flow[..., 1], div)
    rgb[:,:,2] = img1[:,:]/255.0

    return rgb

def saveImage(data, fn, scale=1.0, cmap='Greys'):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    sizes = np.shape(data)
    height = float(sizes[0])
    width = float(sizes[1])
    
    fig = plt.figure()
    fig.set_size_inches(width/height, 1, forward=False)

    ax = plt.Axes(fig, [0., 0., 1., 1.])
    ax.set_axis_off()
    fig.add_axes(ax)

    if len(list(data.shape)) > 2:
        ax.imshow(data)
    else:
        ax.imshow(data, cmap=cmap)
    plt.savefig(fn, dpi=height*scale) 
    plt.close()

def interpolate(A_flow, A_div, A, 
                B_flow, B_div, B, 
                sec, target_folder, start_index, width, height):
    
    cmd = f"offload-glx glslViewer timewarp.frag --headless {A_flow} {A} {B_flow} {B} -w {width} -h {height} -e u_A,{A_div} -e u_B,{B_div} -E sequence,0,{sec}"
    print(cmd)
    subprocess.call(cmd, shell=True) 

    images = getFrames('./')
    for image in images:
        cmd = "mv " + image + " " + target_folder + '/{0:05d}'.format(start_index) + '.png'
        subprocess.call(cmd, shell=True) 
        start_index += 1

    return start_index

if __name__ == '__main__':
    FOLDER_FRAMES = "frames"
    
    if len(sys.argv) > 1:
        if sys.argv[1]:
            FOLDER_FRAMES = sys.argv[1]

    FOLDER_FLOW_FRAMES = FOLDER_FRAMES + "_flow"
    FOLDER_SUB_FRAMES = FOLDER_FRAMES + "_final"

    if not os.path.exists(FOLDER_FLOW_FRAMES):
        os.mkdir(FOLDER_FLOW_FRAMES)

    if not os.path.exists(FOLDER_SUB_FRAMES):
        os.mkdir(FOLDER_SUB_FRAMES)

    # GET FRAMES, filter out link files
    images = getFrames(FOLDER_FRAMES)
    frames = []
    for filename in images: 
        if filename == 'current.png' or filename == 'prev.png':
            continue

        frames.append( {
                            'id': os.path.splitext(filename)[0],
                            'rgb': FOLDER_FRAMES+'/'+filename,
                            'flow': FOLDER_FLOW_FRAMES+'/'+filename,
                            'div': 1.0
                        } )

    test = cv2.imread(frames[0]['rgb'], 0)
    WIDTH = test.shape[1]
    HEIGHT = test.shape[0]
    print(WIDTH, HEIGHT)

    # Make optical flow images
    for index in range(len(frames)-1):
        A = cv2.imread(frames[index]['rgb'])
        B = cv2.imread(frames[index+1]['rgb'])

        A = cv2.cvtColor(A, cv2.COLOR_BGR2GRAY)
        B = cv2.cvtColor(B, cv2.COLOR_BGR2GRAY)

        # A = cv2.pyrDown(A)
        # B = cv2.pyrDown(B)

        O = opticalFlow(A, B, frames[index])
        saveImage(O, frames[index]['flow'])
    
    # Interpolate pairs of frames using velocities from optical flow
    frame = 0
    index = 0
    shader = None
    for A, B in zip(frames[:-1], frames[1:]):
        if frame == len(frames)-2:
            break
        print('Frame:', str(frame) + '/' + str(len(frames)))
        index = interpolate(A['flow'], A['div'], A['rgb'],  
                            B['flow'], B['div'], B['rgb'], 
                            1, FOLDER_SUB_FRAMES, index, WIDTH, HEIGHT)
        frame += 1
