#!/usr/bin/env python3

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

# import time
import json
import numpy as np
# from math import cos, sin
from glslviewer import GlslViewer

uniforms = {
    'u_brow_Tr': [0.0, 0.0],
    'u_brow_Rot': 0.0,
    'u_brow_tw': 0.0,
    'u_brow_uGain': [0.0, 0.0],
    'u_brow_uBias': [0.0, 0.0],
    'u_brow_vBias': [0.0, 0.0],
    'u_brow_innTr': [0.0, 0.0],
    'u_brow_midTr': [0.0, 0.0],
    'u_brow_outTr': [0.0, 0.0],
    'u_brow_shape_innTr': [0.0, 0.0],
    'u_brow_shape_midTr': [0.0, 0.0],
    'u_brow_shape_outTr': [0.0, 0.0]
}

# OPTIONS that handles most of the arguments for glslViewer
options = {
    'size': 1024,
    'headless': True,
    # 'textures': {'u_tex0': 'test.png'},
    'textures': {},
    'textures_vflipped': {},
    'uniforms': uniforms
}


def get_median_filtered(signal, threshold=3):
    """
    signal: is numpy array-like
    returns: signal, numpy array
    """
    difference = np.abs(signal - np.median(signal))
    median_difference = np.median(difference)
    s = 0 if median_difference == 0 else difference / float(median_difference)
    mask = s > threshold
    signal[mask] = np.median(signal)
    return signal


def process_results(name, file_name, results):
    log = {}

    # Prepare averages
    sample_delta = []
    sample_time = []
    # prev_val = 0.0
    for sample in results:
        sample_time.append(sample['time'])
        sample_delta.append(sample['delta'])

    log['name'] = name
    log['data'] = []
    log['mean'] = np.mean(sample_delta)
    log['median'] = np.median(sample_delta)

    sample_smoothdelta = get_median_filtered(np.array(sample_delta))

    # Get the first element
    prev_val = sample_smoothdelta[0]
    log['data'].append({
        'sec': sample_time[0],
        'val': prev_val
    })

    for i in range(1, len(sample_time) - 1):
        sec = sample_time[i]
        val = sample_smoothdelta[i]
        if prev_val != val:
            prev_val = val
            log['data'].append({
                'sec': sec,
                'val': val
            })

    # Get the last element
    log['data'].append({
        'sec': sample_time[-1],
        'val': sample_smoothdelta[-1]
    })

    return log


def benchmark(name, shader_file, options):
    shader = GlslViewer(shader_file, options)
    shader.start()
    duration = 6
    record_from = 2
    data = []
    data = shader.test(duration, record_from)
    shader.stop()
    return process_results(name, shader_file, data)


# Tests
data = []
data.append(benchmark('test', 'test.frag', options))
data.append(benchmark('test_multibuffer', 'test_multibuffer.frag', options))

options['geometry'] = 'head.ply'
data.append(benchmark('model_alone', 'model.frag', options))
data.append(benchmark('model_background', 'model_background.frag', options))
data.append(benchmark('model_background_postprocessing', 'model_background_postprocessing.frag', options))
data.append(benchmark('model_background_postprocessing_depth', 'model_background_postprocessing_depth.frag', options))
data.append(benchmark('model_postprocessing', 'model_postprocessing.frag', options))
data.append(benchmark('model_postprocessing_depth', 'model_postprocessing_depth.frag', options))

with open('index.json', 'w') as outfile:
    json.dump(data, outfile)

