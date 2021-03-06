#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "SaoZhuFFmpeg.h"
#include "macro.h"

SaoZhuFFmpeg *ffmpeg = 0;
JavaVM *javaVm = 0;
ANativeWindow *window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
JavaCallHelper *javaCallHelper = 0;

int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

//画画
void render(uint8_t *data, int lineszie, int w, int h) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineszie, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                    jstring data_source) {
    const char *dataSource = env->GetStringUTFChars(data_source, 0);
    //创建播放器
    javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
    ffmpeg = new SaoZhuFFmpeg(javaCallHelper, dataSource);
    ffmpeg->setRenderFrameCallback(render);
    ffmpeg->prepare();
    //释放资源
    env->ReleaseStringUTFChars(data_source, dataSource);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1start(JNIEnv *env, jobject thiz) {
    ffmpeg->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1stop(JNIEnv *env, jobject thiz) {


    if (ffmpeg){
        ffmpeg->stop();
    }
    DELETE(javaCallHelper);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bochao_ffmpeg_SaoZhuPlayer_native_1release(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
}