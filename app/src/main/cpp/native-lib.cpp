#include <jni.h>
#include <string>


extern "C" {
#include <libavutil/imgutils.h>
#include <android/native_window_jni.h>
#include <zconf.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_bochao_ffmpeg_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(av_version_info());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1start(JNIEnv *env, jobject thiz, jstring path_,
                                                  jobject surface) {


    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    const char *path = env->GetStringUTFChars(path_, 0);
    // FFmpeg 视频绘制
    //初始化网络
    avformat_network_init();
    //总上下文
    AVFormatContext *formatContext = avformat_alloc_context();

    AVDictionary *opts = NULL;
    av_dict_set(&opts, "timeout", "3000000", 0);
    int ret = avformat_open_input(&formatContext, path, NULL, &opts);
    log(ret);
    if (ret) {
        return;
    }
    //视频流
    int vidio_stream_idx = -1;
    avformat_find_stream_info(formatContext, NULL);
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vidio_stream_idx = i;
            break;
        }
    }
    //视频流索引
    AVCodecParameters *codecpar = formatContext->streams[vidio_stream_idx]->codecpar;
    //解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    //获取解码器的上下文对象
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);

    //解码  获取yuv数据
    avcodec_open2(codecContext, dec, NULL);
    AVPacket *packet = av_packet_alloc();

    //从视频流中读取数据包
    SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height,
                                            codecContext->pix_fmt,
                                            codecContext->width, codecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0,
                                            0);

    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer outBuffer;
    while (av_read_frame(formatContext, packet) >= 0) {
        avcodec_send_packet(codecContext, packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        //接收的容器
        uint8_t *dst_data[4];
        //每一行的首地址
        int dst_linesize[4];
        av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                       AV_PIX_FMT_RGBA, 1);
        //绘制
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dst_data,
                  dst_linesize);

        //加锁
        ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

        //渲染
        uint8_t *firstWindow = static_cast<uint8_t *>(outBuffer.bits);
        //输入源
        uint8_t *src_data = dst_data[0];
        //拿到一行的多少个字节RGBA
        int destStride = outBuffer.stride * 4;
        int src_linesize = dst_linesize[0];
        for (int i = 0; i < outBuffer.height; ++i) {
            //内存拷贝进行渲染
            memccpy(firstWindow + i * destStride, src_data + i * src_linesize, destStride, NULL);
        }
        //解锁
        ANativeWindow_unlockAndPost(nativeWindow);
        usleep(1000 * 16);
        av_frame_free(&frame);

    }
    env->ReleaseStringUTFChars(path_, path);
}