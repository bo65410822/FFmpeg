package com.bochao.ffmpeg;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

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
        requestPermission();
        saoZhuPlayer = new SaoZhuPlayer();
        saoZhuPlayer.setSurfaceView(surfaceView);
    }

    public void open(View view) {
        Log.e(TAG, "onCreate: ffmpeg_v = " + stringFromJNI());
//        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/DCIM/Camera/saozhu.mp4");
//        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/Pictures/WeiXin/saozhu.mp4");

        File file = new File(Environment.getExternalStorageDirectory().getPath() + "/123.mp3");
        File file1 = new File(Environment.getExternalStorageDirectory().getPath() + "/123.pcm");
        Log.e(TAG, "open: file = " + file.getAbsolutePath());
//        saoZhuPlayer.start(file.getAbsolutePath());
        saoZhuPlayer.sound(file.getAbsolutePath(), file1.getAbsolutePath());
    }

    public void requestPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {

            Toast.makeText(this, "申请权限", Toast.LENGTH_SHORT).show();

            // 申请 相机 麦克风权限
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE}, 100);
        }
    }

    public native String stringFromJNI();
}
