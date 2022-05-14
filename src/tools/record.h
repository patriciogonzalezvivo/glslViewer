#pragma once

#include <string>
#include <memory>

#ifdef SUPPORT_LIBAV
struct RecordingSettings {
    std::string ffmpegPath      = "ffmpeg";
    std::string src_args        = "";
    size_t      src_width       = 512;
    size_t      src_height      = 512;
    size_t      src_channels    = 3;
    float       src_fps         = 24.0f;

    size_t      trg_width       = 512;
    size_t      trg_height      = 512;
    float       trg_fps         = 24.0f;

    std::string trg_args        = "-pix_fmt yuv420p -vsync 1 -g 1";  // -crf 0 -preset ultrafast -tune zerolatency setpts='(RTCTIME - RTCSTART) / (TB * 1000000)'
    std::string trg_path        = "output.mp4";
};

bool    recordingPipeOpen(const RecordingSettings& _settings, float _start, float _end);
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