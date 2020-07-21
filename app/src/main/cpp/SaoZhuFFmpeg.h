//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_SAOZHUFFMPEG_H
#define FFMPEG_SAOZHUFFMPEG_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C" {
#include <libavformat/avformat.h>
};

class SaoZhuFFmpeg {
public:
    SaoZhuFFmpeg(JavaCallHelper *callHelper, const char *dataSource);

    ~SaoZhuFFmpeg();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderFrameCallback(RenderFrameCallback callback);

private:
    char *dataSource;
    pthread_t pid ;
    pthread_t pid_play;
    AVFormatContext *formatContext;
    JavaCallHelper *callHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    RenderFrameCallback callback;
    bool isPlaying;
};


#endif //FFMPEG_SAOZHUFFMPEG_H
