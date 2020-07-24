//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_VIDEOCHANNEL_H
#define FFMPEG_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"
#include "macro.h"

extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
};

/**
 * 解码  播放
 */
typedef void (*RenderFrameCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {

public:
    VideoChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, int fps);

    ~VideoChannel();

    void setAudioChannel(AudioChannel *audioChannel);

    //解码+播放
    void play();

    void decode();

    void render();

    void setRenderFrameCallback(RenderFrameCallback callback);

private:
    pthread_t pid_decode;
    pthread_t pid_render;
    SwsContext *swsContext = 0;
    RenderFrameCallback callback;
    AudioChannel *audioChannel = 0;
    int fps;
};


#endif //FFMPEG_VIDEOCHANNEL_H
