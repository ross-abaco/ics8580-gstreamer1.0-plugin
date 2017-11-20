#gst-launch-1.0 -e 8580src input=3 type=1 res=2 channel=1 ! "video/x-raw, width=720, height=576"  ! videoscale !\
#	video/x-raw, framerate=10/1, width=200, height=150 ! videomixer name=mix ! \
#	videoconvert ! xvimagesink videotestsrc ! \
#	video/x-raw, framerate=10/1, width=640, height=360 ! mix.

if [ 0 -eq 1 ]; then
 gst-launch-1.0 -e videomixer name=mix ! videoconvert ! xvimagesink \
   videotestsrc pattern="snow" ! video/x-raw, framerate=25/1, width=720, height=576 ! \
     videobox border-alpha=0 top=0 left=0 ! mix. \
   videotestsrc pattern="snow" ! video/x-raw, framerate=25/1, width=720, height=576 ! \
     textoverlay font-desc="Sans 24" text="CAM2" valignment=top halignment=left shaded-background=true ! \
     videobox border-alpha=0 top=0 left=-720 ! mix. 
else
#   8580src input=3 type=1 res=2 channel=1 ! video/x-raw, framerate=25/1, width=720, height=576 ! \

 gst-launch-1.0 -e videomixer name=mix ! queue ! videoconvert ! xvimagesink \
   8580src input=4 type=1 res=2 channel=0 ! video/x-raw, framerate=25/1, width=720, height=576 ! \
     textoverlay font-desc="Sans 24" text="CAM1" valignment=top halignment=left shaded-background=true ! \
     videobox border-alpha=0 top=0 left=0 ! mix. \
   8580src input=3 type=1 res=2 channel=1 ! video/x-raw, framerate=25/1, width=720, height=576 ! \
     textoverlay font-desc="Sans 24" text="CAM2" valignment=top halignment=left shaded-background=true ! \
     videobox border-alpha=0 top=0 left=-720 ! mix. --gst-debug-level=0

exit
fi
