#include <gst/gst.h>
#include <iostream>

#define TARGET_IP  "127.0.0.1"
#define TARGET_PORT 6000

#define WIDTH   640
#define HEIGHT  480

int main(int argc, char **argv) {

  gst_init(&argc, &argv);

  GstElement *pipeline;
  GstElement *screen_src; // src elements
  GstElement *videoconvert, *queue, *x264enc, *rtph264pay; // other elements
  GstElement *udpsink; // sink elements
  GstCaps *rawcaps, *h264caps;
  GstBus *bus;
  GstMessage *msg;

  pipeline = gst_pipeline_new("main-pipline");

  screen_src = gst_element_factory_make("avfvideosrc", "avfvideosrc");
  g_object_set(screen_src, "capture-screen", TRUE, NULL);

  videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  queue = gst_element_factory_make("queue", "queue");
  x264enc = gst_element_factory_make("x264enc", "x264enc");
  g_object_set(x264enc, "tune", "zerolatency", "bitrate", 1000, "key-int-max", 30, NULL);

  rawcaps = gst_caps_new_simple(
    "video/x-raw",
    "width",  G_TYPE_INT, WIDTH,
    "height", G_TYPE_INT, HEIGHT,
    NULL
  );

  h264caps = gst_caps_new_simple(
    "video/x-h264",
    "profile", G_TYPE_STRING, "constrained-baseline",
    NULL
  );

  rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
  g_object_set(rtph264pay, "pt", 96, "mtu", 1200, NULL);
  udpsink = gst_element_factory_make("udpsink", "udpsink");
  g_object_set(udpsink, "host", TARGET_IP, "port", TARGET_PORT, NULL);

  if (!pipeline || !screen_src || !videoconvert || !queue ||
      !x264enc || !rtph264pay || !udpsink || !rawcaps)
  {

    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many(GST_BIN(pipeline), screen_src, videoconvert, queue, x264enc, rtph264pay, udpsink, NULL);
  if (!gst_element_link_filtered(screen_src, videoconvert, rawcaps) ||
      !gst_element_link(videoconvert, queue) ||
      !gst_element_link(queue, x264enc) ||
      !gst_element_link_filtered(x264enc, rtph264pay, h264caps) ||
      !gst_element_link(rtph264pay, udpsink))
  {
    g_printerr("Elements could not be linked.\n");
    return -1;
  }

  gst_caps_unref(rawcaps);
  gst_caps_unref(h264caps);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if (msg != NULL) {
    gst_message_unref(msg);
  }

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
