package com.tinymonster.ffmpegstudy1;

/**
 * Created by TinyMonster on 2018/5/29.
 */

public class FFmpegDecode {
    public FFmpegDecode(){
        System.loadLibrary("opencv_java");
        System.loadLibrary("ffmdecode");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("yuv");
    }
    public static native int Decode();
    public static native int DecodeFile(String file);
}
