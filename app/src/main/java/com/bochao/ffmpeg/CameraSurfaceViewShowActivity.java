package com.bochao.ffmpeg;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.bochao.ffmpeg.utils.PermissionUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;


/**
 * @author 李占博
 * @description:
 * @date :2020/7/14  10:09
 */
public class CameraSurfaceViewShowActivity extends AppCompatActivity {
    private static final String TAG = CameraSurfaceViewShowActivity.class.getName();
    private String[] permission = {Manifest.permission.CAMERA};
    private TextureView mTextureView;    //注意使用TextureView需要开启硬件加速,开启方法很简单在AndroidManifest.xml 清单文件里,你需要使用TextureView的activity添加android:hardwareAccelerated="true"
    private Button mBtnPhotograph;
    private HandlerThread mHandlerThread;
    private Handler mChildHandler = null;
    private CameraManager mCameraManager;                                //相机管理类,用于检测系统相机获取相机id
    private CameraDevice mCameraDevice;                                        //Camera设备类
    private CameraCaptureSession.StateCallback mSessionStateCallback;        //获取的会话类状态回调
    private CameraCaptureSession.CaptureCallback mSessionCaptureCallback;    //获取会话类的获取数据回调
    private CaptureRequest.Builder mCaptureRequest;                            //获取数据请求配置类
    private CameraDevice.StateCallback mStateCallback;                        //摄像头状态回调
    private CameraCaptureSession mCameraCaptureSession;                    //获取数据会话类
    private ImageReader mImageReader;                                        //照片读取器
    private Surface mSurface;
    private SurfaceTexture mSurfaceTexture;
    private String mCurrentCameraId;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();


//    static {// /为了使照片竖直显示
//        ORIENTATIONS.append(Surface.ROTATION_0, 90);
//        ORIENTATIONS.append(Surface.ROTATION_90, 0);
//        ORIENTATIONS.append(Surface.ROTATION_180, 270);
//        ORIENTATIONS.append(Surface.ROTATION_270, 180);
//    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);
        mTextureView = findViewById(R.id.camera_view);
        mBtnPhotograph = findViewById(R.id.btn_Photograph);
        mBtnPhotograph.setOnClickListener(v -> {
            try {
                mCameraCaptureSession.stopRepeating();//停止重复   取消任何正在进行的重复捕获集 在这里就是停止画面预览
                /*  mCameraCaptureSession.abortCaptures(); //终止获取   尽可能快地放弃当前挂起和正在进行的所有捕获。
                 * 这里有一个坑,其实这个并不能随便调用(我是看到别的demo这么使用,但是其实是错误的,所以就在这里备注这个坑).
                 * 最好只在Activity里的onDestroy调用它,终止获取是耗时操作,需要一定时间重新打开会话通道.
                 * 在这个demo里我并没有恢复预览,如果你调用了这个方法关闭了会话又拍照后恢复图像预览,会话就会频繁的开关,
                 * 导致拍照图片在处理耗时缓存时你又关闭了会话.导致照片缓存不完整并且失败.
                 * 所以切记不要随便使用这个方法,会话开启后并不需要关闭刷新.后续其他拍照/预览/录制视频直接操作这个会话即可
                 */
                takePicture();//拍照
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
        });
        initPermission();
        initChildThread();
        initImageReader();
        initTextureView();

    }

    /**
     * 初始化权限
     */
    private void initPermission() {
        PermissionUtil.checkPermissions(this, new String[]{Manifest.permission.CAMERA}, () -> Log.e(TAG, "initPermission: 权限允许"));
    }

    private void initTextureView() {
        mTextureView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                Log.e(TAG, "TextureView 启用成功");
                initCameraManager();
                initCameraCallback();
                initCameraCaptureSessionStateCallback();
                initCameraCaptureSessionCaptureCallback();
                selectCamera();
                openCamera();

            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
                Log.e(TAG, "onSurfaceTextureSizeChanged ");
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                Log.e(TAG, "SurfaceTexture 的销毁");
                //这里返回true则是交由系统执行释放，如果是false则需要自己调用surface.release();
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {
                Log.e(TAG, "onSurfaceTextureUpdated ");
            }
        });
    }

    /**
     * 初始化子线程
     */
    private void initChildThread() {
        mHandlerThread = new HandlerThread("camera2");
        mHandlerThread.start();
        mChildHandler = new Handler(mHandlerThread.getLooper());
    }

    /**
     * 初始化相机管理
     */
    private void initCameraManager() {
        mCameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
    }

    /**
     * 获取匹配的大小
     *
     * @return
     */
    private Size getMatchingSize() {

        Size selectSize = null;
        float selectProportion = 0;
        try {
            float viewProportion = (float) mTextureView.getWidth() / (float) mTextureView.getHeight();
            CameraCharacteristics cameraCharacteristics = mCameraManager.getCameraCharacteristics(mCurrentCameraId);
            StreamConfigurationMap streamConfigurationMap = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            Size[] sizes = streamConfigurationMap.getOutputSizes(ImageFormat.JPEG);
            for (int i = 0; i < sizes.length; i++) {
                Size itemSize = sizes[i];
                float itemSizeProportion = (float) itemSize.getHeight() / (float) itemSize.getWidth();
                float differenceProportion = Math.abs(viewProportion - itemSizeProportion);
                Log.e(TAG, "相减差值比例=" + differenceProportion);
                if (i == 0) {
                    selectSize = itemSize;
                    selectProportion = differenceProportion;
                    continue;
                }
                if (differenceProportion <= selectProportion) {
                    if (differenceProportion == selectProportion) {
                        if (selectSize.getWidth() + selectSize.getHeight() < itemSize.getWidth() + itemSize.getHeight()) {
                            selectSize = itemSize;
                            selectProportion = differenceProportion;
                        }

                    } else {
                        selectSize = itemSize;
                        selectProportion = differenceProportion;
                    }
                }
            }

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        Log.e(TAG, "getMatchingSize: 选择的比例是=" + selectProportion);
        Log.e(TAG, "getMatchingSize: 选择的尺寸是 宽度=" + selectSize.getWidth() + "高度=" + selectSize.getHeight());
        return selectSize;
    }

    /**
     * 选择摄像头
     */
    private void selectCamera() {
        try {
            String[] cameraIdList = mCameraManager.getCameraIdList();//获取摄像头id列表
            if (cameraIdList.length == 0) {
                return;
            }

            for (String cameraId : cameraIdList) {
                Log.e(TAG, "selectCamera: cameraId=" + cameraId);
                //获取相机特征,包含前后摄像头信息，分辨率等
                CameraCharacteristics cameraCharacteristics = mCameraManager.getCameraCharacteristics(cameraId);
                Integer facing = cameraCharacteristics.get(CameraCharacteristics.LENS_FACING);//获取这个摄像头的面向
                //CameraCharacteristics.LENS_FACING_BACK 后摄像头
                //CameraCharacteristics.LENS_FACING_FRONT 前摄像头
                //CameraCharacteristics.LENS_FACING_EXTERNAL 外部摄像头,比如OTG插入的摄像头
                if (facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    mCurrentCameraId = cameraId;

                }

            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * 初始化摄像头状态回调
     */
    private void initCameraCallback() {
        mStateCallback = new CameraDevice.StateCallback() {
            /**
             * 摄像头打开时
             * @param camera
             */
            @Override
            public void onOpened(@NonNull CameraDevice camera) {
                Log.e(TAG, "相机开启");
                mCameraDevice = camera;

                try {
                    mSurfaceTexture = mTextureView.getSurfaceTexture();        //surfaceTexture    需要手动释放
                    Size matchingSize = getMatchingSize();
                    mSurfaceTexture.setDefaultBufferSize(matchingSize.getWidth(), matchingSize.getHeight());//设置预览的图像尺寸
                    mSurface = new Surface(mSurfaceTexture);//surface最好在销毁的时候要释放,surface.release();
//                        CaptureRequest可以完全自定义拍摄参数,但是需要配置的参数太多了,所以Camera2提供了一些快速配置的参数,如下:
// 　　　　　　　　　      TEMPLATE_PREVIEW ：预览
//                        TEMPLATE_RECORD：拍摄视频
//                        TEMPLATE_STILL_CAPTURE：拍照
//                        TEMPLATE_VIDEO_SNAPSHOT：创建视视频录制时截屏的请求
//                        TEMPLATE_ZERO_SHUTTER_LAG：创建一个适用于零快门延迟的请求。在不影响预览帧率的情况下最大化图像质量。
//                        TEMPLATE_MANUAL：创建一个基本捕获请求，这种请求中所有的自动控制都是禁用的(自动曝光，自动白平衡、自动焦点)。
                    mCaptureRequest = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);//创建预览请求
                    mCaptureRequest.addTarget(mSurface); //添加surface   实际使用中这个surface最好是全局变量 在onDestroy的时候mCaptureRequest.removeTarget(mSurface);清除,否则会内存泄露
                    mCaptureRequest.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);//自动对焦
                    /**
                     * 创建获取会话
                     * 这里会有一个容易忘记的坑,那就是Arrays.asList(surface, mImageReader.getSurface())这个方法
                     * 这个方法需要你导入后面需要操作功能的所有surface,比如预览/拍照如果你2个都要操作那就要导入2个
                     * 否则后续操作没有添加的那个功能就报错surface没有准备好,这也是我为什么先初始化ImageReader的原因,因为在这里就可以拿到ImageReader的surface了
                     */
                    mCameraDevice.createCaptureSession(Arrays.asList(mSurface, mImageReader.getSurface()), mSessionStateCallback, mChildHandler);
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }

            }

            /**
             *摄像头断开时
             * @param camera
             */
            @Override
            public void onDisconnected(@NonNull CameraDevice camera) {

            }

            /**
             * 出现异常情况时
             * @param camera
             * @param error
             */
            @Override
            public void onError(@NonNull CameraDevice camera, int error) {

            }

            /**
             * 摄像头关闭时
             * @param camera
             */
            @Override
            public void onClosed(@NonNull CameraDevice camera) {
                super.onClosed(camera);
            }
        };
    }

    /**
     * 摄像头获取会话状态回调
     */
    private void initCameraCaptureSessionStateCallback() {
        mSessionStateCallback = new CameraCaptureSession.StateCallback() {


            //摄像头完成配置，可以处理Capture请求了。
            @Override
            public void onConfigured(@NonNull CameraCaptureSession session) {
                try {
                    mCameraCaptureSession = session;
                    //注意这里使用的是 setRepeatingRequest() 请求通过此捕获会话无休止地重复捕获图像。用它来一直请求预览图像
                    mCameraCaptureSession.setRepeatingRequest(mCaptureRequest.build(), mSessionCaptureCallback, mChildHandler);


//                    mCameraCaptureSession.stopRepeating();//停止重复   取消任何正在进行的重复捕获集
//                    mCameraCaptureSession.abortCaptures();//终止获取   尽可能快地放弃当前挂起和正在进行的所有捕获。请只在销毁activity的时候调用它
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }

            }

            //摄像头配置失败
            @Override
            public void onConfigureFailed(@NonNull CameraCaptureSession session) {

            }
        };
    }

    /**
     * 摄像头获取会话数据回调
     */
    private void initCameraCaptureSessionCaptureCallback() {
        mSessionCaptureCallback = new CameraCaptureSession.CaptureCallback() {
            @Override
            public void onCaptureStarted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, long timestamp, long frameNumber) {
                super.onCaptureStarted(session, request, timestamp, frameNumber);
            }

            @Override
            public void onCaptureProgressed(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull CaptureResult partialResult) {
                super.onCaptureProgressed(session, request, partialResult);
            }

            @Override
            public void onCaptureCompleted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull TotalCaptureResult result) {
                super.onCaptureCompleted(session, request, result);
//                Log.e(TAG, "onCaptureCompleted: 触发接收数据");
//                Size size = request.get(CaptureRequest.JPEG_THUMBNAIL_SIZE);

            }

            @Override
            public void onCaptureFailed(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull CaptureFailure failure) {
                super.onCaptureFailed(session, request, failure);
            }

            @Override
            public void onCaptureSequenceCompleted(@NonNull CameraCaptureSession session, int sequenceId, long frameNumber) {
                super.onCaptureSequenceCompleted(session, sequenceId, frameNumber);
            }

            @Override
            public void onCaptureSequenceAborted(@NonNull CameraCaptureSession session, int sequenceId) {
                super.onCaptureSequenceAborted(session, sequenceId);
            }

            @Override
            public void onCaptureBufferLost(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull Surface target, long frameNumber) {
                super.onCaptureBufferLost(session, request, target, frameNumber);
            }
        };
    }

    /**
     * 打开摄像头
     */
    private void openCamera() {
        try {
            if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
                mCameraManager.openCamera(mCurrentCameraId, mStateCallback, mChildHandler);
                return;
            }
            Toast.makeText(this, "没有授权", Toast.LENGTH_SHORT).show();

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * 初始化图片读取器
     */
    private void initImageReader() {
        //创建图片读取器,参数为分辨率宽度和高度/图片格式/需要缓存几张图片,我这里写的2意思是获取2张照片
        mImageReader = ImageReader.newInstance(1080, 1920, ImageFormat.JPEG, 2);
        mImageReader.setOnImageAvailableListener(reader -> {
//        image.acquireLatestImage();//从ImageReader的队列中获取最新的image,删除旧的
//        image.acquireNextImage();//从ImageReader的队列中获取下一个图像,如果返回null没有新图像可用
            Image image = reader.acquireNextImage();
            try {
                File path = new File(CameraSurfaceViewShowActivity.this.getExternalCacheDir().getPath());
                if (!path.exists()) {
                    Log.e(TAG, "onImageAvailable: 路径不存在");
                    path.mkdirs();
                } else {
                    Log.e(TAG, "onImageAvailable: 路径存在");
                }
                File file = new File(path, "demo.jpg");
                FileOutputStream fileOutputStream = new FileOutputStream(file);
//               这里的image.getPlanes()[0]其实是图层的意思,因为我的图片格式是JPEG只有一层所以是geiPlanes()[0],如果你是其他格式(例如png)的图片会有多个图层,就可以获取指定图层的图像数据　　　　　　　
                ByteBuffer byteBuffer = image.getPlanes()[0].getBuffer();
                byte[] bytes = new byte[byteBuffer.remaining()];
                byteBuffer.get(bytes);
                fileOutputStream.write(bytes);
                fileOutputStream.flush();
                fileOutputStream.close();
                image.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }


        }, mChildHandler);
    }

    private void takePicture() {
        CaptureRequest.Builder captureRequestBuilder = null;
        try {
            captureRequestBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);//自动对焦
            captureRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH);//自动爆光
//             获取手机方向,如果你的app有提供横屏和竖屏,那么就需要下面的方法来控制照片为竖立状态
            int rotation = getWindowManager().getDefaultDisplay().getRotation();
//            Log.e(TAG, "takePicture: 手机方向="+rotation);
//            Log.e(TAG, "takePicture: 照片方向="+ORIENTATIONS.get(rotation));
            captureRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, ORIENTATIONS.get(rotation));//我的项目不需要,直接写死270度 将照片竖立
            Surface surface = mImageReader.getSurface();
            captureRequestBuilder.addTarget(surface);
            CaptureRequest request = captureRequestBuilder.build();
            mCameraCaptureSession.capture(request, null, mChildHandler); //获取拍照
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case 1:
                if (permissions.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "授权成功", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "授权失败", Toast.LENGTH_SHORT).show();
                    finish();
                }
                break;
            default:
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCaptureRequest != null) {
            mCaptureRequest.removeTarget(mSurface);
            mCaptureRequest = null;
        }
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
        if (mSurfaceTexture != null) {
            mSurfaceTexture.release();
            mSurfaceTexture = null;
        }
        if (mCameraCaptureSession != null) {
            try {
                mCameraCaptureSession.stopRepeating();
                mCameraCaptureSession.abortCaptures();
                mCameraCaptureSession.close();
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
            mCameraCaptureSession = null;
        }

        if (mCameraDevice != null) {
            mCameraDevice.close();
            mCameraDevice = null;
        }
        if (null != mImageReader) {
            mImageReader.close();
            mImageReader = null;
        }
        if (mChildHandler != null) {
            mChildHandler.removeCallbacksAndMessages(null);
            mChildHandler = null;
        }
        if (mHandlerThread != null) {
            mHandlerThread.quitSafely();
            mHandlerThread = null;
        }

        mCameraManager = null;
        mSessionStateCallback = null;
        mSessionCaptureCallback = null;
        mStateCallback = null;

    }
}
