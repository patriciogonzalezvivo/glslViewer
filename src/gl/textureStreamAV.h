
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

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update();
    virtual void    clear();

private:

    AVFormatContext *av_format_ctx;
	AVCodecContext  *av_codec_ctx;
	AVStream        *av_video_stream;
	AVCodec         *av_decoder;
	AVFrame         *av_frame;
	AVPacket        *av_packet;
	AVRational 		time_base;
	SwsContext		*conv_ctx;
	
	uint8_t			*frame_data;
	int             m_streamId;

};

#endif
