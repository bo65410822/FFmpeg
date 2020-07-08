#include <jni.h>
#include <string>
#include <android/log.h>

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lzb", __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN,  "lzb", __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "lzb", __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  "lzb", __VA_ARGS__)
extern "C" {
#include <libavutil/imgutils.h>
#include <android/native_window_jni.h>
#include <zconf.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
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
    // 记录结果
    int result;
    // R1 Java String -> C String
    const char *path = env->GetStringUTFChars(path_, 0);
    // 注册 FFmpeg 组件
    avformat_network_init();
    // R2 初始化 AVFormatContext 上下文
    AVFormatContext *format_context = avformat_alloc_context();
    // 打开视频文件
    result = avformat_open_input(&format_context, path, NULL, NULL);
    if (result < 0) {
        ALOGE("Player Error : Can not open video file");
        return;
    }
    // 查找视频文件的流信息
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        ALOGE("Player Error : Can not find video file stream info");
        return;
    }
    // 查找视频编码器
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        // 匹配视频流
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
    }
    // 没找到视频流
    if (video_stream_index == -1) {
        ALOGE("Player Error : Can not find video stream");
        return;
    }
    // 初始化视频编码器上下文
    AVCodecContext *video_codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(video_codec_context,
                                  format_context->streams[video_stream_index]->codecpar);
    // 初始化视频编码器
    AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
    if (video_codec == NULL) {
        ALOGE("Player Error : Can not find video codec");
        return;
    }
    // R3 打开视频解码器
    result = avcodec_open2(video_codec_context, video_codec, NULL);
    if (result < 0) {
        ALOGE("Player Error : Can not find video stream");
        return;
    }
    // 获取视频的宽高
    int videoWidth = video_codec_context->width;
    int videoHeight = video_codec_context->height;
    // R4 初始化 Native Window 用于播放视频
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == NULL) {
        ALOGE("Player Error : Can not create native window");
        return;
    }
    // 通过设置宽高限制缓冲区中的像素数量，而非屏幕的物理显示尺寸。
    // 如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
    result = ANativeWindow_setBuffersGeometry(native_window, videoWidth, videoHeight,
                                              WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        ALOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(native_window);
        return;
    }
    // 定义绘图缓冲区
    ANativeWindow_Buffer window_buffer;
    // 声明数据容器 有3个
    // R5 解码前数据容器 Packet 编码数据
    AVPacket *packet = av_packet_alloc();
    // R6 解码后数据容器 Frame 像素数据 不能直接播放像素数据 还要转换
    AVFrame *frame = av_frame_alloc();
    // R7 转换后数据容器 这里面的数据可以用于播放
    AVFrame *rgba_frame = av_frame_alloc();
    // 数据格式转换准备
    // 输出 Buffer
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    // R8 申请 Buffer 内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, out_buffer, AV_PIX_FMT_RGBA,
                         videoWidth, videoHeight, 1);
    // R9 数据格式转换上下文
    struct SwsContext *data_convert_context = sws_getContext(
            videoWidth, videoHeight, video_codec_context->pix_fmt,
            videoWidth, videoHeight, AV_PIX_FMT_RGBA,
            SWS_BICUBIC, NULL, NULL, NULL);
    // 开始读取帧
    while (av_read_frame(format_context, packet) >= 0) {
        // 匹配视频流
        if (packet->stream_index == video_stream_index) {
            // 解码
            result = avcodec_send_packet(video_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                ALOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(video_codec_context, frame);
            if (result < 0 && result != AVERROR_EOF) {
                ALOGE("Player Error : codec step 2 fail");
                return;
            }
            // 数据格式转换
            result = sws_scale(data_convert_context, (const uint8_t *const *) frame->data,
                               frame->linesize, 0, videoHeight, rgba_frame->data,
                               rgba_frame->linesize);
            if (result < 0) {
                ALOGE("Player Error : data convert fail");
                return;
            }
            // 播放
            result = ANativeWindow_lock(native_window, &window_buffer, NULL);
            if (result < 0) {
                ALOGE("Player Error : Can not lock native window");
            } else {
                // 将图像绘制到界面上
                // 注意 : 这里 rgba_frame 一行的像素和 window_buffer 一行的像素长度可能不一致
                // 需要转换好 否则可能花屏
                uint8_t *bits = (uint8_t *) window_buffer.bits;
                for (int h = 0; h < videoHeight; h++) {
                    memcpy(bits + h * window_buffer.stride * 4,
                           out_buffer + h * rgba_frame->linesize[0],
                           static_cast<size_t>(rgba_frame->linesize[0]));
                }
                ANativeWindow_unlockAndPost(native_window);
            }
        }
        // 释放 packet 引用
        av_packet_unref(packet);
    }
    // 释放 R9
    sws_freeContext(data_convert_context);
    // 释放 R8
    av_free(out_buffer);
    // 释放 R7
    av_frame_free(&rgba_frame);
    // 释放 R6
    av_frame_free(&frame);
    // 释放 R5
    av_packet_free(&packet);
    // 释放 R4
    ANativeWindow_release(native_window);
    // 关闭 R3
    avcodec_close(video_codec_context);
    // 释放 R2
    avformat_close_input(&format_context);
    // 释放 R1
    env->ReleaseStringUTFChars(path_, path);
}extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1sound(JNIEnv *env, jobject thiz, jstring input_,
                                                  jstring out_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *out = env->GetStringUTFChars(out_, 0);
    int ret;
    //初始化网络
    avformat_network_init();
    //总上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //打开音频文件
    ret = avformat_open_input(&formatContext, input, NULL, NULL);
    if (ret != 0) {
        ALOGE("无法打开音频文件");
        return;
    }
    //获取输入文件信息
    ret = avformat_find_stream_info(formatContext, NULL);
    if (ret < 0) {
        ALOGE("无法输入文件信息");
        return;
    }

    //音频时长
    int audio_stream_idx = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
        }
    }
    ALOGE("%i", audio_stream_idx);
    //获取解码器参数
    AVCodecParameters *codecParameters = formatContext->streams[audio_stream_idx]->codecpar;
    //获取解码器
    AVCodec *dec = avcodec_find_decoder(codecParameters->codec_id);
    //获取解码器上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(codecContext, codecParameters);
    avcodec_open2(codecContext, dec, NULL);
    SwrContext *swrContext = swr_alloc();
    //输入参数
    AVSampleFormat in_sample = codecContext->sample_fmt;
    //输入采样率
    int in_sample_rate = codecContext->sample_rate;
    //输入通道布局
    uint64_t in_ch_layout = codecContext->channel_layout;

    //输出参数
    AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    //将参数设置到转换器中
    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample, out_sample_rate, in_ch_layout,
                       in_sample, in_sample_rate, 0, NULL);
    //初始化转换器其他的默认参数
    swr_init(swrContext);
    // 一般都是 通道数 × 输出采样率
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
    FILE *fp_pcm = fopen(out, "wb");


    //读取包 压缩过的数据
    AVPacket *packet = av_packet_alloc();
//    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    int count = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        avcodec_send_packet(codecContext, packet);
        //解压数据  将packet的压缩数据转化为AVFrame
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            ALOGE("解压完成");
            break;
        }

        if (packet->stream_index != audio_stream_idx) {
            continue;
        }
        ALOGE("正在解码 ——》 %i", count++);
        swr_convert(swrContext, &out_buffer, 2 * 44100, (const uint8_t **) frame->data,
                    frame->nb_samples);


        //计算声道数
        int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
        //缓冲区大小
        int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
                                                         out_sample, 1);
        fwrite(out_buffer, 1, out_buffer_size, fp_pcm);
        av_frame_free(&frame);
    }
    //释放资源
    fclose(fp_pcm);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(out_, out);

}