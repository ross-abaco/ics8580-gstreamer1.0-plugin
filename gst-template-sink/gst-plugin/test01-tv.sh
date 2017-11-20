#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*
#gst-launch-1.0 -v videotestsrc pattern=0 horizontal-speed=0 ! "video/x-raw, width=720, height=576" ! 8580sink output=2 type=1 res=2 channel=1 sync=true
gst-launch-1.0 -v videotestsrc pattern=0 horizontal-speed=0 ! "video/x-raw, width=720, height=576" ! 8580sink output=2 type=1 res=2 channel=1 sync=true
#gst-launch-1.0 -v videotestsrc pattern=11 ! "video/x-raw, width=720, height=576" ! 8580sink output=2 type=1 res=2 channel=1 sync=true

