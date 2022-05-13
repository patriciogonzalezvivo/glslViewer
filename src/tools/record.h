#pragma once

#include <string>
#include <memory>

#ifdef SUPPORT_LIBAV
struct RecordingSettings {
    std::string outputPath      = "output.mp4";
    std::string ffmpegPath      = "ffmpeg";
    std::string videoCodec      = "libx264";
    std::string extraInputArgs  = "";
    std::string extraOutputArgs = "-pix_fmt yuv420p -vsync 1 -g 1";  // -crf 0 -preset ultrafast -tune zerolatency setpts='(RTCTIME - RTCSTART) / (TB * 1000000)'
    size_t width                = 512;
    size_t height               = 512;
    size_t channels             = 3;
    float fps                   = 24.0f;
    unsigned int bitrate        = 20000;  // kbps
    bool allowOverwrite         = true;
};

bool    recordingPipeOpen(const RecordingSettings& _settings, float _start, float _end, float _fps);
size_t  recordingPipeFrame( std::unique_ptr<unsigned char[]>&& _pixels );
void    recordingPipeClose();
#endif
bool    recordingPipe();

void    recordingStartSecs(float _start, float _end, float _fps);
void    recordingStartFrames(int _start, int _end, float _fps);

void    recordingFrameAdded();

bool    isRecording();

float   getRecordingPercentage();
int     getRecordingCount();
float   getRecordingDelta();
int     getRecordingFrame();
float   getRecordingTime();