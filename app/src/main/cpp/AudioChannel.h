//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_AUDIOCHANNEL_H
#define FFMPEG_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel {
public:
    AudioChannel(int id, AVCodecContext *avCodecContext,AVRational time_base);

    ~AudioChannel();
    void play();

    void decode();

    void _play();

    int getPcm();

public:
    uint8_t *data = 0;
    int out_channels;//通道数
    int out_samplesize;
    int out_sample_rate;//采样率
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    /**
     * OpenSLES
     */
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    SLObjectItf outputMixObject = 0;

    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;

    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface = 0;


    //重采样
    SwrContext *swrContext = 0;
};


#endif //FFMPEG_AUDIOCHANNEL_H
