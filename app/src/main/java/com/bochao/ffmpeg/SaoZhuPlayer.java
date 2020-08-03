package com.bochao.ffmpeg;

import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.util.HashMap;
import java.util.Map;

/**
 * @author lzb
 */
public class SaoZhuPlayer implements SurfaceHolder.Callback {
    private static final String TAG = "SaoZhuPlayer";
    public static Map<Integer,String> ERROR_CODE = new HashMap<>();
    static {
        System.loadLibrary("saozhu");
        ERROR_CODE.put(1,"打不开视频");
        ERROR_CODE.put(2,"找不到流媒体");
        ERROR_CODE.put(3,"找不到解码器");
        ERROR_CODE.put(4,"创建解码器失败");
        ERROR_CODE.put(5,"参数失败");
        ERROR_CODE.put(6,"打开解码器失败");
        ERROR_CODE.put(7,"没有音视频");
    }



    private String dataSource;
    private SurfaceHolder holder;
    private OnPrepareListener listener;

    /**
     * 让使用 设置播放的文件 或者 直播地址
     */
    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 设置播放显示的画布
     *
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    /**
     * 准备播放
     */
    public void prepare() {
        native_prepare(dataSource);
    }

    /**
     * 开始播放
     */
    public void start() {
        if (TextUtils.isEmpty(dataSource)) {
            throw new NullPointerException("请先调用 setDataSource() 设置路径");
        }
        native_start();
    }


    /**
     * 停止播放
     */
    public void stop() {
        native_stop();
        native_release();
    }

    /**
     * 释放资源
     */
    public void release() {
        holder.removeCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
//        native_setSurface(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.e(TAG, "surfaceChanged:");
        native_setSurface(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    /**
     * 错误回调（C++回调）
     *
     * @param error
     */
    public void onError(int error) {
        onErrorListener.onError(error);
    }

    /**
     * 提供c++反射调用
     */
    public void onPrepare() {
        if (null != listener) {
            listener.onPrepare();
        }
    }

    private OnErrorListener onErrorListener;

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setListener(OnPrepareListener listener) {
        this.listener = listener;

    }

    public interface OnPrepareListener {
        void onPrepare();
    }

    public interface OnErrorListener {
        void onError(int e);
    }

    public native void native_prepare(String dataSource);

    public native void native_start();

    public native void native_setSurface(Surface surface);

    public native void native_stop();

    public native void native_release();
}
