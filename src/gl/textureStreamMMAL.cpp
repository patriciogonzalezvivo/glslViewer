#include "textureStreamMMAL.h"
#include "../window.h"

#include <iostream>

#if defined(DRIVER_VC)

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Stills format information
// 0 implies variable
#define STILLS_FRAME_RATE_NUM 0
#define STILLS_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define MAX_USER_EXIF_TAGS      32
#define MAX_EXIF_PAYLOAD_LENGTH 128

/// Frame advance method
#define FRAME_NEXT_SINGLE        0
#define FRAME_NEXT_TIMELAPSE     1
#define FRAME_NEXT_KEYPRESS      2
#define FRAME_NEXT_FOREVER       3
#define FRAME_NEXT_GPIO          4
#define FRAME_NEXT_SIGNAL        5
#define FRAME_NEXT_IMMEDIATELY   6

#define check() assert(glGetError() == 0)

#include "interface/vcos/vcos.h"
#include "interface/vmcs_host/vc_vchi_gencmd.h"

// There isn't actually a MMAL structure for the following, so make one
typedef struct {
   int enable;       /// Turn colourFX on or off
   int u,v;          /// U and V to use
} MMAL_PARAM_COLOURFX_T;

typedef struct {
   int enable;
   int width,height;
   int quality;
} MMAL_PARAM_THUMBNAIL_CONFIG_T;

typedef struct {
   double x;
   double y;
   double w;
   double h;
} PARAM_FLOAT_RECT_T;

/// struct contain camera settings
typedef struct {
   int sharpness;             /// -100 to 100
   int contrast;              /// -100 to 100
   int brightness;            ///  0 to 100
   int saturation;            ///  -100 to 100
   int ISO;                   ///  TODO : what range?
   int videoStabilisation;    /// 0 or 1 (false or true)
   int exposureCompensation;  /// -10 to +10 ?
   MMAL_PARAM_EXPOSUREMODE_T exposureMode;
   MMAL_PARAM_EXPOSUREMETERINGMODE_T exposureMeterMode;
   MMAL_PARAM_AWBMODE_T awbMode;
   MMAL_PARAM_IMAGEFX_T imageEffect;
   MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imageEffectsParameters;
   MMAL_PARAM_COLOURFX_T colourEffects;
   int rotation;              /// 0-359
   int hflip;                 /// 0 or 1
   int vflip;                 /// 0 or 1
   PARAM_FLOAT_RECT_T  roi;   /// region of interest to use on the sensor. Normalised [0,1] values in the rect
   int shutter_speed;         /// 0 = auto, otherwise the shutter speed in ms
} RASPICAM_CAMERA_PARAMETERS;

MMAL_PORT_T         *preview_port   = NULL;
MMAL_PORT_T         *video_port     = NULL;
MMAL_PORT_T         *still_port     = NULL;
MMAL_QUEUE_T        *video_queue    = NULL;
MMAL_POOL_T         *video_pool     = NULL;

RASPICAM_CAMERA_PARAMETERS camera_parameters;

/**
 * Convert a MMAL status return value to a simple boolean of success
 * ALso displays a fault if code is not success
 *
 * @param status The error code to convert
 * @return 0 if status is sucess, 1 otherwise
 */
int mmal_status_to_int(MMAL_STATUS_T status) {
    if (status == MMAL_SUCCESS)
        return 0;
    else {
        switch (status) {
            case MMAL_ENOMEM :   vcos_log_error("Out of memory"); break;
            case MMAL_ENOSPC :   vcos_log_error("Out of resources (other than memory)"); break;
            case MMAL_EINVAL:    vcos_log_error("Argument is invalid"); break;
            case MMAL_ENOSYS :   vcos_log_error("Function not implemented"); break;
            case MMAL_ENOENT :   vcos_log_error("No such file or directory"); break;
            case MMAL_ENXIO :    vcos_log_error("No such device or address"); break;
            case MMAL_EIO :      vcos_log_error("I/O error"); break;
            case MMAL_ESPIPE :   vcos_log_error("Illegal seek"); break;
            case MMAL_ECORRUPT : vcos_log_error("Data is corrupt \attention FIXME: not POSIX"); break;
            case MMAL_ENOTREADY :vcos_log_error("Component is not ready \attention FIXME: not POSIX"); break;
            case MMAL_ECONFIG :  vcos_log_error("Component is not configured \attention FIXME: not POSIX"); break;
            case MMAL_EISCONN :  vcos_log_error("Port is already connected "); break;
            case MMAL_ENOTCONN : vcos_log_error("Port is disconnected"); break;
            case MMAL_EAGAIN :   vcos_log_error("Resource temporarily unavailable. Try again later"); break;
            case MMAL_EFAULT :   vcos_log_error("Bad address"); break;
            default :            vcos_log_error("Unknown status error"); break;
        }

        return 1;
    }
}

/**
 * Give the supplied parameter block a set of default values
 * @params Pointer to parameter block
 */
void raspicamcontrol_set_defaults(RASPICAM_CAMERA_PARAMETERS *params) {
    vcos_assert(params);

    params->sharpness = 0;
    params->contrast = 0;
    params->brightness = 50;
    params->saturation = 0;
    params->ISO = 0;                    // 0 = auto
    params->videoStabilisation = 0;
    params->exposureCompensation = 0;
    params->exposureMode = MMAL_PARAM_EXPOSUREMODE_AUTO;
    params->exposureMeterMode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
    params->awbMode = MMAL_PARAM_AWBMODE_AUTO;
    params->imageEffect = MMAL_PARAM_IMAGEFX_NONE;
    params->colourEffects.enable = 0;
    params->colourEffects.u = 128;
    params->colourEffects.v = 128;
    params->rotation = 0;
    params->hflip = params->vflip = 0;
    params->roi.x = params->roi.y = 0.0;
    params->roi.w = params->roi.h = 1.0;
    params->shutter_speed = 0;          // 0 = auto
}

/**
 * Get all the current camera parameters from specified camera component
 * @param camera Pointer to camera component
 * @param params Pointer to parameter block to accept settings
 * @return 0 if successful, non-zero if unsuccessful
 */
int raspicamcontrol_get_all_parameters(MMAL_COMPONENT_T *camera, RASPICAM_CAMERA_PARAMETERS *params) {
    vcos_assert(camera);
    vcos_assert(params);

    if (!camera || !params)
        return 1;

/* TODO : Write these get functions
   params->sharpness = raspicamcontrol_get_sharpness(camera);
   params->contrast = raspicamcontrol_get_contrast(camera);
   params->brightness = raspicamcontrol_get_brightness(camera);
   params->saturation = raspicamcontrol_get_saturation(camera);
   params->ISO = raspicamcontrol_get_ISO(camera);
   params->videoStabilisation = raspicamcontrol_get_video_stabilisation(camera);
   params->exposureCompensation = raspicamcontrol_get_exposure_compensation(camera);
   params->exposureMode = raspicamcontrol_get_exposure_mode(camera);
   params->awbMode = raspicamcontrol_get_awb_mode(camera);
   params->imageEffect = raspicamcontrol_get_image_effect(camera);
   params->colourEffects = raspicamcontrol_get_colour_effect(camera);
   params->thumbnailConfig = raspicamcontrol_get_thumbnail_config(camera);
*/
    return 0;
}

/**
 * Adjust the saturation level for images
 * @param camera Pointer to camera component
 * @param saturation Value to adjust, -100 to 100
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_saturation(MMAL_COMPONENT_T *camera, int saturation) {
    int ret = 0;

    if (!camera)
        return 1;

    if (saturation >= -100 && saturation <= 100) {
        MMAL_RATIONAL_T value = {saturation, 100};
        ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SATURATION, value));
    }
    else {
        vcos_log_error("Invalid saturation value");
        ret = 1;
    }

    return ret;
}

/**
 * Set the sharpness of the image
 * @param camera Pointer to camera component
 * @param sharpness Sharpness adjustment -100 to 100
 */
int raspicamcontrol_set_sharpness(MMAL_COMPONENT_T *camera, int sharpness) {
    int ret = 0;

    if (!camera)
        return 1;

    if (sharpness >= -100 && sharpness <= 100) {
        MMAL_RATIONAL_T value = {sharpness, 100};
        ret = mmal_status_to_int( mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SHARPNESS, value) );
    }
    else {
        vcos_log_error("Invalid sharpness value");
        ret = 1;
    }

    return ret;
}

/**
 * Set the contrast adjustment for the image
 * @param camera Pointer to camera component
 * @param contrast Contrast adjustment -100 to  100
 * @return
 */
int raspicamcontrol_set_contrast(MMAL_COMPONENT_T *camera, int contrast) {
    int ret = 0;

    if (!camera)
        return 1;

    if (contrast >= -100 && contrast <= 100) {
        MMAL_RATIONAL_T value = {contrast, 100};
        ret = mmal_status_to_int( mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_CONTRAST, value) );
    }
    else {
        vcos_log_error("Invalid contrast value");
        ret = 1;
    }

    return ret;
}

/**
 * Adjust the brightness level for images
 * @param camera Pointer to camera component
 * @param brightness Value to adjust, 0 to 100
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_brightness(MMAL_COMPONENT_T *camera, int brightness) {
    int ret = 0;

    if (!camera)
        return 1;

    if (brightness >= 0 && brightness <= 100) {
        MMAL_RATIONAL_T value = {brightness, 100};
        ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_BRIGHTNESS, value));
    }
    else {
        vcos_log_error("Invalid brightness value");
        ret = 1;
    }

    return ret;
}

/**
 * Adjust the ISO used for images
 * @param camera Pointer to camera component
 * @param ISO Value to set TODO :
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_ISO(MMAL_COMPONENT_T *camera, int ISO) {
    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO, ISO));
}

/**
 * Adjust the metering mode for images
 * @param camera Pointer to camera component
 * @param saturation Value from following
 *   - MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE,
 *   - MMAL_PARAM_EXPOSUREMETERINGMODE_SPOT,
 *   - MMAL_PARAM_EXPOSUREMETERINGMODE_BACKLIT,
 *   - MMAL_PARAM_EXPOSUREMETERINGMODE_MATRIX
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_metering_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMETERINGMODE_T m_mode ) {
    MMAL_PARAMETER_EXPOSUREMETERINGMODE_T meter_mode = { {MMAL_PARAMETER_EXP_METERING_MODE,sizeof(meter_mode)}, m_mode};
    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set(camera->control, &meter_mode.hdr));
}


/**
 * Set the video stabilisation flag. Only used in video mode
 * @param camera Pointer to camera component
 * @param saturation Flag 0 off 1 on
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_video_stabilisation(MMAL_COMPONENT_T *camera, int vstabilisation) {
    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set_boolean(camera->control, MMAL_PARAMETER_VIDEO_STABILISATION, vstabilisation));
}

/**
 * Adjust the exposure compensation for images (EV)
 * @param camera Pointer to camera component
 * @param exp_comp Value to adjust, -10 to +10
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_exposure_compensation(MMAL_COMPONENT_T *camera, int exp_comp) {
    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set_int32(camera->control, MMAL_PARAMETER_EXPOSURE_COMP , exp_comp));
}


/**
 * Set exposure mode for images
 * @param camera Pointer to camera component
 * @param mode Exposure mode to set from
 *   - MMAL_PARAM_EXPOSUREMODE_OFF,
 *   - MMAL_PARAM_EXPOSUREMODE_AUTO,
 *   - MMAL_PARAM_EXPOSUREMODE_NIGHT,
 *   - MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW,
 *   - MMAL_PARAM_EXPOSUREMODE_BACKLIGHT,
 *   - MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT,
 *   - MMAL_PARAM_EXPOSUREMODE_SPORTS,
 *   - MMAL_PARAM_EXPOSUREMODE_SNOW,
 *   - MMAL_PARAM_EXPOSUREMODE_BEACH,
 *   - MMAL_PARAM_EXPOSUREMODE_VERYLONG,
 *   - MMAL_PARAM_EXPOSUREMODE_FIXEDFPS,
 *   - MMAL_PARAM_EXPOSUREMODE_ANTISHAKE,
 *   - MMAL_PARAM_EXPOSUREMODE_FIREWORKS,
 *
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_exposure_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMODE_T mode) {
    MMAL_PARAMETER_EXPOSUREMODE_T exp_mode = {{MMAL_PARAMETER_EXPOSURE_MODE,sizeof(exp_mode)}, mode};

    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set(camera->control, &exp_mode.hdr));
}


/**
 * Set the aWB (auto white balance) mode for images
 * @param camera Pointer to camera component
 * @param awb_mode Value to set from
 *   - MMAL_PARAM_AWBMODE_OFF,
 *   - MMAL_PARAM_AWBMODE_AUTO,
 *   - MMAL_PARAM_AWBMODE_SUNLIGHT,
 *   - MMAL_PARAM_AWBMODE_CLOUDY,
 *   - MMAL_PARAM_AWBMODE_SHADE,
 *   - MMAL_PARAM_AWBMODE_TUNGSTEN,
 *   - MMAL_PARAM_AWBMODE_FLUORESCENT,
 *   - MMAL_PARAM_AWBMODE_INCANDESCENT,
 *   - MMAL_PARAM_AWBMODE_FLASH,
 *   - MMAL_PARAM_AWBMODE_HORIZON,
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_awb_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_AWBMODE_T awb_mode) {
    MMAL_PARAMETER_AWBMODE_T param = {{MMAL_PARAMETER_AWB_MODE,sizeof(param)}, awb_mode};

    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
}

/**
 * Set the image effect for the images
 * @param camera Pointer to camera component
 * @param imageFX Value from
 *   - MMAL_PARAM_IMAGEFX_NONE,
 *   - MMAL_PARAM_IMAGEFX_NEGATIVE,
 *   - MMAL_PARAM_IMAGEFX_SOLARIZE,
 *   - MMAL_PARAM_IMAGEFX_POSTERIZE,
 *   - MMAL_PARAM_IMAGEFX_WHITEBOARD,
 *   - MMAL_PARAM_IMAGEFX_BLACKBOARD,
 *   - MMAL_PARAM_IMAGEFX_SKETCH,
 *   - MMAL_PARAM_IMAGEFX_DENOISE,
 *   - MMAL_PARAM_IMAGEFX_EMBOSS,
 *   - MMAL_PARAM_IMAGEFX_OILPAINT,
 *   - MMAL_PARAM_IMAGEFX_HATCH,
 *   - MMAL_PARAM_IMAGEFX_GPEN,
 *   - MMAL_PARAM_IMAGEFX_PASTEL,
 *   - MMAL_PARAM_IMAGEFX_WATERCOLOUR,
 *   - MMAL_PARAM_IMAGEFX_FILM,
 *   - MMAL_PARAM_IMAGEFX_BLUR,
 *   - MMAL_PARAM_IMAGEFX_SATURATION,
 *   - MMAL_PARAM_IMAGEFX_COLOURSWAP,
 *   - MMAL_PARAM_IMAGEFX_WASHEDOUT,
 *   - MMAL_PARAM_IMAGEFX_POSTERISE,
 *   - MMAL_PARAM_IMAGEFX_COLOURPOINT,
 *   - MMAL_PARAM_IMAGEFX_COLOURBALANCE,
 *   - MMAL_PARAM_IMAGEFX_CARTOON,
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_imageFX(MMAL_COMPONENT_T *camera, MMAL_PARAM_IMAGEFX_T imageFX) {
    MMAL_PARAMETER_IMAGEFX_T imgFX = {{MMAL_PARAMETER_IMAGE_EFFECT,sizeof(imgFX)}, imageFX};

    if (!camera)
        return 1;

    return mmal_status_to_int( mmal_port_parameter_set(camera->control, &imgFX.hdr) );
}

/* TODO :what to do with the image effects parameters?
   MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imfx_param = {{MMAL_PARAMETER_IMAGE_EFFECT_PARAMETERS,sizeof(imfx_param)},
                              imageFX, 0, {0}};
mmal_port_parameter_set(camera->control, &imfx_param.hdr);
                             */

/**
 * Set the colour effect  for images (Set UV component)
 * @param camera Pointer to camera component
 * @param colourFX  Contains enable state and U and V numbers to set (e.g. 128,128 = Black and white)
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_colourFX(MMAL_COMPONENT_T *camera, const MMAL_PARAM_COLOURFX_T *colourFX) {
    MMAL_PARAMETER_COLOURFX_T colfx = {{MMAL_PARAMETER_COLOUR_EFFECT,sizeof(colfx)}, 0, 0, 0};

    if (!camera)
        return 1;

    colfx.enable = colourFX->enable;
    colfx.u = colourFX->u;
    colfx.v = colourFX->v;

    return mmal_status_to_int(mmal_port_parameter_set(camera->control, &colfx.hdr));

}


/**
 * Set the rotation of the image
 * @param camera Pointer to camera component
 * @param rotation Degree of rotation (any number, but will be converted to 0,90,180 or 270 only)
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_rotation(MMAL_COMPONENT_T *camera, int rotation) {
    int ret;
    int my_rotation = ((rotation % 360 ) / 90) * 90;

    ret = mmal_port_parameter_set_int32(camera->output[0], MMAL_PARAMETER_ROTATION, my_rotation);
    mmal_port_parameter_set_int32(camera->output[1], MMAL_PARAMETER_ROTATION, my_rotation);
    mmal_port_parameter_set_int32(camera->output[2], MMAL_PARAMETER_ROTATION, my_rotation);

    return ret;
}

/**
 * Set the flips state of the image
 * @param camera Pointer to camera component
 * @param hflip If true, horizontally flip the image
 * @param vflip If true, vertically flip the image
 *
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_flips(MMAL_COMPONENT_T *camera, int hflip, int vflip) {
    MMAL_PARAMETER_MIRROR_T mirror = {{MMAL_PARAMETER_MIRROR, sizeof(MMAL_PARAMETER_MIRROR_T)}, MMAL_PARAM_MIRROR_NONE};

    if (hflip && vflip)
        mirror.value = MMAL_PARAM_MIRROR_BOTH;
    else
    if (hflip)
        mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
    else
    if (vflip)
        mirror.value = MMAL_PARAM_MIRROR_VERTICAL;

    mmal_port_parameter_set(camera->output[0], &mirror.hdr);
    mmal_port_parameter_set(camera->output[1], &mirror.hdr);
    return mmal_port_parameter_set(camera->output[2], &mirror.hdr);
}

/**
 * Set the ROI of the sensor to use for captures/preview
 * @param camera Pointer to camera component
 * @param rect   Normalised coordinates of ROI rectangle
 *
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_ROI(MMAL_COMPONENT_T *camera, PARAM_FLOAT_RECT_T rect) {
    MMAL_PARAMETER_INPUT_CROP_T crop = {{MMAL_PARAMETER_INPUT_CROP, sizeof(MMAL_PARAMETER_INPUT_CROP_T)}};

    crop.rect.x = (65536 * rect.x);
    crop.rect.y = (65536 * rect.y);
    crop.rect.width = (65536 * rect.w);
    crop.rect.height = (65536 * rect.h);

    return mmal_port_parameter_set(camera->control, &crop.hdr);
}

/**
 * Adjust the exposure time used for images
 * @param camera Pointer to camera component
 * @param shutter speed in microseconds
 * @return 0 if successful, non-zero if any parameters out of range
 */
int raspicamcontrol_set_shutter_speed(MMAL_COMPONENT_T *camera, int speed) {
    if (!camera)
        return 1;

    return mmal_status_to_int(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_SHUTTER_SPEED, speed));
}



/**
 * Asked GPU how much memory it has allocated
 *
 * @return amount of memory in MB
 */
static int raspicamcontrol_get_mem_gpu(void) {
    char response[80] = "";
    int gpu_mem = 0;
    if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0)
        vc_gencmd_number_property(response, "gpu", &gpu_mem);
    return gpu_mem;
}

/**
 * Ask GPU about its camera abilities
 * @param supported None-zero if software supports the camera 
 * @param detected  None-zero if a camera has been detected
 */
static void raspicamcontrol_get_camera(int *supported, int *detected) {
    char response[80] = "";
    if (vc_gencmd(response, sizeof response, "get_camera") == 0)
    {
        if (supported)
            vc_gencmd_number_property(response, "supported", supported);
        if (detected)
            vc_gencmd_number_property(response, "detected", detected);
    }
}

/**
 * Check to see if camera is supported, and we have allocated enough meooryAsk GPU about its camera abilities
 * @param supported None-zero if software supports the camera 
 * @param detected  None-zero if a camera has been detected
 */
void raspicamcontrol_check_configuration(int min_gpu_mem) {
    int gpu_mem = raspicamcontrol_get_mem_gpu();
    int supported = 0, detected = 0;
    raspicamcontrol_get_camera(&supported, &detected);
    if (!supported)
        vcos_log_error("Camera is not enabled in this build. Try running \"sudo raspi-config\" and ensure that \"camera\" has been enabled\n");
    else if (gpu_mem < min_gpu_mem)
        vcos_log_error("Only %dM of gpu_mem is configured. Try running \"sudo raspi-config\" and ensure that \"memory_split\" has a value of %d or greater\n", gpu_mem, min_gpu_mem);
    else if (!detected)
        vcos_log_error("Camera is not detected. Please check carefully the camera module is installed correctly\n");
    else
        vcos_log_error("Failed to run camera app. Please check for firmware updates\n");
}

/**
 * Set the specified camera to all the specified settings
 * @param camera Pointer to camera component
 * @param params Pointer to parameter block containing parameters
 * @return 0 if successful, none-zero if unsuccessful.
 */
int raspicamcontrol_set_all_parameters(MMAL_COMPONENT_T *camera, const RASPICAM_CAMERA_PARAMETERS *params) {
    int result;
    result  = raspicamcontrol_set_saturation(camera, params->saturation);
    result += raspicamcontrol_set_sharpness(camera, params->sharpness);
    result += raspicamcontrol_set_contrast(camera, params->contrast);
    result += raspicamcontrol_set_brightness(camera, params->brightness);
    result += raspicamcontrol_set_ISO(camera, params->ISO);
    result += raspicamcontrol_set_video_stabilisation(camera, params->videoStabilisation);
    result += raspicamcontrol_set_exposure_compensation(camera, params->exposureCompensation);
    result += raspicamcontrol_set_exposure_mode(camera, params->exposureMode);
    result += raspicamcontrol_set_metering_mode(camera, params->exposureMeterMode);
    result += raspicamcontrol_set_awb_mode(camera, params->awbMode);
    result += raspicamcontrol_set_imageFX(camera, params->imageEffect);
    result += raspicamcontrol_set_colourFX(camera, &params->colourEffects);
    //result += raspicamcontrol_set_thumbnail_parameters(camera, &params->thumbnailConfig);  TODO Not working for some reason
    result += raspicamcontrol_set_rotation(camera, params->rotation);
    result += raspicamcontrol_set_flips(camera, params->hflip, params->vflip);
    result += raspicamcontrol_set_ROI(camera, params->roi);
    result += raspicamcontrol_set_shutter_speed(camera, params->shutter_speed);
    return result;
}

TextureStreamMMAL::TextureStreamMMAL() {

}

TextureStreamMMAL::~TextureStreamMMAL() {
    clear();
}

bool TextureStreamMMAL::load(const std::string& _filepath, bool _vFlip) {
    m_width = 640;
    m_height = 480;
    m_fps = 30;
    m_path = _filepath;
    m_vFlip = _vFlip;

    camera_component = 0;
    MMAL_ES_FORMAT_T *format;

    MMAL_STATUS_T status;
        
    //create the camera component
    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera_component);
    if (status != MMAL_SUCCESS) {
        printf("Failed to create camera component : error %d\n", status);
        clear();
        return false;
    }

    //check we have output ports
    if (!camera_component->output_num) {
        status = MMAL_ENOSYS;
        printf("Camera doesn't have output ports\n");
        clear();
        return false;
    }

    // Get the 3 ports
    preview_port = camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
    video_port = camera_component->output[MMAL_CAMERA_VIDEO_PORT];
    still_port = camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
        
    // Enable the camera, and tell it its control callback function
    status = mmal_port_enable(camera_component->control, camera_control_callback);
    if (status != MMAL_SUCCESS) {
        printf("Unable to enable control port : error %d\n", status);
        clear();
        return false;
    }
        
    //  set up the camera configuration
    MMAL_PARAMETER_CAMERA_CONFIG_T cam_config;
    cam_config.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
    cam_config.hdr.size = sizeof(cam_config);
    cam_config.max_stills_w = m_width;
    cam_config.max_stills_h = m_height;
    cam_config.stills_yuv422 = 0;
    cam_config.one_shot_stills = 0;
    cam_config.max_preview_video_w = m_width;
    cam_config.max_preview_video_h = m_height;
    cam_config.num_preview_video_frames = 3;
    cam_config.stills_capture_circular_buffer_height = 0;
    cam_config.fast_preview_resume = 0;
    cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;
    status = mmal_port_parameter_set(camera_component->control, &cam_config.hdr);
    if (status != MMAL_SUCCESS) {
        printf("Unable to set camera parameters : error %d\n", status);
        clear();
        return false;
    }
        
    // setup preview port format - QUESTION: Needed if we aren't using preview?
    format = preview_port->format;
    format->encoding = MMAL_ENCODING_OPAQUE;
    format->encoding_variant = MMAL_ENCODING_I420;
    format->es->video.width = m_width;
    format->es->video.height = m_height;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = m_width;
    format->es->video.crop.height = m_height;
    format->es->video.frame_rate.num = m_fps;
    format->es->video.frame_rate.den = 1;
    status = mmal_port_format_commit(preview_port);
    if (status != MMAL_SUCCESS) {
        printf("Couldn't set preview port format : error %d\n", status);
        clear();
        return false;
    }

    //setup video port format
    format = video_port->format;
    format->encoding = MMAL_ENCODING_I420;
    format->encoding_variant = MMAL_ENCODING_I420;
    format->es->video.width = m_width;
    format->es->video.height = m_height;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = m_width;
    format->es->video.crop.height = m_height;
    format->es->video.frame_rate.num = m_fps;
    format->es->video.frame_rate.den = 1;
    status = mmal_port_format_commit(video_port);
    if (status != MMAL_SUCCESS) {
        printf("Couldn't set video port format : error %d\n", status);
        clear();
        return false;
    }
        
    //setup still port format
    format = still_port->format;
    format->encoding = MMAL_ENCODING_OPAQUE;
    format->encoding_variant = MMAL_ENCODING_I420;
    format->es->video.width = m_width;
    format->es->video.height = m_height;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = m_width;
    format->es->video.crop.height = m_height;
    format->es->video.frame_rate.num = 1;
    format->es->video.frame_rate.den = 1;

    status = mmal_port_format_commit(still_port);
    if (status != MMAL_SUCCESS) {
        printf("Couldn't set still port format : error %d\n", status);
        clear();
        return false;
    }

    status = mmal_port_parameter_set_boolean(preview_port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    if (status != MMAL_SUCCESS) {
        printf("Failed to enable zero copy on camera video port\n");
        return false;
    }
        
    status = mmal_port_format_commit(preview_port);
    if (status != MMAL_SUCCESS) {
        printf("camera format couldn't be set\n");
        return false;
    }
        
    /* For GL a pool of opaque buffer handles must be allocated in the client.
    * These buffers are used to create the EGL images.
    */
    preview_port->buffer_num = 3;
    preview_port->buffer_size = preview_port->buffer_size_recommended;

    /* Pool + queue to hold preview frames */
    video_pool = mmal_port_pool_create(preview_port,preview_port->buffer_num,preview_port->buffer_size);
    if (!video_pool) {
        printf("Error allocating camera video pool. Buffer num: %d Buffer size: %d\n", preview_port->buffer_num, preview_port->buffer_size);
        status = MMAL_ENOMEM;
        return false;
    }
    printf("Allocated %d MMAL buffers of size %d.\n", preview_port->buffer_num, preview_port->buffer_size);

    /* Place filled buffers from the preview port in a queue to render */
    video_queue = mmal_queue_create();
    if (!video_queue) {
        printf("Error allocating video buffer queue\n");
        status = MMAL_ENOMEM;
        return false;
    }

    /* Enable video port callback */
    //port->userdata = (struct MMAL_PORT_USERDATA_T *)this;
    status = mmal_port_enable(preview_port, video_output_callback);
    if (status != MMAL_SUCCESS) {
        printf("Failed to enable video port\n");
        exit(1);
    }

    // Set up the camera_parameters to default
    raspicamcontrol_set_defaults(&camera_parameters);
    
    // Apply all camera parameters
    raspicamcontrol_set_all_parameters(camera_component, &camera_parameters);

    //enable the camera
    status = mmal_component_enable(camera_component);
    if (status != MMAL_SUCCESS) {
        printf("Couldn't enable camera\n\n");
        clear();
        return false;
    }
        
    // Send all the buffers in our pool to the video port ready for use
    {
        int num = mmal_queue_length(video_pool->queue);
        int q;
        for (q = 0; q < num; q++) {
            MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(video_pool->queue);
            if (!buffer) {
                printf("Unable to get a required buffer %d from pool queue\n\n", q);
                exit(1);
            }
            else if (mmal_port_send_buffer(preview_port, buffer)!= MMAL_SUCCESS) {
                printf("Unable to send a buffer to port (%d)\n\n", q);
                exit(1);
            }
        }
    }
      
    //begin capture
    if (mmal_port_parameter_set_boolean(video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
        printf("Failed to start capture\n\n");
        return false;
    }
        
    printf("Camera initialized.\n");
        
    // //Setup the camera's textures and EGL images.
    // glGenTextures(1, &cam_ytex);
    // glGenTextures(1, &cam_utex);
    // glGenTextures(1, &cam_vtex);

    // Generate an OpenGL texture ID for this texturez
    glEnable(GL_TEXTURE_2D);
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_id);

    // Load the texture
    glTexImage2D ( GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

    // Set the filtering mode
    glTexParameteri ( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
    return true;
}

void TextureStreamMMAL::bind() {
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_id);
}

void TextureStreamMMAL::camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    printf("Camera control callback\n\n");
    return;
}

void TextureStreamMMAL::video_output_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    //to handle the user not reading frames, remove and return any pre-existing ones
    if (mmal_queue_length(video_queue) >= 2) {
        if(MMAL_BUFFER_HEADER_T* existing_buffer = mmal_queue_get(video_queue)) {
            mmal_buffer_header_release(existing_buffer);
            if (port->is_enabled) {
                MMAL_STATUS_T status;
                MMAL_BUFFER_HEADER_T *new_buffer;
                new_buffer = mmal_queue_get(video_pool->queue);

                if (new_buffer)
                    status = mmal_port_send_buffer(port, new_buffer);

                if (!new_buffer || status != MMAL_SUCCESS)
                    printf("Unable to return a buffer to the video port\n\n");
            }   
        }
    }

    // add the buffer to the output queue
    mmal_queue_put(video_queue, buffer);
    // printf("Video buffer callback, output queue len=%d\n\n", mmal_queue_length(video_queue));
}

void updateTexture(EGLDisplay display, EGLenum target, EGLClientBuffer mm_buf, GLuint *texture, EGLImageKHR *egl_image) {
    // vcos_log_trace("%s: mm_buf %u", VCOS_FUNCTION, (unsigned) mm_buf);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, *texture);
    if (*egl_image != EGL_NO_IMAGE_KHR) {
        /* Discard the EGL image for the preview frame */
        eglDestroyImageKHR(display, *egl_image);
        *egl_image = EGL_NO_IMAGE_KHR;
    }

    // const EGLint attribs[] = {
    //         EGL_GL_TEXTURE_LEVEL_KHR, 0,
    //         EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
    //         EGL_NONE, EGL_NONE
    // };
    // *egl_image = eglCreateImageKHR(display, EGL_NO_CONTEXT, target, mm_buf, attribs);
    *egl_image = eglCreateImageKHR(display, EGL_NO_CONTEXT, target, mm_buf, NULL);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, *egl_image);
}

bool TextureStreamMMAL::update() {
    if (m_id == 0)
        return false;

    if (MMAL_BUFFER_HEADER_T* buf = mmal_queue_get(video_queue)) {
        // mmal_buffer_header_mem_lock(buf);
        // printf("Buffer received with length %d\n", buf->length);

        // updateTexture(getEGLDisplay(), EGL_IMAGE_BRCM_MULTIMEDIA, (EGLClientBuffer)buf->data, &m_id, &img);
        updateTexture(getEGLDisplay(), EGL_IMAGE_BRCM_MULTIMEDIA_Y, (EGLClientBuffer)buf->data, &m_id, &yimg);
        // updateTexture(getEGLDisplay(), EGL_IMAGE_BRCM_MULTIMEDIA_U, (EGLClientBuffer)buf->data, &m_id, &uimg);
        // updateTexture(getEGLDisplay(), EGL_IMAGE_BRCM_MULTIMEDIA_V, (EGLClientBuffer)buf->data, &m_id, &vimg);
        
        // mmal_buffer_header_mem_unlock(buf);
        mmal_buffer_header_release(buf);
        
        if(preview_port->is_enabled){
            MMAL_STATUS_T status;
            MMAL_BUFFER_HEADER_T *new_buffer;
            new_buffer = mmal_queue_get(video_pool->queue);
            if (new_buffer)
                status = mmal_port_send_buffer(preview_port, new_buffer);
            if (!new_buffer || status != MMAL_SUCCESS)
                printf("Unable to return a buffer to the video port\n\n");
        }
        
        return true;
    }
        
    return false; //no buffer received
}

void TextureStreamMMAL::clear() {
    if (camera_component)
        mmal_component_destroy(camera_component);
        
    if (video_queue)
        mmal_queue_destroy(video_queue);

    if (video_pool)
        mmal_port_pool_destroy(preview_port, video_pool);

    if (m_id != 0)
        glDeleteTextures(1, &m_id);
        
    m_id = 0;
}


#endif
