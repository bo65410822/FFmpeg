//
// Created by Administrator on 2020/7/16.
//

#include "JavaCallHelper.h"
#include "macro.h"


JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {

    //跨线程回调使用
    this->vm = vm;
    //在主线程直接回调
    this->env = env;
    //一旦涉及到jobject跨方法 跨线程 就需要用全局引用
    this->instance = env->NewGlobalRef(instance);
    //C++反射调用Java方法
    jclass clazz = env->GetObjectClass(instance);
    onErrorId = env->GetMethodID(clazz, "onError", "(I)V");
    onPrepareId = env->GetMethodID(clazz, "onPrepare", "()V");

}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(instance);
}

/**
 * 回调Java函数
 * @param thread
 * @param errorCode
 */
void JavaCallHelper::onJavaBackCall(int thread, jmethodID jmethodId, int errorCode) {
    if (thread == THREAD_MAIN) {
        if (errorCode) {
            env->CallVoidMethod(instance, jmethodId, errorCode);
        } else{
            env->CallVoidMethod(instance, jmethodId);
        }
    } else {
        //子线程
        JNIEnv *env;
        //获取当前线程的env
        vm->AttachCurrentThread(&env, 0);
        if (errorCode) {
            env->CallVoidMethod(instance, jmethodId, errorCode);
        } else{
            env->CallVoidMethod(instance, jmethodId);
        }
        vm->DetachCurrentThread();
    }
}

void JavaCallHelper::onError(int thread, int errorCode) {

    //主线程
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(instance, onErrorId, errorCode);
    } else {
        //子线程
        JNIEnv *env;
        //获取当前线程的env
        vm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(instance, onErrorId, errorCode);
        vm->DetachCurrentThread();
    }
}

void JavaCallHelper::onPrepare(int thread) {
    //主线程
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(instance, onPrepareId);
    } else {
        //子线程
        JNIEnv *env;
        //获取当前线程的env
        vm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(instance, onPrepareId);
        vm->DetachCurrentThread();
    }
}
