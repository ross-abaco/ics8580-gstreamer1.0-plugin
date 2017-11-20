/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2014 user <<user@hostname.org>>
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
 * SECTION:element-8580src
 *
 * FIXME:Describe 8580src here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! 8580src ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#include <string.h>
#include "gst8580src.h"
#include "gst8580capture.h"
#include "videotestsrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_8580src_debug);
#define GST_CAT_DEFAULT gst_8580src_debug 
#define GST_LEVEL_DEBUG            0

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NUM_BUFFERS,
  PROP_RESOLUTION,
  PROP_INPUT,
  PROP_TYPE,
  PROP_DMA_CHANNEL
};

static gboolean gst_video_test_src_create (GstBaseSrc *src, guint64 offset, guint size,
    GstBuffer **buf);
static gboolean gst_video_test_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps);
static GstCaps *gst_video_test_src_src_fixate (GstBaseSrc * bsrc,
    GstCaps * caps);

static gboolean gst_video_test_src_query (GstBaseSrc * bsrc, GstQuery * query);

static gboolean gst_video_test_src_decide_allocation (GstBaseSrc * bsrc,
    GstQuery * query);
static GstFlowReturn gst_video_test_src_fill (GstPushSrc * psrc,
    GstBuffer * buffer);
static gboolean gst_video_test_src_start (GstBaseSrc * basesrc);
static gboolean gst_video_test_src_stop (GstBaseSrc * basesrc);
static gboolean gst_video_test_src_event (GstBaseSrc *src, GstEvent *event);
/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,format=UYVY,framerate=[25/1, 60/1],width=[ 1, 32768 ],height=[ 1, 32768 ]")
    );

#define gst_8580src_parent_class parent_class
G_DEFINE_TYPE (Gst8580src, gst_8580src, GST_TYPE_PUSH_SRC);

static void gst_8580src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_8580src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* GObject vmethod implementations */

/* initialize the 8580src's class */
static void
gst_8580src_class_init (Gst8580srcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_8580src_set_property;
  gobject_class->get_property = gst_8580src_get_property;

  g_object_class_install_property (gobject_class, PROP_NUM_BUFFERS,
      g_param_spec_int ("num-buffers", "Number of buffers", "Number of buffers to output",
          -1,9999,-1, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_RESOLUTION,
      g_param_spec_int ("res", "Resolution", "Configure the 8580 input resolution",
          ICS8580_VIDEO_RESOLUTION_NONE, 
          ICS8580_MAX_NUM_VIDEO_RESOLUTIONS, 
          ICS8580_VIDEO_RESOLUTION_PAL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_INPUT,
      g_param_spec_int ("input", "Input", "Configure the 8580 input",
          ICS8580_VIDEO_INPUT0_NONE,
          ICS8580_MAX_NUM_VIDEO_INPUTS,
          ICS8580_VIDEO_INPUT1_SD_HD_RGBHV_DVI, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_TYPE,
      g_param_spec_int ("type", "Type", "Configure the 8580 input type",
          ICS8580_VIDEO_TYPE_NONE, 
          ICS8580_MAX_NUM_VIDEO_TYPES, 
          ICS8580_VIDEO_TYPE_COMPOSITE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_DMA_CHANNEL,
      g_param_spec_int ("channel", "Channel", "Configure the 8580 input DMA channel",
          DMA_CHANNEL_0, 
          DMA_CHANNEL_MAX, 
          DMA_CHANNEL_0, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "ICS8580 Source",
    "Video",
    "A video source for the Abaco Systems ICS-8580 video capture card.",
    "Ross Newman <ross.newman@abaco.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));

  gstbasesrc_class->set_caps = gst_video_test_src_setcaps;
  gstbasesrc_class->start = gst_video_test_src_start;
  gstbasesrc_class->stop = gst_video_test_src_stop;
  gstbasesrc_class->decide_allocation = gst_video_test_src_decide_allocation;
  gstbasesrc_class->query = gst_video_test_src_query;
  gstbasesrc_class->fixate = gst_video_test_src_src_fixate;
  gstpushsrc_class->fill = gst_video_test_src_fill;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static char username[] = "admin";
static char password[] = "admin";

static void
gst_8580src_init (Gst8580src * filter)
{
  Gst8580src *videotestsrc;
  videotestsrc = GST_8580SRC (filter);

  filter->timestamp_offset = 0;
  filter->num_buffers = -1;
  filter->input = 0;
  filter->type = 0;
  filter->res = 0;

  gst_debug_set_threshold_for_name ("8580src", GST_LEVEL_DEBUG); 

  /* we operate in time */
  gst_base_src_set_live (GST_BASE_SRC (filter), TRUE);
  gst_base_src_set_format (GST_BASE_SRC (filter), GST_FORMAT_TIME);
  gst_base_src_set_do_timestamp(GST_BASE_SRC (filter), TRUE);

  /* Initalise capture device */
  videotestsrc->args.video_in.videoInput=filter->input;
  videotestsrc->args.video_in.videoInputType=filter->type;
  videotestsrc->args.video_in.videoInputResolution=filter->res;
  videotestsrc->args.debug=0;
  videotestsrc->args.output=0;
  videotestsrc->args.streamId=1;
  videotestsrc->args.online=0;
  videotestsrc->args.username=username;
  videotestsrc->args.password=password;
}

static gboolean 
gst_video_test_src_create (GstBaseSrc *bsrc, guint64 offset, guint size,
                                 GstBuffer **buf)
{ 
  Gst8580src *videotestsrc;
  videotestsrc = GST_8580SRC (bsrc);
  Init8580Channels(videotestsrc->args);
  GST_LOG ("gst_video_test_src_create.\n");
  return TRUE;
}

static gboolean 
gst_video_test_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps)
{
  const GstStructure *structure;
  Gst8580src *src;

  GST_LOG ("gst_video_test_src_setcaps.\n");

  src = GST_8580SRC (bsrc);

  structure = gst_caps_get_structure (caps, 0);
  GST_LOG ("caps are %" GST_PTR_FORMAT, caps);

#if 0
  /* Attempt to automatically work out the capabilities NOT WORKING */
  /* Set defaults if no caps are provided */
  if (src->res == ICS8580_VIDEO_RESOLUTION_PAL)
  {
    resolutionTableLookup(ICS8580_VIDEO_RESOLUTION_PAL, &src->width, &src->height, &src->framerate);

    gst_video_info_set_format (&src->info, GST_VIDEO_FORMAT_YVYU, src->width, src->height);
    src->info.size = src->width * src->height * 2;
    caps = gst_caps_new_simple ("video/x-raw",
       "format", G_TYPE_STRING, "UYVY",
       "framerate", GST_TYPE_FRACTION, 25, 1,
       "pixel-aspect-ratio", G_TYPE_INT, 1,
       "width", G_TYPE_INT, src->width,
       "height", G_TYPE_INT, src->height,
       NULL);

    GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
  }

  if (src->res == ICS8580_VIDEO_RESOLUTION_NTSC)
  {
    resolutionTableLookup(ICS8580_VIDEO_RESOLUTION_NTSC, &src->width, &src->height, &src->framerate);
    gst_video_info_set_format (&src->info, GST_VIDEO_FORMAT_YVYU, src->width, src->height);
  }
#endif

  if (gst_structure_has_name (structure, "video/x-raw")) {
    /* we can use the parsing code */
    if (!gst_video_info_from_caps (&src->info, caps))
      goto parse_failed;

  } else {
    goto unsupported_caps;
  }


#if 0
  GST_LOG ("gst_video_test_src_setcaps (%d %d).\n",src->height,src->width);
  GST_LOG ("gst_video_test_src_setcaps (%d %d).\n",src->info.height,src->info.width);
  if (src->height != src->info.height)
	goto unsupported_caps;
  if (src->width != src->info.width)
	goto unsupported_caps;
#endif

  src->n_frames = 0;

  /* Update input settings from params */
  src->args.video_in.videoInput=src->input;
  src->args.video_in.videoInputType=src->type;
  src->args.video_in.videoInputResolution=src->res;
  src->args.channel=src->channel;
  GST_LOG ("gst_video_test_src_setcaps (%d %d %d).\n",src->input,src->type,src->res);

  if (videoOpen(src->args) != ICS8580_OK)
  {
    GST_ERROR ("videoOpen() failed to open channel!!.\n");
    return FALSE;
  }

  return TRUE;

  /* ERRORS */
parse_failed:
  {
    GST_LOG ("failed to parse caps");
    return FALSE;
  }
unsupported_caps:
  {
    GST_LOG ("unsupported caps: %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}

static GstCaps*
gst_video_test_src_src_fixate (GstBaseSrc * bsrc,
    GstCaps * caps)
{
  GstStructure *structure;
  Gst8580src *src;

  src = GST_8580SRC (bsrc);
  caps = gst_caps_make_writable (caps);

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "width", src->width);
  gst_structure_fixate_field_nearest_int (structure, "height", src->height);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", src->framerate, 1);

  if (Init8580(src->args) != ICS8580_OK)
  {
    GST_ERROR ("Init8580() failed to initalize!!.\n");
    return FALSE;
  }

  return GST_BASE_SRC_CLASS (parent_class)->fixate (bsrc, caps);;
}

static gboolean 
gst_video_test_src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  return  GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);;
}

static gboolean 
gst_video_test_src_decide_allocation (GstBaseSrc * bsrc,
    GstQuery * query)
{
  Gst8580src *src;
  GstBufferPool *pool;
  gboolean update;
  guint size, min, max;
  GstStructure *config;
  GstCaps *caps = NULL;

  src = GST_8580SRC (bsrc);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    GST_LOG ("gst_video_test_src_decide_allocation adjusting size. size=%d min=%d max=%d.\n", size, min, max);
    /* adjust size */
    size = MAX (size, src->info.size);
    update = TRUE;
  } else {
    pool = NULL;
    size = src->info.size;
    min = max = 0;
    update = FALSE;
  }

  /* no downstream pool, make our own */
  if (pool == NULL)
    pool = gst_video_buffer_pool_new ();

  config = gst_buffer_pool_get_config (pool);

  gst_query_parse_allocation (query, &caps, NULL);

  if (caps)
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

  gst_buffer_pool_set_config (pool, config);

  if (update)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);

  if (pool)
    gst_object_unref (pool);

  return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
}

static GstFlowReturn 
gst_video_test_src_fill (GstPushSrc * psrc, GstBuffer * buffer)
{
  Gst8580src *src;
  GstMapInfo info;

  src = GST_8580SRC (psrc);
  /* get WRITE access to the memory and fill with 0xff */
  gst_buffer_map (buffer, &info, GST_MAP_WRITE);

  /* g_print ("frame %d.\n", (int)src->n_frames++);*/
  GetFrame8580((char*)info.data); 
  src->n_frames++;

  gst_buffer_unmap (buffer, &info);
  /* g_print ("frame %d/%d.\n", (int)src->n_frames,src->num_buffers);*/
  if ((src->n_frames >= src->num_buffers) && (src->num_buffers!=-1)) return GST_FLOW_EOS;
  return GST_FLOW_OK;
}

static gboolean 
gst_video_test_src_start (GstBaseSrc * basesrc)
{
  return TRUE;
}

static gboolean 
gst_video_test_src_stop (GstBaseSrc * basesrc)
{
  Finalize8580Channels();
  videoClose();
  return TRUE;
}

static void
gst_8580src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gst8580src *filter = GST_8580SRC (object);

  switch (prop_id) {
    case PROP_NUM_BUFFERS:
      filter->num_buffers = g_value_get_int (value);
      break;
    case PROP_INPUT:
      filter->input = g_value_get_int (value);
      break;
    case PROP_TYPE:
      filter->type = g_value_get_int (value);
      break;
    case PROP_DMA_CHANNEL:
      filter->channel = g_value_get_int (value);
      break;
    case PROP_RESOLUTION:
      filter->res = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_8580src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gst8580src *filter = GST_8580SRC (object);

  switch (prop_id) {
    case PROP_NUM_BUFFERS:
      g_value_set_int (value, filter->num_buffers);
      break;
    case PROP_INPUT:
      g_value_set_int (value, filter->input);
      break;
    case PROP_TYPE:
      g_value_set_int (value, filter->type);
      break;
    case PROP_DMA_CHANNEL:
      g_value_set_int (value, filter->channel);
      break;
    case PROP_RESOLUTION:
      g_value_set_int (value, filter->res);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

#if 0
/* this function handles sink events */
static gboolean
gst_8580src_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
/*  Gst8580src *filter;

  filter = GST_8580SRC (parent);

*/
switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}
#endif

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
my_8580src_init (GstPlugin * my8580src)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template 8580src' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_8580src_debug, "8580src", 0, "Template 8580src");

  return gst_element_register (my8580src, "8580src", GST_RANK_NONE,
      GST_TYPE_8580SRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirst8580src"
#endif

/* gstreamer looks for this structure to register 8580srcs
 *
 * exchange the string 'Template 8580src' with your 8580src description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    8580src,
    "ICS-8580 plugin",
    my_8580src_init,
    VERSION,
    "LGPL",
    "Abaco Systems",
    "http://abaco.com/"
)
