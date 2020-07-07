package com.bochao.ffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import java.io.File;

/**
 * @author 李占博
 */
public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private SurfaceView surfaceView;

    private SaoZhuPlayer saoZhuPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);


        saoZhuPlayer = new SaoZhuPlayer();
        saoZhuPlayer.setSurfaceView(surfaceView);
    }

    public void open(View view) {
        Log.e(TAG, "onCreate: ffmpeg_v = " + stringFromJNI());
        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/DCIM/Camera/saozhu.mp4");
//        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/Pictures/WeiXin/saozhu.mp4");
        Log.e(TAG, "open: file = " + file.getAbsolutePath());
        saoZhuPlayer.start(file.getAbsolutePath());
    }

    public native String stringFromJNI();
}
