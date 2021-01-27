package com.example.crashsdk;

import android.util.Log;

/**
 * 创建人   : yuelinwang
 * 创建时间 : 2021/1/26
 * 描述    : 崩溃日志
 */
public class CrashLog {
    public static final String TAG = CrashLog.class.getSimpleName();

    public static void onNativeLog(String nativeLog) {
        if (nativeLog == null) {
            nativeLog = "";
        }
        Log.e(TAG, "onNativeLog 接收到native的崩溃日志: " + nativeLog);
    }
}
