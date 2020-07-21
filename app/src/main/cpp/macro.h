//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_MACRO_H
#define FFMPEG_MACRO_H

#include <android/log.h>

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lzb", __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN,  "lzb", __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "lzb", __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  "lzb", __VA_ARGS__)

//宏函数
#define DELETE(obj) if(obj){ delete obj; obj = 0; }

//标记线程是子线程还是主线程
#define THREAD_MAIN 1
#define THREAD_CHILD 2

//-----------错误代码------------
//打不开视频
#define FFMPEG_CAN_NOT_OPEN 1
//找不到流媒体
#define  FFMPEG_CAN_NOT_FIND_STREAMS 2
//找不到解码器
#define FFMPEG_FIND_DECODER_FALL 3
//无法根据解码器创建上下文
#define FFMPEG_ALLOC_CODEC_CONTEXT_FALL 4
//根据流信息 配置上下文参数失败
#define FFMPEG_CODEC_CONTEXT_PARAMETERS_FALL 5
//打开解码器失败
#define FFMPEG_OPEN_DECODER_FALL 6
//没有音视频
#define FFMPEG_NOMEDID 7

#endif //FFMPEG_MACRO_H
