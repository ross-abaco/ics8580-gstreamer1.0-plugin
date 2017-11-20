#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*

gst-launch-1.0 multifilesrc location="test.png" index=0 caps="image/png,framerate=\(fraction\)12/1" ! pngdec !  videoscale ! videoconvert ! videorate ! "video/x-raw, width=720, height=576" ! 8580sink output=2 type=1 res=2 channel=1 sync=false


