#include <gst/gst.h>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#define TARGET_IP  "127.0.0.1"
#define TARGET_PORT 5000

#define WIDTH   1240
#define HEIGHT  1080

GstFlowReturn appsinkCallback(GstElement *sink, int *client_fd) {
  GstSample *sample;
  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if (!sample) {
    std::cout << "NOSAMPLE" << std::endl;
    return GST_FLOW_ERROR;
  }
  GstBuffer *buffer = gst_sample_get_buffer(sample);

  std::vector<uint8_t> msgToOther;

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);
  std::copy(map.data, map.data + map.size, std::back_inserter(msgToOther));
  gst_buffer_unmap(buffer, &map);

  /* ---------- send msgToOther using socket ----------- */
  /* implement in here */
  // ::send(*client_fd, reinterpret_cast<const char*>(msgToOther.data()), msgToOther.size(), 0);

  /* debugging */
  auto data_len = msgToOther.size() > 16 * 10 ? 16 * 10 : msgToOther.size();
  std::cout << std::hex;
  for (size_t i = 0; i < 16 * 10; i++) {
    std::cout << std::setw(2) << std::setfill('0') << (int) msgToOther[i] << " ";
    if ((i + 1) % 16 == 0) {
      std::cout << std::endl;
    }
  }
  std::cout << std::dec;
  if (msgToOther.size() > 16 * 10) {
    std::cout << "..." << std::endl;
  }
  /* ---------- send msgToOther using socket ----------- */

  gst_sample_unref(sample);

  return GST_FLOW_OK;
}

int main(int argc, char **argv) {

  gst_init(&argc, &argv);

  GstElement *pipeline;
  GstElement *videosrc; // src elements
  GstElement *videoconvert, *videoscale, *queue, *x264enc, *rtph264pay; // other elements
  // GstElement *rawCapsFilter, *h264CapsFilter; // capsfilter
  GstElement *appsink; // sink elements
  GstCaps *rawcaps, *h264caps;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  // int status, valread, client_fd;
  // struct sockaddr_in server_addr;
  // if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
  //   std::cout << "Socket creation error" << std::endl;
  //   return -1;
  // }

  // server_addr.sin_family = AF_INET;
  // server_addr.sin_port = htons(TARGET_PORT);
  // if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
  //   std::cout << "Invalid address/ Address not supported" << std::endl;
  //   return -1;
  // }

  // if ((status = connect(client_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0) {
  //   std::cout << "Connect failed" << std::endl;
  //   return -1;
  // }

  pipeline = gst_pipeline_new("main-pipline");

#ifdef __LINUX__
  videosrc = gst_element_factory_make("v4l2src", "v4l2src");
  g_object_set(G_OBJECT(videosrc), "device", "/dev/video0", NULL);
#elif __APPLE__
  videosrc = gst_element_factory_make("avfvideosrc", "avfvideosrc") ;
#endif

  videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  videoscale = gst_element_factory_make("videoscale", "videoscale");
  queue = gst_element_factory_make("queue", "queue");
  x264enc = gst_element_factory_make("x264enc", "x264enc");
  g_object_set(x264enc, "tune", /* zerolatency */0x00000004, "bitrate", 1000, "key-int-max", 30, NULL);

  rawcaps = gst_caps_new_simple(
    "video/x-raw",
    "width",      G_TYPE_INT,         WIDTH,
    "height",     G_TYPE_INT,         HEIGHT,
    NULL
  );

  h264caps = gst_caps_new_simple(
    "video/x-h264",
    "profile", G_TYPE_STRING, "constrained-baseline",
    NULL
  );

  rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
  g_object_set(rtph264pay, "pt", 96, "mtu", 1200, NULL);
  appsink = gst_element_factory_make("appsink", "appsink");
  // g_object_set(udpsink, "host", TARGET_IP, "port", TARGET_PORT, NULL);
  g_object_set(appsink, "emit-signals", TRUE, NULL);
  // g_signal_connect(appsink, "new-sample", G_CALLBACK(appsinkCallback), &client_fd);
  g_signal_connect(appsink, "new-sample", G_CALLBACK(appsinkCallback), NULL);

  if (!pipeline || !videosrc || !videoconvert || !videoscale || !rawcaps || !queue ||
      !x264enc || !rtph264pay || !appsink)
  {

    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many(GST_BIN(pipeline), videosrc, videoconvert, videoscale, queue, x264enc, rtph264pay, appsink, NULL);

  if (!gst_element_link_many(videosrc, videoconvert, videoscale, NULL) ||
      !gst_element_link_filtered(videoscale, queue, rawcaps) ||
      !gst_element_link(queue, x264enc) ||
      !gst_element_link_filtered(x264enc, rtph264pay, h264caps) ||
      !gst_element_link(rtph264pay, appsink))
  {
    g_printerr("Elements could not be linked.\n");
    return -1;
  }

  gst_caps_unref(rawcaps);
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
