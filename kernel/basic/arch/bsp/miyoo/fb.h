#ifndef __FB_H__
#define __FB_H__
#include <stdint.h>

typedef enum
{
    DISP_OUTPUT_PAL = 0,
    DISP_OUTPUT_NTSC,
    DISP_OUTPUT_960H_PAL,              /* ITU-R BT.1302 960 x 576 at 50 Hz (interlaced)*/
    DISP_OUTPUT_960H_NTSC,             /* ITU-R BT.1302 960 x 480 at 60 Hz (interlaced)*/

    DISP_OUTPUT_1080P24,
    DISP_OUTPUT_1080P25,
    DISP_OUTPUT_1080P30,

    DISP_OUTPUT_720P50,
    DISP_OUTPUT_720P60,
    DISP_OUTPUT_1080I50,
    DISP_OUTPUT_1080I60,
    DISP_OUTPUT_1080P50,
    DISP_OUTPUT_1080P60,

    DISP_OUTPUT_576P50,
    DISP_OUTPUT_480P60,

    DISP_OUTPUT_640x480_60,            /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
    DISP_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1024x768_60,           /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1366x768_60,           /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1440x900_60,           /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
    DISP_OUTPUT_1280x800_60,           /* 1280*800@60Hz VGA@60Hz*/
    DISP_OUTPUT_1680x1050_60,          /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1920x2160_30,          /* 1920x2160_30 */
    DISP_OUTPUT_1600x1200_60,          /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
    DISP_OUTPUT_1920x1200_60,          /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
    DISP_OUTPUT_2560x1440_30,          /* 2560x1440_30 */
    DISP_OUTPUT_2560x1600_60,          /* 2560x1600_60 */
    DISP_OUTPUT_3840x2160_30,          /* 3840x2160_30 */
    DISP_OUTPUT_3840x2160_60,          /* 3840x2160_60 */
    DISP_OUTPUT_USER,
    DISP_OUTPUT_MAX,
} DispDeviceTiming;

typedef struct
{
    uint16_t HsyncWidth;
    uint16_t HsyncBackPorch;
    uint16_t Hstart;
    uint16_t Hactive;
    uint16_t Htotal;

    uint16_t VsyncWidth;
    uint16_t VsyncBackPorch;
    uint16_t Vstart;
    uint16_t Vactive;
    uint16_t Vtotal;
    uint16_t Fps;

    uint16_t SscStep;
    uint16_t SscSpan;
}DispTimingConfig;

#endif