/**
 * @file play_video.cpp
 * @author Snagin-In Lee (tkddls0614@gmail.com)
 * @version 0.1
 * @date 2023-06-23
 * 
 * @copyright Copyright (c) 2023
 * 
 * Header file for play video with gstreamer 
 * 
 */

#ifndef PLAY_VIDEO_H
#define PLAY_VIDEO_H

#include <rtc/rtc.hpp>
#include <gst/gst.h>
#include <string>
#include <shared_mutex>

using std::shared_ptr;

static const rtc::SSRC ssrc = 42; // about h246 codec

class PlayVideo
{
private:
  GstElement *pipeline;
  GstElement *videosrc; // src elements
  GstElement *videoconvert, *videoscale, *queue, *x264enc, *rtph264pay; // other elements
  GstElement *appsink; // sink elements
  GstCaps *rawcaps, *h264caps;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  std::string video_device;
  int width, height;

  shared_ptr<rtc::Track> videoTrack;

  static GstFlowReturn sinkCallback(GstElement *sink,
                                    shared_ptr<rtc::Track> *videoTrack);

public:
    PlayVideo(); // default computer(Laptop) WebCam ex) Linux-/dev/video0
    PlayVideo(std::string video_dev);
    PlayVideo(std::string video_dev, int width, int height);
    ~PlayVideo();

    int play(); // Must run as thread

    void setVideoMediaTrack(shared_ptr<rtc::PeerConnection> pc);
};

#endif
