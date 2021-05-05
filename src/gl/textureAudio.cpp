#include "textureAudio.h"

#include <iostream>
#include <mutex>

#ifdef SUPPORT_FOR_LIBAV

#define MA_NO_DECODING 
#define MA_NO_ENCODING 
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

extern "C" {
#include <libavcodec/avfft.h>
#include <libavutil/mem.h>
#include "miniaudio/miniaudio.h"
}

ma_device_config a_deviceConfig;
ma_device a_device;
static RDFTContext *ctx;

std::mutex mtx;

void data_collect_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

    std::vector<uint8_t> * m_buffer = (std::vector<uint8_t>*)pDevice->pUserData;
    uint8_t * m_buffer_wr = m_buffer->data();
    size_t m_buf_len = m_buffer->size();

    assert(m_buffer_wr != nullptr);

    std::lock_guard<std::mutex> lock(mtx);

    // shift left buffer's data to the number of new samples
    memmove(m_buffer_wr, m_buffer_wr+frameCount, m_buf_len-frameCount);
    memcpy(m_buffer_wr + m_buf_len-frameCount, pInput, frameCount);
    (void)pOutput;
}

TextureAudio::TextureAudio(): TextureStream() {
    // texture size
    m_width = 256;
    m_height = 1;
    m_texture.resize(m_width * m_height * 4, 0);
    m_dft_buffer = (float*)av_malloc_array(sizeof(float), m_buf_len);
    m_buffer_wr.resize(m_buf_len, 0);
    m_buffer_re.resize(m_buf_len, 0);
}

TextureAudio::~TextureAudio() {
    this->clear();
}

bool TextureAudio::load() {
    // set up audio format
    a_deviceConfig = ma_device_config_init(ma_device_type_capture);
    a_deviceConfig.capture.format   = ma_format_u8;
    a_deviceConfig.capture.channels = 1;
    a_deviceConfig.sampleRate       = 44100;
    a_deviceConfig.dataCallback     = data_collect_callback;
    a_deviceConfig.pUserData        = &m_buffer_wr;

    // init default capture device
    if (ma_device_init(NULL, &a_deviceConfig, &a_device) != MA_SUCCESS) {
        std::cout << "Failed to initialize capture device." << std::endl;
        return false;
    }

    // start capture device
    if (ma_device_start(&a_device) != MA_SUCCESS) {
        ma_device_uninit(&a_device);
        std::cout << "Failed to start device."  << std::endl;
        return false;
    }

    // init dft calculator
    ctx = av_rdft_init((int) log2(m_buf_len), DFT_R2C);
    if (!ctx) {
        std::cout << "Failed to init dft calculator."  << std::endl;
        return false;
    }

    return true;
}

bool TextureAudio::update() {

    {
        std::lock_guard<std::mutex> lock(mtx);
        m_buffer_re = m_buffer_wr;
    }

    // copy amplitude values to GREEN pixels
    for (int i = 0; i < m_width; i++) {
        m_texture[1+i*4] = m_buffer_re.at(m_buf_len - m_width + i);
    }

    // prerare samples for dtf
    for (int i = 0; i < m_buf_len; i++) {
        // hann window
        double window_modifier = (0.5 * (1 - cos(2 * M_PI * i / (m_buf_len - 1))));
        // scale value from 0-255 to -1.0-+1.0
        float value = (float)(window_modifier * ((m_buffer_re.at(i)) / 127.0f - 1.0f));
        // limit values
        if (value > 1.0) value = 1.0;
        if (value < -1.0) value = -1.0;
        m_dft_buffer[i] = value;
    }

    av_rdft_calc(ctx, m_dft_buffer);

    // copy frequency values to RED pixels
    for (int i = 0; i < m_width; i++) {
        
        // magnitude
        float im = m_dft_buffer[i*2];
        float re = m_dft_buffer[i*2 + 1];
        float mag = sqrt(im * im + re * re);

        // experimental scale factor
        float mag_scaled = sqrt(mag) * 50;

        // limit values
        // if (mag_scaled > 255.0) mag_scaled = 255.0;
        // if (mag_scaled < 0.0) mag_scaled = 0.0;

        uint8_t mag_uint8 = mag_scaled;

        auto prev_value = m_texture[4*(i)];

        // failing effect for decreasing value
        double decr_factor = 0.97;
        if (prev_value > mag_uint8) {
            m_texture[i*4] = prev_value * decr_factor;
        }
        else {
            m_texture[i*4] = mag_uint8;
        }
    }
    Texture::load(m_width, m_height, 4, 8, &m_texture[0]);
    return true;
}

void TextureAudio::clear() {
    ma_device_uninit(&a_device);

   if (ctx) {
        av_rdft_end(ctx); 
    }
    av_free(m_dft_buffer);
}

#endif
