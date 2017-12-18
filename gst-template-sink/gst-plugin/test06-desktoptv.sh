#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*

gst-launch-1.0 ximagesrc ! video/x-raw,framerate=5/1 ! videoconvert ! videoscale ! videorate ! video/x-raw,width=720, height=576 ! 8580sink output=2 type=1 res=2 channel=1 sync=true

