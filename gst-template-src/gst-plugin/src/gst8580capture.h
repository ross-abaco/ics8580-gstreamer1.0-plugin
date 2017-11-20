#ifndef GST_8580_CAPTURE_H
#define GST_8580_CAPTURE_H
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "ics8580FunctionalApi.h"

typedef unsigned char byte;

#define DMA_CHANNEL_0              0               /* PCIe data channel number (value from 0 to 3). */
#define DMA_CHANNEL_1              1               /* PCIe data channel number (value from 0 to 3). */
#define DMA_CHANNEL_2              2               /* PCIe data channel number (value from 0 to 3). */
#define DMA_CHANNEL_MAX            3               /* Max channels */

typedef struct
{   
  /* Video input fields. */
  int videoInput;              /**< Video input source for the stream */
  int videoInputType;          /**< Video input type for the source */
  int videoInputResolution;    /**< Video input resolution of the source */
} ICS8580_VIDEO_IN;

typedef struct
{   
  /* Video input fields. */
  int videoOutput;              /**< Video output source for the stream */
  int videoOutputType;          /**< Video output type for the source */
  int videoOutputResolution;    /**< Video output resolution of the source */
} ICS8580_VIDEO_OUT;

typedef struct
{   
  /* Video fields. */
  ICS8580_VIDEO_IN video_in;   /**< Video input  */
  ICS8580_VIDEO_OUT video_out; /**< Video output  */
  int debug;
  int output;
  int streamId;
  int online;
  int channel;
  char* username;
  char* password;
} ICS8580_USER_ARGS;

/* Forward declerations */
void resolutionTableLookup(int resolution,int *width,int *height,int *framerate);
int Init8580(ICS8580_USER_ARGS args);
int Init8580Channels(ICS8580_USER_ARGS args);
int Finalize8580(void);
int Finalize8580Channels(void);
void deInterlace(int videotype, byte * data);
int GetFrame8580(char *buf);
int fillTestRGB(char *winBuf,int size);
int ics8580_encode(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T *hndl, ICS8580_USER_ARGS *video_args);
int ics8580_encode_start(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T *hnd);
int ics8580_encode_stop(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T *hndl);
int ics8580_encode_pause(ICS8580_COMMUNICATION_CHANNEL_HANDLE_T *hndl);
int videoOpen(ICS8580_USER_ARGS args);
int videoClose(void);
#endif /* GST_8580_CAPTURE_H */

