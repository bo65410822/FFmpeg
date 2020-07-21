//
// Created by Administrator on 2020/7/16.
//

#ifndef FFMPEG_JAVACALLHELPER_H
#define FFMPEG_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance);

    ~JavaCallHelper();

    void onPrepare(int thread);

    void onError(int thread, int errorCode);

private:
    JavaVM *vm;
    JNIEnv *env;
    jobject instance;
    jmethodID onErrorId;
    jmethodID onPrepareId;
};


#endif //FFMPEG_JAVACALLHELPER_H
