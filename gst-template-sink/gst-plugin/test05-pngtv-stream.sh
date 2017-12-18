#!/bin/bash
#export GST_DEBUG=3,gst_8580sink_debug:*
export HEIGHT_PNG=8192
export WIDTH_PNG=8192
export TV_X=720
export TV_Y=576

source global.sh

# Extract the centre of the image without scaling
POS_X=$(( $WIDTH_PNG/2 - (TV_X - 2) ))
POS_Y=$(( $HEIGHT_PNG/2 - (TV_y - 2) ))

RIGHT=$POS_X
LEFT=$(( $WIDTH_PNG - ($POS_X + $TV_X) ))
BOTTOM=$(( $HEIGHT_PNG - ($POS_Y + $TV_Y) ))
TOP=$POS_Y

export TIMER='timeoverlay halignment=center valignment=bottom text="Stream time:" shaded-background=true font-desc="Sans, 24"'

echo $RIGHT,$TOP - $LEFT,$BOTTOM
echo $IPADDR:$PORT

#Cleanup and leftover processes
pkill gst-launch-1.0

gst-launch-1.0 filesrc location="test.png" ! pngdec ! imagefreeze ! \
    videocrop top=${TOP} left=${LEFT} right=${RIGHT} bottom=${BOTTOM} ! \
    videoconvert ! \
    $TIMER \
    ! "video/x-raw, width=${TV_X}, height=${TV_Y}, pixel-aspect-ratio=1/1, framerate=${FRAMERATE}/1, format=(string)UYVY" \
    ! videoconvert \
    ! queue \
    ! rtpvrawpay mtu=1472 ssrc=305419896 \
    ! udpsink  host=${IPADDR} port=${PORT} ttl-mc=15 loop=false &

# Stream RTP YUV raw to loopback interface
sleep 2
echo "################################# Streaming ####################################"
gst-launch-1.0 udpsrc port=${PORT} caps="application/x-rtp, media=(string)video, encoding-name=(string)RAW, sampling=(string)YCbCr-4:2:2, depth=(string)8, width=(string)${TV_X}, height=(string)${TV_Y}, payload=(int)96, framerate=${FRAMERATE}/1" \
    ! queue \
    ! rtpvrawdepay \
    ! queue \
    ! videoconvert \
    ! 8580sink output=${ICS8580_VIDEO_INPUT2_SD_HD_RGBHV} type=${ICS8580_VIDEO_TYPE_COMPOSITE} res=${ICS8580_VIDEO_RESOLUTION_PAL} channel=1 sync=false

exit
# Replace the 8580sink with this to display on local screen
    ! xvimagesink

# Non streaming version of pipeline
gst-launch-1.0 filesrc location="test.png" ! pngdec ! imagefreeze ! \
    videocrop top=${TOP} left=${LEFT} right=${RIGHT} bottom=${BOTTOM} ! \
    videoconvert ! \
    $TIMER \
    ! "video/x-raw, width=${TV_X}, height=${TV_Y}, pixel-aspect-ratio=1/1, framerate=${FRAMERATE}/1, format=(string)UYVY" \
    ! videoconvert \
    ! 8580sink output=${ICS8580_VIDEO_INPUT2_SD_HD_RGBHV} type=${ICS8580_VIDEO_TYPE_COMPOSITE} res=${ICS8580_VIDEO_RESOLUTION_PAL} channel=1 sync=false


