#!/bin/bash
export frames=1000

while [ 1 ]
do
  gst-launch-1.0 8580src input=4 type=1 res=2 num-buffers=$frames ! "video/x-raw, width=720, height=576" ! timeoverlay ! textoverlay font-desc="Sans 26" text="PAL Video Input 4 ICS-8580" shaded-background=true auto-resize=false  ! xvimagesink
  gst-launch-1.0 8580src input=3 type=1 res=2 num-buffers=$frames ! "video/x-raw, width=720, height=576" ! timeoverlay ! textoverlay font-desc="Sans 26" text="PAL Video Input 3 ICS-8580" shaded-background=true auto-resize=false  ! xvimagesink
  gst-launch-1.0 8580src input=5 type=8 res=6 num-buffers=$frames ! "video/x-raw, width=1280, height=720" ! textoverlay font-desc="Sans 26" text="HD-SDI Video Input 5 ICS-8580" shaded-background=true auto-resize=false ! timeoverlay ! autovideosink
  sleep 2
done
