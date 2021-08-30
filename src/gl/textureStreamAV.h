
#pragma once

#include "textureStream.h"

// #define SUPPORT_FOR_LIBAV
#ifdef SUPPORT_FOR_LIBAV

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

class TextureStreamAV : public TextureStream {
public:
    TextureStreamAV();
    virtual ~TextureStreamAV();

    virtual float   getTotalSeconds();
    virtual int     getTotalFrames();
    virtual int     getCurrentFrame();
    virtual double  getFPS();

    virtual bool    load(const std::string& _filepath, bool _vFlip, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    update();
    virtual void    clear();

    bool device;

private:
    double          dts_to_sec(int64_t dts);
    int64_t         dts_to_frame_number(int64_t dts);

    AVFormatContext *av_format_ctx;
    AVCodecContext  *av_codec_ctx;
    AVCodec         *av_decoder;
    AVFrame         *av_frame;
    AVPacket        *av_packet;
    AVRational      time_base;
    SwsContext      *conv_ctx;
        
    uint8_t         *frame_data;
    long            m_currentFrame;
    int             m_streamId;

};

#endif
