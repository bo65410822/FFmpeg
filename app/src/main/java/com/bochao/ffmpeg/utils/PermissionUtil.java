package com.bochao.ffmpeg.utils;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.List;

import io.reactivex.rxjava3.core.Observable;

/**
 * @author 李占博
 * @description: 权限管理工具类
 * @date :2020/7/14  10:25
 */
public class PermissionUtil {
    /**
     * 权限请求码
     */
    private static final int REQUEST_CODE = 100;

    /**
     * 权限申请成功回到
     */
    public interface IPermissionSuccess {

        /**
         * 成功
         */
        void onSuccess();
    }

    /**
     * 权限申请失败回到
     */
    public interface IPermissionFail {

        /**
         * 失败
         */
        void onFail();
    }

    /**
     * 检测是否需要申请权限
     *
     * @param context     上下文对象
     * @param permissions 权限
     * @param success     成功回调
     */
    public static void checkPermissions(Context context, String[] permissions, IPermissionSuccess success) {
        //6.0才用动态权限
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            success.onSuccess();
            return;
        }
        //未授予的权限存储到permissionList中
        List<String> permissionList = new ArrayList<>();
        //逐个判断你要的权限是否已经通过
        Observable.fromArray(permissions)
                .filter(permission -> ContextCompat.checkSelfPermission(context, permission) != PackageManager.PERMISSION_GRANTED)
                .subscribe(permissionList::add);

        //申请权限
        if (permissionList.size() > 0) {
            ActivityCompat.requestPermissions((Activity) context,permissionList.toArray(new String[0]), REQUEST_CODE);
        } else {
            //权限都已经通过
            success.onSuccess();
        }
    }

    /***
     * 请求权限后回调的方法
     * @param requestCode  自己定义的权限请求码
     * @param grantResults 数组的长度对应的是权限名称数组的长度，数组的数据0表示允许权限，-1表示我们点击了禁止权限
     */
    public static void onRequestPermissionsResult(int requestCode, @NonNull int[] grantResults, IPermissionSuccess success, IPermissionFail fail) {
        //有权限没有通过
        boolean hasPermissionDismiss = false;
        if (REQUEST_CODE == requestCode) {
            for (int i = 0; i < grantResults.length; i++) {
                if (grantResults[i] == -1) {
                    hasPermissionDismiss = true;
                }
            }
        }
        if (hasPermissionDismiss) {
            //权限没有被允许
            fail.onFail();
        } else {
            //全部权限通过
            success.onSuccess();
        }
    }
}
