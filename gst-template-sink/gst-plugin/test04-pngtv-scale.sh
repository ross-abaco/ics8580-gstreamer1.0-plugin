#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*
export HEIGHT_PNG=8192
export WIDTH_PNG=8192
export TV_X=720
export TV_Y=576

# Extract the centre of the image without scaling
POS_X=$(( $WIDTH_PNG/2 - (TV_X - 2) ))
POS_Y=$(( $HEIGHT_PNG/2 - (TV_y - 2) ))

RIGHT=$POS_X
LEFT=$(($WIDTH_PNG - ($POS_X + $TV_X) ))
BOTTOM=$(($HEIGHT_PNG - ($POS_Y + $TV_Y) ))
TOP=$POS_Y

export TIMER='timeoverlay halignment=center valignment=bottom text="Stream time:" shaded-background=true font-desc="Sans, 24"'

# Good pipeline
gst-launch-1.0 filesrc location="test.png" ! pngdec ! imagefreeze ! videoscale ! videoconvert ! ${TIMER} ! "video/x-raw, width=720, height=576, pixel-aspect-ratio=1/1, framerate=25/1" ! 8580sink output=2 type=1 res=2 channel=1 sync=false

