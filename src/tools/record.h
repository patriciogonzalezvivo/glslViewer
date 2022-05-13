#pragma once


void    recordingStartSecs(float _start, float _end, float _fps);
void    recordingStartFrames(int _start, int _end, float _fps);

void    recordingFrameAdded();

bool    isRecording();

float   getRecordingPercentage();
int     getRecordingCount();
float   getRecordingDelta();
int     getRecordingFrame();
float   getRecordingTime();