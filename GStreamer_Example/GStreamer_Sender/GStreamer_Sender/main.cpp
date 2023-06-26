#include <gst/gst.h>

int main(int argc, char* argv[]) {
    // GStreamer �ʱ�ȭ
    gst_init(&argc, &argv);

    // GStreamer ���������� ����
    GstElement* pipeline = gst_pipeline_new("sender_pipeline");
    GstElement* src = gst_element_factory_make("mfvideosrc", "src");
    GstElement* enc = gst_element_factory_make("x264enc", "enc");
    GstElement* pay = gst_element_factory_make("rtph264pay", "pay");
    GstElement* capsfilter = gst_element_factory_make("capsfilter", "filter");
    GstElement* sink = gst_element_factory_make("udpsink", "sink");

    // ���������ο� ��� �߰�
    gst_bin_add_many(GST_BIN(pipeline), src, enc, pay, capsfilter, sink, NULL);

    // ��� �� ��ũ ����
    gst_element_link_many(src, enc, pay, capsfilter, sink, NULL); 

    // �۽� �ּ� ���� (IP �ּҴ� 127.0.0.1�� ����, ��Ʈ ��ȣ 5001)
    g_object_set(sink, "host", "127.0.0.1", "port", 5001, NULL);

    // RTP ����(caps) ����
    GstCaps* caps = gst_caps_from_string("application/x-rtp, media=(string)video, payload=(int)96");
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // ��Ʈ�� ī�޶� ����
    g_object_set(src, "device-index", 0, NULL);  // 0�� ù ��° ī�޶� ��Ÿ���ϴ�.

    // x264enc ����� tune �Ӽ��� "zerolatency"�� ����
    g_object_set(enc, "tune", 0x00000004, NULL);

    // ���������� ����
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to start the pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // ���� ���� ����
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg;
    while ((msg = gst_bus_poll(bus, GST_MESSAGE_ANY, GST_CLOCK_TIME_NONE))) {
        GError* err;
        gchar* debug_info;
        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_error_free(err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            g_print("State changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
            break;
        }
        case GST_MESSAGE_BUFFERING: {
            gint percent = 0;
            gst_message_parse_buffering(msg, &percent);
            g_print("Buffering %d%%\n", percent);
            break;
        }
        default:
            break;
        }
        gst_message_unref(msg);
    }

    // ���������� ���� �� ����
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
