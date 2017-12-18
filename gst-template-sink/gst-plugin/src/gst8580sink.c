/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) YEAR AUTHOR_NAME AUTHOR_EMAIL
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-plugin
 *
 * FIXME:Describe plugin here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! plugin ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#include <string.h>
#include "gst8580sink.h"
#include "gst8580capture.h"
#include "videotestsrc.h"

/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_NUM_BUFFERS,
    PROP_RESOLUTION,
    PROP_OUTPUT,
    PROP_TYPE,
    PROP_DMA_CHANNEL
};

static gboolean gst_8580sink_start(GstBaseSink * src);
static gboolean gst_8580sink_setcaps(GstBaseSink * bsink, GstCaps * caps);
static GstCaps *gst_8580sink_fixate(GstBaseSink * bsrc, GstCaps * caps);
static gboolean gst_8580sink_stop(GstBaseSink * bsink);
static GstFlowReturn gst_8580sink_render(GstBaseSink * sink,
                                         GstBuffer * buffer);

/* the capabilities of the outputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS
                                                                   ("video/x-raw,format=UYVY,framerate=[0/1, 60/1],width=[ 1, 32768 ],height=[ 1, 32768 ]")
    );

#define gst_8580sink_parent_class parent_class
G_DEFINE_TYPE(Gst8580sink, gst_8580sink, GST_TYPE_VIDEO_SINK);

static void gst_8580sink_set_property(GObject * object, guint prop_id,
                                      const GValue * value,
                                      GParamSpec * pspec);
static void gst_8580sink_get_property(GObject * object, guint prop_id,
                                      GValue * value, GParamSpec * pspec);

static gboolean gst_8580sink_event(GstPad * pad, GstObject * parent,
                                   GstEvent * event);
static GstFlowReturn gst_8580sink_chain(GstPad * pad, GstObject * parent,
                                        GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void gst_8580sink_class_init(Gst8580sinkClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstBaseSinkClass *gstbasesink_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;
    gstbasesink_class = (GstBaseSinkClass *) klass;

    gobject_class->set_property = gst_8580sink_set_property;
    gobject_class->get_property = gst_8580sink_get_property;

    g_object_class_install_property(gobject_class, PROP_NUM_BUFFERS,
                                    g_param_spec_int("num-buffers",
                                                     "Number of buffers",
                                                     "Number of buffers to output",
                                                     -1, 9999, -1,
                                                     G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_RESOLUTION,
                                    g_param_spec_int("res", "Resolution",
                                                     "Configure the 8580 output resolution",
                                                     ICS8580_VIDEO_RESOLUTION_NONE,
                                                     ICS8580_MAX_NUM_VIDEO_RESOLUTIONS,
                                                     ICS8580_VIDEO_RESOLUTION_PAL,
                                                     G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_OUTPUT,
                                    g_param_spec_int("output", "output",
                                                     "Configure the 8580 output",
                                                     ICS8580_VIDEO_OUTPUT1_RGB,
                                                     ICS8580_VIDEO_OUTPUT8_PCIe_3,
                                                     ICS8580_VIDEO_OUTPUT2_TV,
                                                     G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_TYPE,
                                    g_param_spec_int("type", "Type",
                                                     "Configure the 8580 output type",
                                                     ICS8580_VIDEO_TYPE_NONE,
                                                     ICS8580_MAX_NUM_VIDEO_TYPES,
                                                     ICS8580_VIDEO_TYPE_COMPOSITE,
                                                     G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_DMA_CHANNEL,
                                    g_param_spec_int("channel", "Channel",
                                                     "Configure the 8580 output DMA channel",
                                                     DMA_CHANNEL_0,
                                                     DMA_CHANNEL_MAX,
                                                     DMA_CHANNEL_0,
                                                     G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "ICS8580 Sink",
                                         "Video",
                                         "A video sink for the Abaco Systems ICS-8580 video capture card.",
                                         "Ross Newman <ross.newman@abaco.com>");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get
                                       (&sink_factory));

    gstbasesink_class->start = gst_8580sink_start;
    gstbasesink_class->set_caps = gst_8580sink_setcaps;
    gstbasesink_class->fixate = gst_8580sink_fixate;
    gstbasesink_class->stop = gst_8580sink_stop;
    gstbasesink_class->render = gst_8580sink_render;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static char username[] = "admin";
static char password[] = "admin";

static void gst_8580sink_init(Gst8580sink * filter)
{
    Gst8580sink *videotestsink;
    videotestsink = GST_8580SINK(filter);

    filter->timestamp_offset = 0;
    filter->num_buffers = -1;
    filter->output = 0;
    filter->type = 0;
    filter->res = 0;

    gst_debug_set_threshold_for_name("8580sink", GST_LEVEL_DEBUG);

    filter->sinkpad =
        gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(filter->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_8580sink_event));
    gst_pad_set_chain_function(filter->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_8580sink_chain));

    /* Initalise capture device */
    videotestsink->args.video_out.videoOutput = filter->output;
    videotestsink->args.video_out.videoOutputType = filter->type;
    videotestsink->args.video_out.videoOutputResolution = filter->res;
    videotestsink->args.debug = 0;
    videotestsink->args.output = 0;
    videotestsink->args.streamId = 1;
    videotestsink->args.online = 0;
    videotestsink->args.username = username;
    videotestsink->args.password = password;

    GST_LOG("Init8580.\n");
    Init8580(videotestsink->args);
}

static gboolean gst_8580sink_start(GstBaseSink * bsink)
{
    GST_LOG("gst_8580sink_create.\n");
    return TRUE;
}

static gboolean gst_8580sink_setcaps(GstBaseSink * bsink, GstCaps * caps)
{
    const GstStructure *structure;
    Gst8580sink *sink;

    GST_LOG("gst_video_test_src_setcaps.\n");

    sink = GST_8580SINK(bsink);

    structure = gst_caps_get_structure(caps, 0);
    GST_LOG("caps are %" GST_PTR_FORMAT, caps);

    if (gst_structure_has_name(structure, "video/x-raw")) {
        /* we can use the parsing code */
        if (!gst_video_info_from_caps(&sink->info, caps))
            goto parse_failed;

    } else {
        goto unsupported_caps;
    }

    sink->n_frames = 0;

    /* Update output settings from params */
    sink->args.video_out.videoOutput = sink->output;
    sink->args.video_out.videoOutputType = sink->type;
    sink->args.video_out.videoOutputResolution = sink->res;
    sink->args.channel = sink->channel;

    GST_LOG("gst_video_test_src_setcaps (%d %d %d).\n", sink->output,
            sink->type, sink->res);

    GST_LOG("Init8580channels...\n");
    if (Init8580Channels(sink->args) != ICS8580_OK) {
        GST_ERROR("Init8580Channels() failed to initalize!!.\n");
        return FALSE;
    }


    if (videoOpen(sink->args) != ICS8580_OK) {
        GST_ERROR("videoOpen() failed to open channel!!.\n");
        return FALSE;
    }

    return TRUE;

    /* ERRORS */
  parse_failed:
    {
        GST_LOG("failed to parse caps");
        return FALSE;
    }
  unsupported_caps:
    {
        GST_LOG("unsupported caps: %" GST_PTR_FORMAT, caps);
        return FALSE;
    }
}

static GstCaps *gst_8580sink_fixate(GstBaseSink * bsink, GstCaps * caps)
{
    GstStructure *structure;
    Gst8580sink *sink;

    sink = GST_8580SINK(bsink);
    caps = gst_caps_make_writable(caps);

    structure = gst_caps_get_structure(caps, 0);

    gst_structure_fixate_field_nearest_int(structure, "width",
                                           sink->width);
    gst_structure_fixate_field_nearest_int(structure, "height",
                                           sink->height);
    gst_structure_fixate_field_nearest_fraction(structure, "framerate",
                                                sink->framerate, 1);

    return GST_BASE_SINK_CLASS(parent_class)->fixate(bsink, caps);;
}

GstFlowReturn gst_8580sink_render(GstBaseSink * bsink, GstBuffer * buffer)
{
    Gst8580sink *sink;
    GstMapInfo info;
    int interlaced = 0;

    sink = GST_8580SINK(bsink);
    /* get WRITE access to the memory and fill with 0xff */
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    if (sink->type == ICS8580_VIDEO_RESOLUTION_NTSC) interlaced = 1;
    if (sink->type == ICS8580_VIDEO_RESOLUTION_PAL) interlaced = 1;
    /* g_print ("frame %d.\n", (int)src->n_frames++); */
    GetPut8580((char *) info.data, interlaced);

/*  printf ("frame %d.\n", (int)sink->n_frames++); */

    gst_buffer_unmap(buffer, &info);

    return GST_FLOW_OK;
}

static gboolean gst_8580sink_stop(GstBaseSink * bsink)
{
    Finalize8580Channels();
    videoClose();
    return TRUE;
}

static void
gst_8580sink_set_property(GObject * object, guint prop_id,
                          const GValue * value, GParamSpec * pspec)
{
    Gst8580sink *filter = GST_8580SINK(object);

    switch (prop_id) {
    case PROP_NUM_BUFFERS:
        filter->num_buffers = g_value_get_int(value);
        break;
    case PROP_OUTPUT:
        filter->output = g_value_get_int(value);
        break;
    case PROP_TYPE:
        filter->type = g_value_get_int(value);
        break;
    case PROP_DMA_CHANNEL:
        filter->channel = g_value_get_int(value);
        break;
    case PROP_RESOLUTION:
        filter->res = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_8580sink_get_property(GObject * object, guint prop_id,
                          GValue * value, GParamSpec * pspec)
{
    Gst8580sink *filter = GST_8580SINK(object);

    switch (prop_id) {
    case PROP_NUM_BUFFERS:
        g_value_set_int(value, filter->num_buffers);
        break;
    case PROP_OUTPUT:
        g_value_set_int(value, filter->output);
        break;
    case PROP_TYPE:
        g_value_set_int(value, filter->type);
        break;
    case PROP_DMA_CHANNEL:
        g_value_set_int(value, filter->channel);
        break;
    case PROP_RESOLUTION:
        g_value_set_int(value, filter->res);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_8580sink_event(GstPad * pad, GstObject * parent, GstEvent * event)
{
    gboolean ret;

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CAPS:
        {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);
            /* do something with the caps */

            /* and forward */
            ret = gst_pad_event_default(pad, parent, event);
            break;
        }
    default:
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_8580sink_chain(GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    Gst8580sink *filter;

    filter = GST_8580SINK(parent);

    GST_LOG("Running %d\n", filter->n_frames++);
/*  printf ("frame %d.\n", (int)filter->n_frames++); */

    /* just push out the incoming buffer without touching it */
    return gst_pad_push(filter->sinkpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean my_8580sink_init(GstPlugin * my8580sink)
{
    /* debug category for fltering log messages
     *
     * exchange the string 'Template plugin' with your description
     */
    GST_DEBUG_CATEGORY_INIT(gst_8580sink_debug, "8580sink", 0,
                            "Template 8580sink");

    return gst_element_register(my8580sink, "8580sink", GST_RANK_NONE,
                                GST_TYPE_8580SINK);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirst8580sink"
#endif

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template 8580sink' with your plugin description
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  8580 sink,
                  "ICS-8580 plugin",
                  my_8580sink_init,
                  VERSION, "LGPL", "Abaco Systems", "http://abaco.com/")

