package com.bochao.ffmpeg;

import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * @author lzb
 */
public class SaoZhuPlayer implements SurfaceHolder.Callback {
    private static final String TAG = "SaoZhuPlayer";

    static {
        System.loadLibrary("saozhu");
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

    }

    /**
     * 提供c++反射调用
     */
    public void onPrepare() {
        if (null != listener) {
            listener.onPrepare();
        }
    }



    public void setListener(OnPrepareListener listener) {
        this.listener = listener;

    }

    public interface OnPrepareListener {
        void onPrepare();
    }

    public native void native_prepare(String dataSource);

    public native void native_start();

    public native void native_setSurface(Surface surface);
}
