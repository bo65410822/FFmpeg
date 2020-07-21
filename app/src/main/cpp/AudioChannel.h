//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_AUDIOCHANNEL_H
#define FFMPEG_AUDIOCHANNEL_H


#include "BaseChannel.h"

class AudioChannel : public BaseChannel {
public:
    AudioChannel(int id, AVCodecContext *avCodecContext);
    void play();
};


#endif //FFMPEG_AUDIOCHANNEL_H
