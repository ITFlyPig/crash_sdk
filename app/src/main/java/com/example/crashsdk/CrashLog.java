package com.example.crashsdk;

import android.text.TextUtils;
import android.util.Log;

import java.util.Set;

/**
 * 创建人   : yuelinwang
 * 创建时间 : 2021/1/26
 * 描述    : 崩溃日志
 */
public class CrashLog {
    public static final String TAG = CrashLog.class.getSimpleName();

    public static void onNativeLog(String nativeLog, String threadName) {
        if (nativeLog == null) {
            nativeLog = "";
        }
        String finalStackTrace = nativeLog + dumpJavaStack(threadName);
        Log.e(TAG, "程序的崩溃堆栈：\n" + finalStackTrace);
    }

    /**
     * 获取Java的堆栈
     * @param threadName
     * @return
     */
    private static String dumpJavaStack(String threadName) {
        Log.e(TAG, "dumpJavaStack: 开始");
        Thread thread = getThreadByName(threadName);
        if (thread == null) {
            thread = getThreadByName("main");
        }
        Log.e(TAG, "dumpJavaStack: 获取到线程：" + (thread == null ? "null" : thread.getName()));
        if (thread == null) {
            return "";
        }
        StringBuilder builder = new StringBuilder();
        for (StackTraceElement stackTraceElement : thread.getStackTrace()) {
            builder.append("at ")
                    .append(stackTraceElement.getClassName())
                    .append(".")
                    .append(stackTraceElement.getMethodName())
                    .append("(")
                    .append(stackTraceElement.getFileName()).append(":").append(stackTraceElement.getLineNumber())
                    .append(")")
                    .append("\n");
        }
        return builder.toString();
    }

    /**
     * 据名称获取线程
     * @param threadName
     * @return
     */
    private static Thread getThreadByName(String threadName) {
        Log.e(TAG, "getThreadByName: threadName = " + (threadName == null ? "null" : threadName));

        Set<Thread> threadSet = Thread.getAllStackTraces().keySet();
        Thread[] threadArray = threadSet.toArray(new Thread[threadSet.size()]);

        Thread theThread = null;
        for(Thread thread : threadArray) {
            Log.e(TAG, "线程名称: " + thread.getName() + " id:" + thread.getId());
            if (thread.getName().equals(threadName)) {
                theThread =  thread;
            }
        }
        return theThread;
    }


}
