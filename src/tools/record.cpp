#include "record.h"

#include <stdio.h>
#include <string.h>

#include <cstdio>
#include <atomic>
#include <thread>
#include <chrono>

#include "ada/tools/fs.h"
#include "ada/tools/text.h"

#include "lockFreeQueue.h"
#include "console.h"

#if defined( _WIN32 )
#define P_CLOSE( file ) _pclose( file )
#define P_OPEN( cmd ) _popen( cmd, "wb" )  // write binary?
#else
#define P_CLOSE( file ) pclose( file )
#define P_OPEN( cmd ) popen( cmd, "w" )
#endif

float fdelta = 0.04166666667f;
size_t counter = 0;

// PNG Sequence by secs
float sec_start = 0.0f;
float sec_head = 0.0f;
float sec_end = 0.0f;
bool  sec = false;

// PNG Sequence by frames
size_t frame_start = 0;
size_t frame_head = 0;
size_t frame_end = 0;
bool   frame = false;

#ifdef SUPPORT_LIBAV

// Video by Seconds
using Clock         = std::chrono::steady_clock;
using TimePoint     = std::chrono::time_point<Clock>;
using Seconds       = std::chrono::duration<float>;

FILE*                       pipe = nullptr;
std::atomic<bool>           pipe_isRecording;
std::thread                 pipe_thread;
RecordingSettings           pipe_settings;

TimePoint                   pipe_start;
TimePoint                   pipe_lastFrame;
LockFreeQueue               pipe_frames;

bool recordingPipe() { return (pipe != nullptr && pipe_isRecording.load()); }

// From https://github.com/tyhenry/ofxFFmpeg
bool recordingPipeOpen(const RecordingSettings& _settings, float _start, float _end) {
    if (pipe_isRecording.load()) {
        std::cout << "Can't start recording - already started." << std::endl;
        return false;
    }

    if (pipe_frames.size() > 0) {
        std::cerr << "Can't start recording - previous recording is still processing." << std::endl;
        return false;
    }

    pipe_settings = _settings;
     
    if ( pipe_settings.outputPath.empty() ) {
        std::cerr << "Can't start recording - output path is not set!" << std::endl;
        return false;
    }

    if (ada::urlExists(pipe_settings.outputPath)) {
        int file_copy = 0;
        std::string extension = ada::getExt(pipe_settings.outputPath); 
        std::string basename = pipe_settings.outputPath.substr(0,pipe_settings.outputPath.size() - extension.size() - 1 );
        while ( ada::urlExists(pipe_settings.outputPath) ) {
            pipe_settings.outputPath = basename + "_" + ada::toString(file_copy, 0, 3, '0') + '.' + extension;
            // return false;
            file_copy++;
        }

        std::cerr << "That file already exists, Data will be save to: " << pipe_settings.outputPath << std::endl;
    }

    if ( pipe_settings.ffmpegPath.empty() )
        pipe_settings.ffmpegPath = "ffmpeg";

    fdelta = 1.0/pipe_settings.fps;
    counter = 0;

    sec_start = _start;
    sec_head = _start;
    sec_end = _end;

    std::string cmd = pipe_settings.ffmpegPath;
    std::vector<std::string> args = {
        "-y",   // overwrite

        "-an",  // disable audio -- todo: add audio,
        #ifdef SUPPORT_NCURSES
        "-loglevel quiet",                                      // no log output 
        // "-stats",                                            // only stats
        #endif

        // input
        "-r " + ada::toString( pipe_settings.fps ),             // input frame rate
        "-s " + std::to_string( pipe_settings.width ) +         // input resolution width
            "x" + std::to_string( pipe_settings.height ),       // input resolution height
        "-f rawvideo",                                          // input codec
        "-pix_fmt rgb24",                                       // input pixel format
        pipe_settings.inputArgs,                                // custom input args
        "-i pipe:",                                             // input source (default pipe)

        // output
        // "-r " + ada::toString( pipe_settings.fps ),             // output frame rate
        // "-c:v libx264" + pipe_settings.videoCodec,              // output codec
        // "-b:v " + ada::toString( pipe_settings.bitrate ) + "k", // output bitrate kbps (hint)
        // "-vf vflip",                                            // flip image
        pipe_settings.outputArgs,                               // custom output args
        pipe_settings.outputPath                                // output path
    };

    for ( size_t i = 0; i < args.size(); i++)
        if ( !args[i].empty() ) 
            cmd += " " + args[i];

    if ( pipe != nullptr )
        P_CLOSE( pipe );

    pipe = P_OPEN( cmd.c_str() );

    if ( !pipe ) {
        // // get error string from 'errno' code
        // char errmsg[500];
        // std::strerror_s( errmsg, 500, errno );
        // std::cerr << "Unable to start recording. Error: " << errmsg << std::endl;

        std::cerr << "Unable to start recording." << std::endl;
        return false;
    }

    return pipe_isRecording = true;
}

void processFrame() {
    while ( pipe_isRecording.load() ) {

        TimePoint lastFrameTime = Clock::now();
        const float framedur    = 1.f / pipe_settings.fps;

        while ( pipe_frames.size() ) {  // allows finish processing queue after we call stop()

            // feed frames at constant fps
            float delta = Seconds( Clock::now() - lastFrameTime ).count();

            if ( delta >= framedur ) {

                if ( !pipe_isRecording.load() )
                    std::cout << "Don't close. Recording stopped, but still processing " << pipe_frames.size() << " frames" << std::endl;

                Pixels pixels;
                if ( pipe_frames.consume( std::move( pixels ) ) && pixels ) {
                    std::unique_ptr<unsigned char[]> data = std::move( pixels );
                    const size_t dataLength = pipe_settings.width * pipe_settings.height * pipe_settings.channels;
                    const size_t written = pipe ? fwrite( data.get(), sizeof( char ), dataLength, pipe ) : 0;

                    if ( written <= 0 )
                        std::cout << "Unable to write the frame." << std::endl;

                    lastFrameTime = Clock::now();
                }

                console_refresh();
            }
        }
    }

    // close ffmpeg pipe once stopped recording

    if ( pipe ) {
        console_clear();

        if ( P_CLOSE( pipe ) < 0 ) {
            // // get error string from 'errno' code
            // char errmsg[500];
            // strerror_s( errmsg, 500, errno );
            // std::cerr << "Error closing FFmpeg pipe. Error: " << errmsg << std::endl;
            std::cerr << "Error closing FFmpeg pipe." << std::endl;
        }
    }

    pipe   = nullptr;
    counter = 0;
}

size_t recordingPipeFrame( std::unique_ptr<unsigned char[]>&& _pixels ) {
    if ( !pipe_isRecording ) {
        std::cerr << "Can't add new frame - not in recording mode." << std::endl;
        return 0;
    }

    if ( !pipe ) {
        std::cerr << "Can't add new frame - FFmpeg pipe is invalid!" << std::endl;
        return 0;
    }

    if ( counter == 0 ) {
        if ( pipe_thread.joinable() ) pipe_thread.join();  //detach();
        pipe_thread     = std::thread( &processFrame );
        pipe_start      = Clock::now();
        pipe_lastFrame  = pipe_start;
    }

    pipe_frames.produce( std::move(_pixels) );
    pipe_lastFrame = Clock::now();

    size_t written              = 0;

    // // add new frame(s) at specified frame rate
    // float recordedDuration      = counter / pipe_settings.fps;
    // const float delta           = Seconds( Clock::now() - pipe_start ).count() - recordedDuration;
    // const size_t framesToWrite  = delta * pipe_settings.fps;
    // Pixels pixels;
    // // drop or duplicate frames to maintain constant framerate
    // while ( counter == 0 || framesToWrite > written ) {
    //     // copy pixel data
    //     if ( !pixPtr )
    //         pixPtr = new ofPixels( pixels );  

    //     if ( written == framesToWrite - 1 )
    //         // only the last frame we produce owns the pixel data
    //         m_frames.produce( pixPtr );
    //     else {
    //         // otherwise, we reference the data
    //         ofPixels *pixRef = new ofPixels();
    //         pixRef->setFromExternalPixels( pixPtr->getData(), pixPtr->getWidth(), pixPtr->getHeight(), pixPtr->getPixelFormat() );  // re-use already copied pointer
    //         m_frames.produce( pixRef );
    //     }
    //     ++counter;
    //     ++written;
    //     pipe_lastFrame = Clock::now();
    // }

    return written;
}

void recordingPipeClose() {
    frame = false;
    sec = false;

    if ( pipe_thread.joinable() ) 
        pipe_thread.join();

    if ( pipe != nullptr )
        P_CLOSE( pipe );
}

#else

bool    recordingPipe() { return false};
#endif

// ---------------------------------------------------------------------------

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
    else if (recordingPipe()) {
        sec_head += fdelta;
        if (sec_head >= sec_end)
            pipe_isRecording = false;
    }
    else if (frame) {
        frame_head++;
        if (frame_head >= frame_end)
            frame = false;
    }
}

bool isRecording() { return sec || frame || recordingPipe(); }

int getRecordingCount() { return counter; }
float getRecordingDelta() { return fdelta; }

float getRecordingPercentage() {
    if (sec || recordingPipe() )
        return ((sec_head - sec_start) / (sec_end - sec_start));
    else if (frame)
        return ( (float)(frame_head - frame_start) / (float)(frame_end - frame_start));
    else 
        return 1.0;
}

int getRecordingFrame() {
    if (sec || recordingPipe() ) 
        return (int)(sec_start / fdelta) + counter;
    else
        return frame_head;
    
    return 0;
}

float getRecordingTime() {
    if (sec || recordingPipe() )
        return sec_head;
    else
        return frame_head * fdelta;
}
