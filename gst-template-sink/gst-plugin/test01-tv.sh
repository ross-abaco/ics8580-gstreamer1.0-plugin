#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*
#gst-launch-1.0 -v v4l2src ! "video/x-raw, width=640, height=480" ! videoscale ! videoconvert ! "video/x-raw, width=720, height=576" ! textoverlay text="CAM1" font-desc="Sans, 32" shaded-background=true ! 8580sink output=2 type=1 res=2 channel=1 sync=false
gst-launch-1.0 -v videotestsrc horizontal-speed=1 ! "video/x-raw, width=720, height=576" ! 8580sink output=2 type=1 res=2 channel=1 sync=true

