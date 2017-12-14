/*
 * � Abaco Systems (2017)
 *
 * All rights reserved. No part of this software may be re-produced, re-engineered, 
 * re-compiled, modified, used to create derivatives, stored in a retrieval system, 
 * or transmitted in any form or by any means, electronic, mechanical, photocopying, 
 * recording, or otherwise without the prior written permission of Abaco 
 * Systems.
 */
#include "ics8580FunctionalApi.h"

#include <gst/gst.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/poll.h>

#include "gst8580capture.h"

#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wmissing-braces"

#ifdef WIN32
#define snprintf _snprintf
#endif
typedef unsigned long DWORD;
/* Globals definitions. Redefine them to fit a specific setup. */

#define NUMBER_OF_BUFFERS          2    /* Number of buffers allocated for DMA (2 for double buffering). */
#define DEBUG                      1    /* enable TRACE */
#define ENABLE_CONFIG              1
#define OUTPUT_ENABLED             1    /* Enable Input */
#define PCI_NAME                   "PCIe://0"
#define TIMESTAMP                  0

/* Some additional error codes */
#define ICS8580_INPUT_ERROR       -100
#define ICS8580_OUTPUT_ERROR      -200
#define ICS8580_STREAM_ERROR      -300

#if TIMESTAMP
#include <time.h>
clock_t start;
clock_t finish;
#endif

#if DEBUG
#if 0
#define TRACE(format, ...)  g_print(format,__VA_ARGS__)
#else
#define TRACE GST_LOG
#endif
#define TRACE_ERROR GST_ERROR
#else
#define TRACE(format, ...)
#define TRACE_ERROR GST_ERROR
#endif

#define HRESULT int
struct _RESOLUTION_TABLE {      /* Defines a lookup table with video resolution options */
    int resolution;             /* ICS8580_VIDEO_RESOLUTION values from ics8580FunctionalApi.h header */
    int width;                  /* Video data width for the selected resolution */
    int height;                 /* Video data height for the selected resolution */
    int framerate;              /* Video data frame rate for the selected resolution */
} ResolutionTable[] = {
ICS8580_VIDEO_RESOLUTION_NONE, 0, 0, 0,
        ICS8580_VIDEO_RESOLUTION_NTSC, 720, 487, 30,
        ICS8580_VIDEO_RESOLUTION_PAL, 720, 576, 25,
        ICS8580_VIDEO_RESOLUTION_525P_60, 720, 480, 60,
        ICS8580_VIDEO_RESOLUTION_625P_50, 720, 576, 50,
        ICS8580_VIDEO_RESOLUTION_720P_50, 1280, 720, 50,
        ICS8580_VIDEO_RESOLUTION_720P_60, 1280, 720, 60,
        ICS8580_VIDEO_RESOLUTION_1080I_50, 1920, 1080, 25,
        ICS8580_VIDEO_RESOLUTION_1080I_60, 1920, 1080, 30,
        ICS8580_VIDEO_RESOLUTION_1080P_24, 1920, 1080, 24,
        ICS8580_VIDEO_RESOLUTION_1080P_25, 1920, 1080, 25,
        ICS8580_VIDEO_RESOLUTION_1080P_30, 1920, 1080, 30,
        ICS8580_VIDEO_RESOLUTION_1080P_60, 1920, 1080, 60,
        ICS8580_VIDEO_RESOLUTION_VGA_60, 640, 480, 60,
        ICS8580_VIDEO_RESOLUTION_VGA_72, 640, 480, 72,
        ICS8580_VIDEO_RESOLUTION_VGA_75, 640, 480, 75,
        ICS8580_VIDEO_RESOLUTION_VGA_85, 640, 480, 85,
        ICS8580_VIDEO_RESOLUTION_SVGA_60, 800, 600, 60,
        ICS8580_VIDEO_RESOLUTION_SVGA_72, 800, 600, 72,
        ICS8580_VIDEO_RESOLUTION_SVGA_75, 800, 600, 75,
        ICS8580_VIDEO_RESOLUTION_SVGA_85, 800, 600, 85,
        ICS8580_VIDEO_RESOLUTION_XGA_60, 1024, 768, 60,
        ICS8580_VIDEO_RESOLUTION_XGA_70, 1024, 768, 70,
        ICS8580_VIDEO_RESOLUTION_XGA_75, 1024, 768, 75,
        ICS8580_VIDEO_RESOLUTION_XGA_85, 1024, 768, 85,
        ICS8580_VIDEO_RESOLUTION_SXGA_60, 1280, 1024, 60,
        ICS8580_VIDEO_RESOLUTION_STANAG_3350A, 1280, 875, 30,
        ICS8580_VIDEO_RESOLUTION_STANAG_3350B, 720, 576, 25,
        ICS8580_VIDEO_RESOLUTION_STANAG_3350C, 720, 487, 30,
        ICS8580_VIDEO_RESOLUTION_UXGA_60, 1600, 1200, 60};

void resolutionTableLookup(int resolution, int *width, int *height,
                           int *framerate)
{
    int i = 0;

    for (i = 0; i < 30; i++) {
        if (ResolutionTable[i].resolution == resolution) {
            *width = ResolutionTable[i].width;
            *height = ResolutionTable[i].height;
            *framerate = ResolutionTable[i].framerate;
        }
    }
}

typedef struct {
    unsigned char r, g, b, a;
} uchar4;

typedef struct {
    unsigned char r, g, b;
} uchar3;

ICS8580_COMMUNICATION_CHANNEL_HANDLE_T hChannelHandle =
    ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE;
ICS8580_COMMUNICATION_CHANNEL_HANDLE_T hPcieHandle =
    ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE;
ICS8580_DATA_CHANNEL_HANDLE_T hDataTransfer = NULL;
ICS8580_INT32_T nCount;
ICS8580_INT32_T nStatus;
ICS8580_INT32_T uSkipFrames = 0;
ICS8580_INT32_T uTxFrames = 0;
ICS8580_INT8_T *pBuffer[NUMBER_OF_BUFFERS];
int g_VideoW = 0;
int g_VideoH = 0;
int g_BufferAllocated = 0;

enum VIEWER_STATE {
    VIEWER_STATE_NOT_CONNECTED,
    VIEWER_STATE_CONNECT,
    VIEWER_STATE_CONNECTING,
    VIEWER_STATE_CONNECTED,
    VIEWER_STATE_DISCONNECTED,
    VIEWER_STATE_ERROR
};

int state = VIEWER_STATE_NOT_CONNECTED;

/*
  View the generated file 'viewer_ui.h' to see the following:
    - Constants that can be used.
    - Input group structure containing the input variables.
    - Output group structre containing the output variables.
    - The attribute initialization structures to be passed into dv_create().
    - Example code on how to instrument your code.
*/

ICS8580_VIDEO_IN input_presets[5] = {
    {ICS8580_VIDEO_INPUT5_SDI, ICS8580_VIDEO_TYPE_HD_SDI_SMPTE292M,
     ICS8580_VIDEO_RESOLUTION_1080I_60},
    {ICS8580_VIDEO_INPUT1_SD_HD_RGBHV_DVI, ICS8580_VIDEO_TYPE_ANALOG_RGBHV,
     ICS8580_VIDEO_RESOLUTION_UXGA_60},
    {ICS8580_VIDEO_INPUT2_SD_HD_RGBHV, ICS8580_VIDEO_TYPE_ANALOG_RGBHV,
     ICS8580_VIDEO_RESOLUTION_SXGA_60},
    {ICS8580_VIDEO_INPUT3_SD, ICS8580_VIDEO_TYPE_COMPOSITE,
     ICS8580_VIDEO_RESOLUTION_PAL},
    {ICS8580_VIDEO_INPUT4_SD, ICS8580_VIDEO_TYPE_COMPOSITE,
     ICS8580_VIDEO_RESOLUTION_PAL},
};

ICS8580_VIDEO_OUT output_presets[5] = {
    {ICS8580_VIDEO_OUTPUT4_SDI, ICS8580_VIDEO_TYPE_HD_SDI_SMPTE292M,
     ICS8580_VIDEO_RESOLUTION_1080I_60},
    {ICS8580_VIDEO_OUTPUT3_DVI, ICS8580_VIDEO_TYPE_HDMI_DVI,
     ICS8580_VIDEO_RESOLUTION_UXGA_60},
    {ICS8580_VIDEO_OUTPUT1_RGB, ICS8580_VIDEO_TYPE_ANALOG_RGBHV,
     ICS8580_VIDEO_RESOLUTION_SXGA_60},
    {ICS8580_VIDEO_OUTPUT2_TV, ICS8580_VIDEO_TYPE_COMPOSITE,
     ICS8580_VIDEO_RESOLUTION_PAL},
    {ICS8580_VIDEO_OUTPUT2_TV, ICS8580_VIDEO_TYPE_COMPOSITE,
     ICS8580_VIDEO_RESOLUTION_PAL},
};

static ICS8580_USER_ARGS params = { 0, 0, 0, 0, 0, 0, 0, 0 };

static int once_fill = 0;       /* Quick hack to reduce CPU load */

/*******************************************************************
* NAME :            videoOpen(ICS8580_USER_ARGS args)
*
* DESCRIPTION :     Initialise the video channel for input.
*
* INPUTS :
*       PARAMETERS:
*           ICS8580_USER_ARGS     args              GUI arguments struct.
* OUTPUTS :
*       PARAMETERS:
*           none
*       RETURN :
*           Type:   int               Result of update
*           Values: ICS8580_OK            success
*
* NOTES :           none.
*/
int videoOpen(ICS8580_USER_ARGS args)
{
    params = args;
    TRACE("Allocating video buffer.");
    return Init8580Channels(args);
}

int col = 0;

int videoClose(void)
{
    Finalize8580();
    return ICS8580_OK;
}

int Init8580(ICS8580_USER_ARGS args)
{
    ICS8580_CHAR_T pcieName[] = PCI_NAME;
    ICS8580_LOGIN_PARAMS sLoginParams;

    memset(&sLoginParams, 0, sizeof(ICS8580_LOGIN_PARAMS));

    /* Create Socket interface handle for configuring PCIe. Disable automatic event listener. */
    TRACE("Opening PCIe device %s --> \n", (char *) pcieName);
    if (NULL ==
        (hChannelHandle =
         ics8580DeviceOpenEx((char *) pcieName,
                             ICS8580_STOP_EVENT_LISTENER_FLAG))) {
        Finalize8580();
        TRACE_ERROR("Failed to open PCIe device 'PCIe:\\0'");
        return ICS8580_OUTPUT_ERROR;
    }

    /* Login to DSP application */
    strcpy((char *) sLoginParams.username, args.username);
    strcpy((char *) sLoginParams.password, args.password);
    if (ICS8580_OK != ics8580UserLogin(hChannelHandle, &sLoginParams)) {
        Finalize8580();
        TRACE_ERROR("ics8580UserLogin: user %s, password %s - failed\n",
                    sLoginParams.username, sLoginParams.password);
        return ICS8580_ERROR;
    }
    return ICS8580_OK;
}

/*******************************************************************
* NAME :            int Init8580Channels(void)
*
* DESCRIPTION :     Initialise the ICS-8580.
*
* INPUTS :
*       PARAMETERS:
*           none
* OUTPUTS :
*       PARAMETERS:
*           none
*       RETURN :
*            Type:   int                   Result of update
*            Values: 1                     success
*
* NOTES :           Default configuration is for PCI control and video channels (See PCI_CONFIG macro).
*                   Video input resolutions are hardcoded to XGA 1024x768 RBG.
*/
int Init8580Channels(ICS8580_USER_ARGS args)
{
    if (args.debug) {
        /* Nothing to do in debug mode */
    } else {
        /* Local variable declaration */
        ICS8580_CHAR_T pcieName[] = PCI_NAME;
        ICS8580_USER_PCIE_CONFIGS sPcieConfig;
        ICS8580_INT32_T enableConfig = ENABLE_CONFIG;
        ICS8580_ULONG_T uBufferSize;

        memset(&sPcieConfig, 0, sizeof(ICS8580_USER_PCIE_CONFIGS));
        g_VideoW =
            ResolutionTable[(int) args.video_out.videoOutputResolution].
            width;
        g_VideoH =
            ResolutionTable[(int) args.video_out.videoOutputResolution].
            height;

        /* Put selected values into the PCIe configuration structure */
        sPcieConfig.pcieChannelId = args.channel;
        sPcieConfig.pcieChannelEnable = 1;
        sPcieConfig.pcieMode = ICS8580_PCIE_MODE_TRANSMIT;
        sPcieConfig.pcieType = ICS8580_PCIE_TYPE_VIDEO;

        sPcieConfig.videoOutput =
            (ICS8580_VIDEO_OUTPUT) args.video_out.videoOutput;
        sPcieConfig.videoOutputType =
            (ICS8580_VIDEO_TYPE) args.video_out.videoOutputType;
        sPcieConfig.videoOutputResolution =
            (ICS8580_VIDEO_RESOLUTION) args.video_out.
            videoOutputResolution;
        uBufferSize = (ResolutionTable[(int) sPcieConfig.videoOutputResolution].width * ResolutionTable[(int) sPcieConfig.videoOutputResolution].height) * NUMBER_OF_BUFFERS;   /* Buffer size for YUV422 data */

        TRACE("\n---- User configurations for all channels---- \n");
        TRACE("\tX Resoltution: %d\n", g_VideoW);
        TRACE("\tY Resoltution: %d\n", g_VideoH);
        TRACE("\tY Video Format: YUV422\n");
        TRACE("\tPCIe Configuration: %s\n",
              ((1 == enableConfig) ? "enabled" : "disabled"));

        if (1 == enableConfig) {
            TRACE("\n---- PCIe configurations for channel %d ---- \n",
                  sPcieConfig.pcieChannelId);
            TRACE("\tOutput ADV Channel: %d\n", sPcieConfig.videoOutput);
            TRACE("\tOutput Format: %d\n", sPcieConfig.videoOutputType);
            TRACE("\tOutput Std: %d (%d,%d)\n",
                  sPcieConfig.videoOutputResolution, g_VideoW, g_VideoH);

            /* Check for HD output std on channel 2 & 3 */
            if ((2 == sPcieConfig.pcieChannelId)
                || (3 == sPcieConfig.pcieChannelId)) {
                if ((sPcieConfig.videoOutputResolution >=
                     ICS8580_VIDEO_RESOLUTION_720P_50)
                    && (sPcieConfig.videoOutputResolution <=
                        ICS8580_VIDEO_RESOLUTION_1080P_60)) {
                    TRACE_ERROR
                        ("HD resolution not supported on PCIe channel 2 & 3 \n");
                    Finalize8580Channels();
                    return ICS8580_ERROR;
                }
            }

            /* Call ics8580ConfigurePcieDevice for PCIe output configuration over socket */
            TRACE("Configuring PCIe channel --> \n");
            if (ICS8580_OK !=
                ics8580ConfigurePcieDevice(hChannelHandle, &sPcieConfig)) {
                TRACE_ERROR("failed to open\n");
                Finalize8580Channels();
                return ICS8580_ERROR;
            }
        }

        /* Create PCIe interface handles for data transfer */
        TRACE("Opening data device %s --> \n", (char *) pcieName);
        hPcieHandle = ics8580DataDeviceOpen(pcieName);
        if ((NULL == hPcieHandle)
            || (ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE ==
                hPcieHandle)) {
            TRACE_ERROR("failed\n");
            printf("failed\n");
            Finalize8580Channels();
            return ICS8580_INPUT_ERROR;
        }

        TRACE("Opened all channels success %d\n",
                        args.channel);

        /* Create Data Channel with the user parameters */
        if (ICS8580_OK != ics8580DataChannelCreate(&hDataTransfer,
                                                   hPcieHandle,
                                                   args.channel,
                                                   pBuffer,
                                                   uBufferSize,
                                                   ICS8580_PCIE_TYPE_VIDEO,
                                                   NUMBER_OF_BUFFERS)) {
            TRACE_ERROR("ics8580DataChannelCreate failed. channel %d\n",
                        args.channel);
        }

        /* Queue all the buffers */
        if (ICS8580_OK != ics8580StartWriteQueue(hDataTransfer)) {
            TRACE_ERROR("Failed to start the queue\n");
        }

        nCount = 0;
        TRACE
            ("Please wait while the application is receiving frames...\n");
    }

    return ICS8580_OK;
}

/*******************************************************************
* NAME :            int GetFrame8580(char **buf)
*
* DESCRIPTION :     Wait for one frame of video and return the buffer.
*
* INPUTS :
*       PARAMETERS:
*           none
* OUTPUTS :
*       PARAMETERS:
*           char     **buf             The video buffer.
*       RETURN :
*            Type:   int                   Result of update
*            Values: 1                     success
*
* NOTES :           none.
*/
#define STRIDE 1
int GetPut8580(char *buf, int interlaced)
{
    int width, height;
    width =
        ResolutionTable[(int) params.video_out.videoOutputResolution].
        width * 2;
    height =
        ResolutionTable[(int) params.video_out.videoOutputResolution].
        height;

    ICS8580_ULONG_T nIndex = nCount % NUMBER_OF_BUFFERS;

    TRACE("Processing frame %d %d...\n", nCount, nIndex);
    nCount++;

    if (ICS8580_OK != ics8580DataDequeue(hDataTransfer, nIndex, 7000 /*mseconds */ )) { /* Wait for completion for oldest pBuffer in the queue */

        ics8580DataDequeueGetReturn(hDataTransfer, nIndex, &nStatus);   /* Get PCIe transfer status */
        if (ICS8580_AIO_EINPROGRESS == nStatus) /* The transfer is in progress, must to wait until it will be released */
            return ICS8580_OK;

        uSkipFrames++;
        printf("Skipped %d of %d with error %d \n", uSkipFrames, nCount,
               nStatus);
    } else {
        /* No colour conversion required */
        if (!interlaced) {
            // Handle Progressive video
            memcpy((byte *) pBuffer[nIndex], (byte *) buf, width * 320);
        } else {
            // Handle Interlaced video
            {
                int i;
                byte *pbuff;

                pbuff = (byte *) pBuffer[nIndex];

                for (i = 0; i < height / 2; i++) {
                    memcpy(&pbuff[i * width],
                           (byte *) & buf[(i * 2) * width], width);
                    memcpy(&pbuff[(i + height / 2) * width],
                           (byte *) & buf[((i + 1) * 2) * width], width);
                }
            }
        }
/*      printf("Processing frame %d %d...\n", nCount, nIndex); */
        if (ICS8580_OK != ics8580DataWriteQueue(hDataTransfer, nIndex)) {       /* Re-submit the pBuffer */
            printf("Write Queue failed for channel\n");
        }
    }

    uTxFrames++;
    return ICS8580_OK;
}

/*******************************************************************
* NAME :            int Finalize8580 ( void )
*
* DESCRIPTION :     Finalize the video
*
* INPUTS :
* OUTPUTS :
*       PARAMETERS:
*           none
*       RETURN :
*            Type:   int                  On error the return value is false.
*
* NOTES :           none.
*/
int Finalize8580(void)
{
    /* Close PCIe communication channel 0 */
    if (ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE !=
        hChannelHandle) {
        ics8580DeviceClose(hChannelHandle);
        hChannelHandle =
            ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE;
    }
    return ICS8580_OK;
}

int Finalize8580Channels(void)
{
    if (!params.debug) {
        /* Close Data channel 0 */
        if (NULL != hDataTransfer) {
            ics8580DataChannelClose(hDataTransfer);
            hDataTransfer = NULL;
        }
        /* Close PCIe device 0 */
        if (ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE !=
            hPcieHandle) {
            ics8580DataDeviceClose(hPcieHandle);
            hPcieHandle =
                ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE;
        }

        if (params.output) {
            /* Queue all the buffers */
            if (ICS8580_OK != ics8580StopWriteQueue(hDataTransfer)) {
                TRACE_ERROR("Failed to stop the queue\n");
            }

            /* Close Data channel 1 */
            if (NULL != hDataTransfer) {
                ics8580DataChannelClose(hDataTransfer);
                hDataTransfer = NULL;
            }
            /* Close PCIe device 1 */
            if (ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE !=
                hPcieHandle) {
                ics8580DataDeviceClose(hPcieHandle);
                hPcieHandle =
                    ICS8580_INVALID_COMMUNICATION_CHANNEL_HANDLE_VALUE;
            }
        }
    }
    return ICS8580_OK;
}

int ics8580_encode_start(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T * handle)
{
    /* Start the streams */
    if (ICS8580_OK != ics8580StreamStartAll(*handle)) {
        TRACE_ERROR("ics8580StreamStartAll failed\n");
        return ICS8580_STREAM_ERROR;
    }
    TRACE("Stream started ...\n");
    return ICS8580_OK;
}

int ics8580_encode_stop(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T * handle)
{
    /* Stop the streams */
    if (ICS8580_OK != ics8580StreamStopAll(*handle)) {
        TRACE("ics8580StreamStopAll failed\n");
        return ICS8580_STREAM_ERROR;
    }
    TRACE("Stream stopped ...\n");
    return ICS8580_OK;
}

int paused = 0;
int ics8580_encode_pause(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T * handle)
{
    ICS8580_STREAM_PAUSE_PARAMS pause;
    /* Pause the streams */
    pause.streamId = (ICS8580_STREAM_ID) params.streamId;

    if (paused == 0) {
        if (ICS8580_OK != ics8580StreamPause(*handle, &pause)) {
            TRACE_ERROR("ics8580StreamPause failed\n");
            return ICS8580_STREAM_ERROR;
        }
        TRACE("Stream paused...\n");
        paused = 1;
    } else {
        ICS8580_STREAM_RESUME_PARAMS resume;
        resume.streamId = (ICS8580_STREAM_ID) params.streamId;
        if (ICS8580_OK != ics8580StreamResume(*handle, &resume)) {
            TRACE_ERROR("ics8580StreamResume failed\n");
            return ICS8580_STREAM_ERROR;
        }
        TRACE("Stream unpaused...\n");
        paused = 0;
    }
    return ICS8580_OK;
}
