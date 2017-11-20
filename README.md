# ics8580-gstreamer1.0-plugin
Sink and source plugin for the Abaco Systems ICS8580 board.

Thi plugin is tested under Gstreamer 1.2.4 in Ubuntu x64 14.04 LTS.

# Dependancies
On Ubuntu Linux the following dependancies are required to build the Gstreamer plugin:

## Hardware
Requires the ISC-8580 XMC card, reffer to manual for installation instructions.

## Software
Please install the ICS-8580 SDK Version 3.x required for complation of the plugin.

```
sudo apt-get install libgstreamer1.0-dev autoconf automake libtool
```

# Building

Configure the build environment.
```
cd ./gst-plugin-[src|sink]/gst-plugin
./autogen.sh
```

# Installation
```
cd ./gst-plugin-[src|sink]/gst-plugin
./sudo make install
gst-inspect-1.0 8580s[src|sink]
```

# Links
[Abaco Systems ICS-8580](https://www.abaco.com/products/ics-8580-video-compression-board)
