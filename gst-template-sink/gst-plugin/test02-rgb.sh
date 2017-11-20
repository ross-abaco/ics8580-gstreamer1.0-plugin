#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*
gst-launch-1.0 -v videotestsrc pattern=0 ! "video/x-raw, width=640, height=480" ! 8580sink output=1 type=4 res=13 channel=1 sync=true

