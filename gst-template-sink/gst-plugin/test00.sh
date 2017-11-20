#!/bin/bash
export GST_DEBUG=3,gst_8580sink_debug:*
gst-launch-1.0 -v videotestsrc pattern=0 ! "video/x-raw, width=720, height=576" ! xvimagesink

