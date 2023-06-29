#include <gst/gst.h>
#include "recvMain.h"

// Create GStreamer loop
GMainLoop* gReceiverLoop;

// define callback function
static gboolean on_video_bus_message(GstBus* bus, GstMessage* const message, gpointer user_data) {
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ELEMENT: {
        if (gst_message_has_name(message, "jitterbuffer")) {
            const GstStructure* stats = gst_message_get_structure(message);
            if (stats != NULL && gst_structure_has_field(stats, "jitter")) {
                guint64 jitter;
                gst_structure_get_uint64(stats, "jitter", &jitter);
                g_print("Receiver : Jitter: %" G_GUINT64_FORMAT " ns\n", jitter);
            }
        }
        break;
    }
    default:
        break;
    }
    return FALSE;
}

// define callback function
static gboolean on_audio_bus_message(GstBus* bus, GstMessage* const message, gpointer user_data) {
    g_print("Receiver: Callback!\n");
    return FALSE;
}

int recvMain()
{
    g_printerr("Receiver : Start recvMain thread\n");

    // Create GStreamer pipline for video
    GstElement* videoPipeline = gst_pipeline_new("videoReceiver_pipeline");

    // Create GStreamer pipline elements for video
    GstElement* videoSrc = gst_element_factory_make("udpsrc", "videoSrc");
    GstElement* videoCapsfilter = gst_element_factory_make("capsfilter", "videoCapsfilter");
    GstElement* videoJitterbuffer = gst_element_factory_make("rtpjitterbuffer", "videoJitterbuffer");
    GstElement* videoDepay = gst_element_factory_make("rtph264depay", "videoDepay");
    GstElement* videoDec = gst_element_factory_make("avdec_h264", "videoDec");
    GstElement* videoSink = gst_element_factory_make("autovideosink", "videoSink");

    // Create GStreamer pipline for audio
    GstElement* audioPipeline = gst_pipeline_new("audioReceiver_pipeline");

    // Create GStreamer pipline elements for audio
    GstElement* audioSrc = gst_element_factory_make("udpsrc", "audioSrc");
    GstElement* audioCapsfilter = gst_element_factory_make("capsfilter", "audioCapsfilter");
    GstElement* audioJitterbuffer = gst_element_factory_make("rtpjitterbuffer", "audioJitterbuffer");
    GstElement* audioDec = gst_element_factory_make("opusdec", "audioDec");
    GstElement* audioDepay = gst_element_factory_make("rtpopusdepay", "audioDepay");
    GstElement* audioConv = gst_element_factory_make("audioconvert", "audioConv");
    GstElement* audioSink = gst_element_factory_make("autoaudiosink", "audioSink");

    // set up video bus and callback function for jitter statistics
    GstBus* videoBus = gst_element_get_bus(videoPipeline);
    gst_bus_add_watch(videoBus, on_video_bus_message, NULL);
    gst_object_unref(videoBus);

    // set up audio bus and callback function to check whether an audio packet has been received
    GstBus* audioBus = gst_element_get_bus(audioPipeline);
    gst_bus_add_watch(audioBus, on_audio_bus_message, NULL);
    gst_object_unref(audioBus);

    // Add element to pipeline
    gst_bin_add_many(GST_BIN(videoPipeline), videoSrc, videoCapsfilter, videoJitterbuffer, videoDepay, videoDec, videoSink, NULL);
    gst_bin_add_many(GST_BIN(audioPipeline), audioSrc, audioCapsfilter, audioJitterbuffer, audioDec, audioDepay, audioConv, audioSink, NULL);

    // linking elements
    gst_element_link_many(videoSrc, videoCapsfilter, videoJitterbuffer, videoDepay, videoDec, videoSink, NULL);
    gst_element_link_many(audioSrc, audioCapsfilter, audioJitterbuffer, audioDepay, audioDec, audioConv, audioSink, NULL);

    // Receive port setting
    g_object_set(videoSrc, "port", 5001, NULL);
    g_object_set(audioSrc, "port", 5002, NULL);

    // RTP video format (caps) settings
    GstCaps* videoCaps = gst_caps_from_string("application/x-rtp, media=(string)video, payload=(int)96");
    g_object_set(G_OBJECT(videoCapsfilter), "caps", videoCaps, NULL);
    gst_caps_unref(videoCaps);

    // RTP audio format (caps) settings
    GstCaps* audioCaps = gst_caps_from_string("application/x-rtp, media=(string)audio, encoding-name=OPUS,  payload=(int)96");
    g_object_set(G_OBJECT(audioCapsfilter), "caps", audioCaps, NULL);
    gst_caps_unref(audioCaps);

    // Set RTP jitter-buffer latency
    g_object_set(videoJitterbuffer, "latency", 0, "do-lost", TRUE, NULL);
    g_object_set(audioJitterbuffer, "latency", 500, "do-lost", TRUE, NULL);

    // Pipeline execution
    GstStateChangeReturn videoRet = gst_element_set_state(videoPipeline, GST_STATE_PLAYING);
    GstStateChangeReturn audioRet = gst_element_set_state(audioPipeline, GST_STATE_PLAYING);
    if (videoRet == GST_STATE_CHANGE_FAILURE || audioRet == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to start the pipeline.\n");
        gst_object_unref(videoPipeline);
        return -1;
    }

    g_printerr("Receiver : Run receiver main loop!\n");

    // Start the main loop
    gReceiverLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(gReceiverLoop);

    g_printerr("Receiver : Stop receiver main loop!\n");

    // Release the pipeline
    gst_element_set_state(videoPipeline, GST_STATE_NULL);
    gst_element_set_state(audioPipeline, GST_STATE_NULL);
    gst_object_unref(videoPipeline);
    gst_object_unref(audioPipeline);
    g_main_loop_unref(gReceiverLoop);

    return 0;
}

void quiteReceiverMain()
{
    g_main_loop_quit(gReceiverLoop);
}