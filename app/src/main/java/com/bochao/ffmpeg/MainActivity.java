package com.bochao.ffmpeg;

import android.Manifest;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.bochao.ffmpeg.utils.PermissionUtil;


/**
 * @author 李占博
 */
public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private SurfaceView surfaceView;
    private SaoZhuPlayer saoZhuPlayer;

    private String[] permissions = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };
    private String path;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);
        path = Environment.getExternalStorageDirectory() + "/DCIM/Camera/saozhu.mp4";
        PermissionUtil.checkPermissions(this, permissions, this::initPlayer);
    }


    /**
     * 初始化播放器
     */
    private void initPlayer() {
        Log.e(TAG, "initPlayer: 创建播放器");
        saoZhuPlayer = new SaoZhuPlayer();
        saoZhuPlayer.setSurfaceView(surfaceView);
        saoZhuPlayer.setDataSource(path);
        saoZhuPlayer.setListener(() -> runOnUiThread(() -> Toast.makeText(MainActivity.this, "骚猪准备完毕，可以发骚了", Toast.LENGTH_LONG).show()));
    }


    @Override
    protected void onResume() {
        super.onResume();
        saoZhuPlayer.prepare();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        PermissionUtil.onRequestPermissionsResult(requestCode, grantResults, this::initPlayer,
                () -> Toast.makeText(MainActivity.this, "请打开打开权限", Toast.LENGTH_SHORT).show());
    }

    public void open(View view) {
        saoZhuPlayer.start();
    }

}
