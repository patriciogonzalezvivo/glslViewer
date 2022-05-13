#include "record.h"

#include <cstdio>

float fdelta = 0.04166666667f;
size_t counter = 0;

float sec_start = 0.0f;
float sec_head = 0.0f;
float sec_end = 0.0f;
bool  sec = false;

size_t frame_start = 0;
size_t frame_head = 0;
size_t frame_end = 0;
bool   frame = false;

void recordingStartSecs(float _start, float _end, float _fps) {
    fdelta = 1.0/_fps;
    counter = 0;

    sec_start = _start;
    sec_head = _start;
    sec_end = _end;
    sec = true;
}

void recordingStartFrames(int _start, int _end, float _fps) {
    fdelta = 1.0/_fps;
    counter = 0;

    frame_start = _start;
    frame_head = _start;
    frame_end = _end;
    frame = true;
}

void recordingFrameAdded() {
    counter++;

    if (sec) {
        sec_head += fdelta;
        if (sec_head >= sec_end)
            sec = false;
    }
    else if (frame) {
        frame_head++;
        if (frame_head >= frame_end)
            frame = false;
    }
}

bool isRecording() { return sec || frame; }

int getRecordingCount() { return counter; }
float getRecordingDelta() { return fdelta; }

float getRecordingPercentage() {
    if (sec)
        return ((sec_head - sec_start) / (sec_end - sec_start));
    else if (frame)
        return ( (float)(frame_head - frame_start) / (float)(frame_end - frame_start));
    else 
        return 1.0;
}

int getRecordingFrame() {
    if (sec) 
        return (int)(sec_start / fdelta) + counter;
    else if (frame) 
        return frame_head;
}

float getRecordingTime() {
    if (sec)
        return sec_head;
    else if (frame)
        return frame_head * fdelta;
}
