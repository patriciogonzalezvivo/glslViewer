#include "textureStreamOMX.h"

#ifdef DRIVER_LEGACY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_host.h"
extern "C" {
#include "ilclient.h"
}

#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>

#include "interface/vcos/vcos_logging.h"

#include "interface/vmcs_host/vcilcs.h"
#include "interface/vmcs_host/vchost.h"
#include "interface/vmcs_host/vcilcs_common.h"

#ifdef SUPPORT_FOR_LIBAV
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
//#include <libavresample/avresample.h>
}
#endif

#include "../window.h"

TextureStreamOMX::TextureStreamOMX() : 
    m_eglImage(NULL),
    m_thread {},
    m_changed(false)
    {
    m_width = 1920;
    m_height = 1080;
}

TextureStreamOMX::~TextureStreamOMX() {
    clear();
}

// Helping functions from https://jan.newmarch.name/RPi/OpenMAX/Video/
// ---------------------------------------------------------------

static OMX_BUFFERHEADERTYPE* eglBuffer = NULL;
static COMPONENT_T* egl_render = NULL;
static OMX_VIDEO_CODINGTYPE decoderType = OMX_VIDEO_CodingAVC;
int thread_run = 0;

#ifdef SUPPORT_FOR_LIBAV
static AVStream *video_stream = NULL;
AVFormatContext *pFormatCtx = NULL;
static int video_stream_idx = -1;

void get_info(const char *filename, int* _width, int* _height) {
    // Register all formats and codecs
    av_register_all();
    av_log_set_level(AV_LOG_QUIET);

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0) {
        fprintf(stderr, "Can't get format\n");
        return -1; // Couldn't open file
    }
    
    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // Couldn't find stream information

    int ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret >= 0) {
        video_stream_idx = ret;
        video_stream = pFormatCtx->streams[video_stream_idx];
        *_width     = video_stream->codec->width;
        *_height    = video_stream->codec->height;

        // AVCodec *codec = avcodec_find_decoder(video_stream->codec->codec_id);
        // if (codec)
        //     printf("Codec name %s\n", codec->name);

        switch (video_stream->codec->codec_id) {
            case AV_CODEC_ID_H264:
                decoderType = OMX_VIDEO_CodingAVC;
                // printf("video codec AVC (H264)\n");
                break;

            case AV_CODEC_ID_MPEG4:
                decoderType = OMX_VIDEO_CodingMPEG4;
                // printf("video codec MPEG4\n");
                break;

            case AV_CODEC_ID_MPEG1VIDEO:
            case AV_CODEC_ID_MPEG2VIDEO:
                decoderType = OMX_VIDEO_CodingMPEG2;
                // printf("video codec MPEG2\n");
                break;

            case AV_CODEC_ID_H263:
                decoderType = OMX_VIDEO_CodingMPEG4;
                // printf("video codec MPEG4 (H263)\n");
                break;

            case AV_CODEC_ID_VP6:
            case AV_CODEC_ID_VP6F:
            case AV_CODEC_ID_VP6A:
                decoderType = OMX_VIDEO_CodingVP6;
                // printf("video codec VP6\n");
                break;

            case AV_CODEC_ID_VP8:
                decoderType = OMX_VIDEO_CodingVP8;
                // printf("video codec VP8\n");
                break;

            case AV_CODEC_ID_THEORA:
                decoderType = OMX_VIDEO_CodingTheora;
                // printf("video codec Theora\n");
                break;

            case AV_CODEC_ID_MJPEG:
            case AV_CODEC_ID_MJPEGB:
                decoderType = OMX_VIDEO_CodingMJPEG;
                // printf("video codec MJPEG\n");
                break;

            case AV_CODEC_ID_VC1:
            case AV_CODEC_ID_WMV3:
                decoderType = OMX_VIDEO_CodingWMV;
                // printf("video codec WMV\n");
                break;

            default:
                printf("Video codec unknown: %x \n", video_stream->codec->codec_id);
                break;
        }
    }

    if (pFormatCtx)
        avformat_free_context(pFormatCtx);
}
#endif

// Mostly from https://jan.newmarch.name/RPi/OpenMAX/EGL/
bool TextureStreamOMX::load(const std::string& _filepath, bool _vFlip, TextureFilter _filter, TextureWrap _wrap) {
    m_filter = _filter;
    m_wrap = _wrap;
    
    // TODOs:
    //  - get video width and height

    #ifdef SUPPORT_FOR_LIBAV
    get_info(_filepath.c_str(), &m_width, &m_height);
    #endif

    glEnable(GL_TEXTURE_2D);

    // load three texture buffers but use them on six OGL|ES texture surfaces
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(   GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getMinificationFilter(m_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getMagnificationFilter(m_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getWrap(m_wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getWrap(m_wrap));

    /* Create EGL Image */
    m_eglImage = createImage( getEGLDisplay(), getEGLContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)m_id, 0);
    
    if (m_eglImage == EGL_NO_IMAGE_KHR) {
        printf("eglCreateImageKHR failed.\n");
        exit(1);
    }

    m_thread = std::thread{decode_video, _filepath.c_str(), this};
    
    return true;
}


static OMX_ERRORTYPE set_video_decoder_input_format(COMPONENT_T *component) {
   // set input video format
    // printf("Setting video decoder format\n");
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    // format.eCompressionFormat = OMX_VIDEO_CodingAVC;
    format.eCompressionFormat = decoderType;
    return OMX_SetParameter(ilclient_get_handle(component), OMX_IndexParamVideoPortFormat, &format);
}

static OMX_ERRORTYPE set_clock(COMPONENT_T *component) {
    OMX_TIME_CONFIG_CLOCKSTATETYPE state;
    memset(&state, 0, sizeof(state));
    state.nSize = sizeof(state);
    state.nVersion.nVersion = OMX_VERSION;
    state.eState = OMX_TIME_ClockStateWaitingForStartTime;
    state.nWaitMask = 1;
    return OMX_SetParameter(ilclient_get_handle(component), OMX_IndexConfigTimeClockState, &state);
}

void my_fill_buffer_done(void* data, COMPONENT_T* comp) {
    OMX_STATETYPE state;

    if (OMX_GetState(ilclient_get_handle(egl_render), &state) != OMX_ErrorNone) {
        printf("OMX_FillThisBuffer failed while getting egl_render component state\n");
        return;
    }

    if (state != OMX_StateExecuting)
        return;

    if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
        printf("OMX_FillThisBuffer failed in callback\n");
}

// Modified function prototype to work with pthreads
void* TextureStreamOMX::decode_video(const char* filename, void* _streamTexture) {
    TextureStreamOMX* stream = (TextureStreamOMX*)_streamTexture;
    void* eglImage = stream->m_eglImage;

    if (eglImage == 0) {
        printf("eglImage is null.\n");
        exit(1);
    }

    COMPONENT_T *video_decode = NULL;
    COMPONENT_T *video_scheduler = NULL;
    COMPONENT_T *clock = NULL;
    COMPONENT_T *list[5];
    TUNNEL_T tunnel[4];
    ILCLIENT_T *client;

    FILE *in;
    int status = 0;
    unsigned int data_len = 0;

    thread_run = 1;

    memset(list, 0, sizeof(list));
    memset(tunnel, 0, sizeof(tunnel));

    if ((in = fopen(filename, "rb")) == NULL)
        return (void *)-2;

    if ((client = ilclient_init()) == NULL) {
        fclose(in);
        return (void *)-3;
    }

    if (OMX_Init() != OMX_ErrorNone) {
        ilclient_destroy(client);
        fclose(in);
        return (void *)-4;
    }

    // callback
    ilclient_set_fill_buffer_done_callback(client, my_fill_buffer_done, 0);

    // create video_decode
    // -------------------
    if (ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
        status = -14;
    list[0] = video_decode;

    // create egl_render
    // -----------------
    if (status == 0 && ilclient_create_component(client, &egl_render, "egl_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0)
        status = -14;
    list[1] = egl_render;

    // create clock
    // ------------
    if (status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
        status = -14;
    list[2] = clock;

    if (clock != NULL && set_clock(clock) != OMX_ErrorNone)
        status = -13;

    // create video_scheduler
    // ----------------------
    if (status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
        status = -14;
    list[3] = video_scheduler;

    // Setup tunnel
    // -------------
    set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
    set_tunnel(tunnel+1, video_scheduler, 11, egl_render, 220);
    set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

    // setup clock tunnel first
    if (status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
        status = -15;
    else
        ilclient_change_component_state(clock, OMX_StateExecuting);

    if (status == 0)
        ilclient_change_component_state(video_decode, OMX_StateIdle);

    if (status == 0 &&
        set_video_decoder_input_format(video_decode) == OMX_ErrorNone &&
        ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0) {
        
        OMX_BUFFERHEADERTYPE *buf;
        int port_settings_changed = 0;
        int first_packet = 1;
        ilclient_change_component_state(video_decode, OMX_StateExecuting);

        while((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL) {
            // feed data and wait until we get port settings changed
            unsigned char *dest = buf->pBuffer;

            // loop if at end
            if (feof(in))
                rewind(in);

            data_len += fread(dest, 1, buf->nAllocLen-data_len, in);

            if (port_settings_changed == 0 &&
                ((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
                (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1, ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {

                port_settings_changed = 1;

                if (ilclient_setup_tunnel(tunnel, 0, 0) != 0) {
                    status = -7;
                    break;
                }

                ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

                // now setup tunnel to egl_render
                if (ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0) {
                    status = -12;
                    break;
                }

                // Set egl_render to idle
                ilclient_change_component_state(egl_render, OMX_StateIdle);


                // Enable the output port and tell egl_render to use the texture as a buffer
                if (OMX_SendCommand(ilclient_get_handle(egl_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone) {
                    printf("OMX_CommandPortEnable failed.\n");
                    exit(1);
                }

                if (OMX_UseEGLImage(ilclient_get_handle(egl_render), &eglBuffer, 221, NULL, eglImage) != OMX_ErrorNone) {
                    printf("OMX_UseEGLImage failed.\n");
                    exit(1);
                }


                // Set egl_render to executing
                ilclient_change_component_state(egl_render, OMX_StateExecuting);


                // Request egl_render to write data to the texture buffer
                if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone) {
                    printf("OMX_FillThisBuffer failed.\n");
                    exit(1);
                }
            }

            if (!data_len)
                break;

            if (!thread_run)
                break;

            buf->nFilledLen = data_len;
            data_len = 0;

            buf->nOffset = 0;
            if (first_packet) {
                buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                first_packet = 0;
            }
            else
                buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

            if (OMX_EmptyThisBuffer(ilclient_get_handle(video_decode), buf) != OMX_ErrorNone) {
                status = -6;
                break;
            }
        }

        buf->nFilledLen = 0;
        buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

        if (OMX_EmptyThisBuffer(ilclient_get_handle(video_decode), buf) != OMX_ErrorNone)
            status = -20;

        // need to flush the renderer to allow video_decode to disable its input port
        ilclient_flush_tunnels(tunnel, 0);

        ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
    }

    fclose(in);

    ilclient_disable_tunnel(tunnel);
    ilclient_disable_tunnel(tunnel+1);
    ilclient_disable_tunnel(tunnel+2);
    ilclient_teardown_tunnels(tunnel);

    ilclient_state_transition(list, OMX_StateIdle);
    
    /*
        * To cleanup egl_render, we need to first disable its output port, then
        * free the output eglBuffer, and finally request the state transition
        * from to Loaded.
        */
    OMX_ERRORTYPE omx_err;
    omx_err = OMX_SendCommand(ilclient_get_handle(egl_render), OMX_CommandPortDisable, 221, NULL);
    if (omx_err != OMX_ErrorNone)
        printf("Failed OMX_SendCommand\n");
    
    omx_err = OMX_FreeBuffer(ilclient_get_handle(egl_render), 221, eglBuffer);
    if (omx_err != OMX_ErrorNone)
        printf("OMX_FreeBuffer failed\n");

    ilclient_state_transition(list, OMX_StateLoaded);
    
    ilclient_cleanup_components(list);

    OMX_Deinit();

    ilclient_destroy(client);
    return (void *)status;
}


void TextureStreamOMX::clear() {
    thread_run = 0;
    // pthread_join(thread1, NULL);
    m_thread.join();
        
    if (m_eglImage != 0) {
        if (!destroyImage(getEGLDisplay(), (EGLImageKHR) m_eglImage))
            printf("eglDestroyImageKHR failed.");
    }

    if (m_id != 0)
        glDeleteTextures(1, &m_id);
    m_id = 0;
}

#endif
