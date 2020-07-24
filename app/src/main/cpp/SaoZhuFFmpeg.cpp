//
// Created by Administrator on 2020/7/16.
#include <cstring>
#include <pthread.h>
#include "SaoZhuFFmpeg.h"
#include "macro.h"

extern "C" {
#include <libavformat/avformat.h>
}

/**
 * 线程中执行的函数
 * @param args 传过来的参数  this
 * @return  线程函数必须返回
 */
void *task_prepare(void *args) {
    SaoZhuFFmpeg *ffmpeg = static_cast<SaoZhuFFmpeg *>(args);
    ffmpeg->_prepare();
    return 0;
}

SaoZhuFFmpeg::SaoZhuFFmpeg(JavaCallHelper *callHelper, const char *dataSource) {
    this->callHelper = callHelper;
    //防止 dataSource参数 指向的内存被释放
    //strlen 获得字符串的长度 不包括\0
    this->dataSource = new char[strlen(dataSource) + 1];
    strcpy(this->dataSource, dataSource);
    ALOGE("dataSource = %s", dataSource);
}

SaoZhuFFmpeg::~SaoZhuFFmpeg() {
    //释放
    DELETE(dataSource);
    DELETE(callHelper);
}


void SaoZhuFFmpeg::prepare() {
    //创建一个线程
    pthread_create(&pid, 0, task_prepare, this);
}

void SaoZhuFFmpeg::_prepare() {
    // 初始化网络 让ffmpeg能够使用网络
    avformat_network_init();
    //1、打开媒体地址(文件地址、直播地址)
    // AVFormatContext  包含了 视频的 信息(宽、高等)
    formatContext = 0;
    //文件路径不对 手机没网
    AVDictionary *options = 0;
    //设置超时时间 微妙 超时时间30秒
    av_dict_set(&options, "timeout", "30000000", 0);
    int ret = avformat_open_input(&formatContext, dataSource, 0, &options);
    av_dict_free(&options);
    //ret不为0表示 打开媒体失败
    if (ret != 0) {
        ALOGE("打开媒体失败:%i", ret);
        ALOGE("打开媒体失败:%s", av_err2str(ret));
        callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId, FFMPEG_CAN_NOT_OPEN);
        return;
    }
    //2、查找媒体中的 音视频流 (给context里的 streams等成员赋)
    ret = avformat_find_stream_info(formatContext, 0);
    // 小于0 则失败
    if (ret < 0) {
        ALOGE("查找流失败:%s", av_err2str(ret));
        callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId,
                                   FFMPEG_CAN_NOT_FIND_STREAMS);
        return;
    }
    //nb_streams :几个流(几段视频/音频)
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //可能代表是一个视频 也可能代表是一个音频
        AVStream *stream = formatContext->streams[i];
        //包含了 解码 这段流 的各种参数信息(宽、高、码率、帧率)
        AVCodecParameters *codecpar = stream->codecpar;
        AVCodecID id = codecpar->codec_id;

        // TODO lzb 有的视频获取的是 AVCodecID = 0(AV_CODEC_ID_NONE)，0直接导致 查找解码器失败  不懂为什么
        ALOGE("AVCodecID = %i", id);
        if (id == 0) {
            continue;
        }

        //无论视频还是音频都需要干的一些事情（获得解码器）
        // 1、通过 当前流 使用的 编码方式，查找解码器
        AVCodec *dec = avcodec_find_decoder(id);
        if (dec == NULL) {
            ALOGE("查找解码器失败");
            callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId,
                                       FFMPEG_FIND_DECODER_FALL);
            return;
        }
        //2、获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(dec);
        if (context == NULL) {
            ALOGE("创建解码上下文失败:%s", av_err2str(ret));
            callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId,
                                       FFMPEG_ALLOC_CODEC_CONTEXT_FALL);
            return;
        }
        //3、设置上下文内的一些参数 (context->width)
//        context->width = codecpar->width;
//        context->height = codecpar->height;
        ret = avcodec_parameters_to_context(context, codecpar);
        //失败
        if (ret < 0) {
            ALOGE("设置解码上下文参数失败:%s", av_err2str(ret));
            callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId,
                                       FFMPEG_CODEC_CONTEXT_PARAMETERS_FALL);
            return;
        }
        // 4、打开解码器
        ret = avcodec_open2(context, dec, 0);
        if (ret != 0) {
            ALOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId,
                                       FFMPEG_OPEN_DECODER_FALL);
            return;
        }

        //获得单位
        AVRational time_base = stream->time_base;
        //音频
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            //0
            audioChannel = new AudioChannel(i, context, time_base);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//视频
            //1
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = static_cast<int>(av_q2d(frame_rate));
            videoChannel = new VideoChannel(i, context, time_base, fps);
            videoChannel->setRenderFrameCallback(callback);
        }
    }
    //没有音视频  (很少见)
    if (!audioChannel && !videoChannel) {
        ALOGE("没有音视频");
        callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onErrorId, FFMPEG_NOMEDID);
        return;
    }
    // 准备完了 通知java 你随时可以开始播放
    callHelper->onJavaBackCall(THREAD_CHILD, callHelper->onPrepareId, NULL);
};

void *play(void *args) {
    SaoZhuFFmpeg *ffmpeg = static_cast<SaoZhuFFmpeg *>(args);
    ffmpeg->_start();
    return 0;
}


void SaoZhuFFmpeg::start() {
    // 正在播放
    isPlaying = 1;

    //启动声音的解码与播放
    if (audioChannel) {
        audioChannel->play();
    }
    //视频解码+播放
    if (videoChannel) {
        //设置为工作状态
//        videoChannel->packets.setWork(1);
        videoChannel->play();
        videoChannel->setAudioChannel(audioChannel);
    }
    pthread_create(&pid_play, 0, play, this);
}

/**
 * 专门读取数据包
 */
void SaoZhuFFmpeg::_start() {
    //1、读取媒体数据包(音视频数据包)
    int ret;
    while (isPlaying) {
        //读取音视频文件时候，可能导致一下子读完，导致oom
        if (audioChannel && audioChannel->packets.size() > 100) {
            //休息10ms
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }


        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(formatContext, packet);
        //=0成功 其他:失败
        if (ret == 0) {
            //stream_index 这一个流的一个序号
            if (audioChannel && packet->stream_index == audioChannel->id) {//音频
                audioChannel->packets.push(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->id) {//视频
                videoChannel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完成 但是可能还没播放完
            //为什么这里要让它继续循环 而不是sleep
            //如果是做直播 ，可以sleep
            //如果要支持点播(播放本地文件） seek 后退
            if (audioChannel->packets.empty() && audioChannel->frames.empty()
                && videoChannel->packets.empty() && videoChannel->frames.empty()) {
                break;
            }
        } else {
            //
            break;
        }

    }
    isPlaying = 0;
    audioChannel->stop();
    videoChannel->stop();
};

void SaoZhuFFmpeg::setRenderFrameCallback(RenderFrameCallback callback) {
    this->callback = callback;
}

/**
 * 释放资源
 * @param args
 * @return
 */
void *sync_stop(void *args) {
    SaoZhuFFmpeg *fFmpeg = static_cast<SaoZhuFFmpeg *>(args);
    //等待prepare结束
    pthread_join(fFmpeg->pid, 0);
    //等待play线程结束
    pthread_join(fFmpeg->pid_play, 0);
    DELETE(fFmpeg->videoChannel);
    DELETE(fFmpeg->audioChannel);

    if (fFmpeg->formatContext) {
        avformat_close_input(&fFmpeg->formatContext);
        avformat_free_context(fFmpeg->formatContext);
        fFmpeg->formatContext = 0;
    }
    DELETE(fFmpeg);
    return 0;
}

void SaoZhuFFmpeg::stop() {
    //暂停 释放所有资源
    isPlaying = 0;
    callHelper = 0;
    pthread_create(&pid_stop, 0, sync_stop, this);
}
