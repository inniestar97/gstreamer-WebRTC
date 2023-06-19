#include <gst/gst.h>
#include <iostream>

#define TARGET_IP  "127.0.0.1"
#define TARGET_PORT 5000

#define WIDTH   1240
#define HEIGHT  1080

int main(int argc, char **argv) {

  gst_init(&argc, &argv);

  GstElement *pipeline;
  GstElement *videosrc; // src elements
  GstElement *videoconvert, *queue, *x264enc, *rtph264pay; // other elements
  // GstElement *rawCapsFilter, *h264CapsFilter; // capsfilter
  GstElement *udpsink; // sink elements
  GstCaps *rawcaps, *h264caps;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  pipeline = gst_pipeline_new("main-pipline");

  videosrc = gst_element_factory_make("v4l2src", "v4l2src");
  g_object_set(G_OBJECT(videosrc), "device", "/dev/video0", NULL);

  videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  queue = gst_element_factory_make("queue", "queue");
  x264enc = gst_element_factory_make("x264enc", "x264enc");
  g_object_set(x264enc, "tune", /* zerolatency */0x00000004, "bitrate", 1000, "key-int-max", 30, NULL);

  // rawcaps = gst_caps_new_simple(
  //   "video/x-raw",
  //   "width",      G_TYPE_INT,         WIDTH,
  //   "height",     G_TYPE_INT,         HEIGHT,
  //   NULL
  // );
  // rawCapsFilter = gst_element_factory_make("capsfilter", "raw-capsfilter");
  // g_object_set(rawCapsFilter, "caps", rawcaps, NULL);

  h264caps = gst_caps_new_simple(
    "video/x-h264",
    "profile", G_TYPE_STRING, "constrained-baseline",
    NULL
  );
  // h264CapsFilter = gst_element_factory_make("capsfilter", "h264-capsfilter");
  // g_object_set(h264CapsFilter, "caps", h264caps, NULL);

  rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
  g_object_set(rtph264pay, "pt", 96, "mtu", 1200, NULL);
  udpsink = gst_element_factory_make("udpsink", "udpsink");
  g_object_set(udpsink, "host", TARGET_IP, "port", TARGET_PORT, NULL);

  if (!pipeline || !videosrc || !videoconvert || !queue ||
      !x264enc || !rtph264pay || !udpsink || !rawcaps)
  // if (!pipeline || !videosrc || !rawCapsFilter || !videoconvert || !queue ||
  //     !x264enc || !h264CapsFilter || !rtph264pay || !udpsink || !rawcaps)
  {

    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many(GST_BIN(pipeline), videosrc, videoconvert, queue, x264enc, rtph264pay, udpsink, NULL);
  // gst_bin_add_many(GST_BIN(pipeline), videosrc, rawCapsFilter, videoconvert,
  //                 queue, x264enc, h264CapsFilter, rtph264pay, udpsink, NULL);

  if (!gst_element_link(videosrc, videoconvert) ||
      !gst_element_link(videoconvert, queue) ||
      !gst_element_link(queue, x264enc) ||
      !gst_element_link_filtered(x264enc, rtph264pay, h264caps) ||
      !gst_element_link(rtph264pay, udpsink))
  // if (!gst_element_link_many(videosrc, rawCapsFilter, videoconvert, queue,
  //                            x264enc, h264CapsFilter, rtph264pay, udpsink, NULL))
  {
    g_printerr("Elements could not be linked.\n");
    return -1;
  }

  // gst_caps_unref(rawcaps);
  gst_caps_unref(h264caps);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  
  bus = gst_element_get_bus(pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        terminate = TRUE;
        break;
      case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        terminate = TRUE;
        break;
      case GST_MESSAGE_STATE_CHANGED:
        /* We are only interested in state-changed messages from the pipeline */
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
          GstState old_state, new_state, pending_state;
          gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
          g_print ("Pipeline state changed from %s to %s:\n",
              gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
        }
        break;
      default:
        /* We should not reach here */
        g_printerr ("Unexpected message received.\n");
        break;
      }
      gst_message_unref(msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  
  return 0;
}
