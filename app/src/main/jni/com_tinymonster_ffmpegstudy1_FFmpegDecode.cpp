#include <com_tinymonster_ffmpegstudy1_FFmpegDecode.h>
#include <string>
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//编码
extern "C"
{
#include <libyuv.h>
#include <libavcodec/avcodec.h>
//封装格式处理
#include <libavformat/avformat.h>
//像素处理
#include <libswscale/swscale.h>
//视频滤镜
#include <libavfilter/avfilter.h>
}
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ccj",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ccj",FORMAT,##__VA_ARGS__);
using namespace cv;
JNIEXPORT jint JNICALL Java_com_tinymonster_ffmpegstudy1_FFmpegDecode_Decode
  (JNIEnv * env,jclass obj){
  LOGE("%s","1");
  const char input[]="/storage/emulated/0/123.mp4";
  const char *input1=input;
  LOGE("%s","2");
  //1.注册所有组件
  av_register_all();
  LOGE("%s","3");
  //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
  AVFormatContext *pFormatCtx = avformat_alloc_context();
  LOGE("%s","4");
  if (avformat_open_input(&pFormatCtx, input1, NULL, NULL) != 0)
  {
  //无法打开视频
  LOGE("%s","无法打开输入视频文件");
  return 1;
  }
  LOGE("%s","5");
  //3.获取视频文件信息
  if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
  {
  //无法获取视频文件信息
 // FFLOGE("%s","无法获取视频文件信息");
  return 2;
  }
  LOGE("%s","6");
  //获取视频流的索引位置
  //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
  int v_stream_idx = -1;
  int i = 0;
  //number of streams
  for (; i < pFormatCtx->nb_streams; i++)
  {
  //流的类型
  if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
  {
  v_stream_idx = i;
  break;
  }
  }
  LOGE("%s","7");
  if (v_stream_idx == -1)
  {
 // FFLOGE("%s","找不到视频流\n");
  return 3;
  }
  LOGE("%s","8");
  //只有知道视频的编码方式，才能够根据编码方式去找到解码器
  //获取视频流中的编解码上下文
  AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
  //4.根据编解码上下文中的编码id查找对应的解码
  AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  LOGE("%s","9");
  if (pCodec == NULL)
  {
  //FFLOGE("%s","找不到解码器\n");
  return 4;
  }
  LOGE("%s","10");
  //5.打开解码器
  if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
  {
 // FFLOGE("%s","解码器无法打开\n");
  return 5;
  }
  LOGE("%s","11");
  //输出视频信息
LOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
LOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
LOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
LOGI("解码器的名称：%s",pCodec->name);
  //准备读取
  //AVPacket用于存储一帧一帧的压缩数据（H264）
  //缓冲区，开辟空间
  AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));//缓冲区,存储每一帧的压缩数据
  //AVFrame用于存储解码后的像素数据(YUV)
  //内存分配
  AVFrame *pFrame = av_frame_alloc();//每一帧解码后数据
  //YUV420
  AVFrame *pFrameYUV = av_frame_alloc();//每一帧解码后YUV数据
  //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
  //缓冲区分配内存
  uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
  //初始化缓冲区
  avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
  //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
  struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
  pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
  SWS_BICUBIC, NULL, NULL, NULL);
  int got_picture, ret;
  int frame_count = 0;
  LOGE("%s","12");
  //6.一帧一帧的读取压缩数据
  while (av_read_frame(pFormatCtx, packet) >= 0)
  {
  //只要视频压缩数据（根据流的索引位置判断）
  if (packet->stream_index == v_stream_idx)
  {
  LOGE("%s","14");
  //7.解码一帧视频压缩数据，得到视频像素数据
  ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
  if (ret < 0)
  {
//  FFLOGE("%s","解码错误");
  return 6;
  }
 LOGE("%s","15");
  //为0说明解码完成，非0正在解码
  if (got_picture)
  {
  //AVFrame转为像素格式YUV420，宽高
  //2 6输入、输出数据
  //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
  //4 输入数据第一列要转码的位置 从0开始
  //5 输入画面的高度
  sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
  pFrameYUV->data, pFrameYUV->linesize);

  //输出到YUV文件
  //AVFrame像素帧写入文件
  //data解码后的图像像素数据（音频采样数据）
  //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
  //U V 个数是Y的1/4
//  int y_size = pCodecCtx->width * pCodecCtx->height;
//  fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);
//  fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);
//  fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);

  frame_count++;
  LOGI("解码第%d帧",frame_count);
  }
  LOGE("%s","16");
    //释放资源
    av_free_packet(packet);
    }
LOGE("%s","17");
 //   fclose(fp_yuv);

    av_frame_free(&pFrame);
LOGE("%s","18");
    avcodec_close(pCodecCtx);
LOGE("%s","19");
    avformat_free_context(pFormatCtx);
LOGE("%s","20");
  //  env->ReleaseStringUTFChars(input_, input1);
    }
    return 21;
    LOGE("%s","13");
    return 10;
  }




  JNIEXPORT jint JNICALL Java_com_tinymonster_ffmpegstudy1_FFmpegDecode_DecodeFile
    (JNIEnv * env, jclass obj, jstring input_){
      LOGE("%s","1");
      const char *filename = env->GetStringUTFChars(input_, 0);
          AVCodec *pCodec; //解码器指针
          AVCodecContext* pCodecCtx; //ffmpeg解码类的类成员
          AVFrame* pAvFrame; //多媒体帧，保存解码后的数据帧
          AVFormatContext* pFormatCtx; //保存视频流的信息
          av_register_all(); //注册库中所有可用的文件格式和编码器
          pFormatCtx = avformat_alloc_context();
          if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) { //检查文件头部
          LOGE("%s","Can't find the stream!");
              }
          if (avformat_find_stream_info(pFormatCtx,NULL) < 0) { //查找流信息
          LOGE("%s","Can't find the stream information !");
              }
          int videoindex = -1;
          for (int i=0; i < pFormatCtx->nb_streams; ++i) //遍历各个流，找到第一个视频流,并记录该流的编码信息
              {
                  if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                      videoindex = i;
                      break;
                  }
              }
          if (videoindex == -1) {
                  LOGE("%s","Don't find a video stream !");
                  return 1;
              }
          pCodecCtx = pFormatCtx->streams[videoindex]->codec; //得到一个指向视频流的上下文指针
          pCodec = avcodec_find_decoder(pCodecCtx->codec_id); //到该格式的解码器
          if (pCodec == NULL) {
                      LOGE("%s","Cant't find the decoder !");
                      return 2;
               }
          if (avcodec_open2(pCodecCtx,pCodec,NULL) < 0) { //打开解码器
                        LOGE("%s","Can't open the decoder !");
                      return 3;
               }
          pAvFrame = avcodec_alloc_frame(); //分配帧存储空间
          AVFrame* pFrameBGR = avcodec_alloc_frame(); //存储解码后转换的RGB数据
          // 保存BGR，opencv中是按BGR来保存的
                  int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
                  uint8_t *out_buffer = (uint8_t *)av_malloc(size);
                  avpicture_fill((AVPicture *)pFrameBGR, out_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
                  AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket));
                  LOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
                  LOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
                  LOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
                  LOGI("解码器的名称：%s",pCodec->name);
                  struct SwsContext *img_convert_ctx;
                  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
                  //opencv
                  cv::Mat pCvMat;
                  pCvMat.create(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
                  int ret;
                  int got_picture;
                  //读取每一帧
                  int frame_count = 0;
                      while (av_read_frame(pFormatCtx, packet) >= 0)
                      {
                          if(packet->stream_index==videoindex)
                          {
                              ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
                              if(ret < 0)
                              {
                                  printf("Decode Error.（解码错误）\n");
                                  return 4;
                              }
                              LOGI("解码第%d帧",frame_count);
                              if (got_picture)
                              {
                                  //YUV to RGB
                                  sws_scale(img_convert_ctx, (const uint8_t* const*)pAvFrame->data, pAvFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data, pFrameBGR->linesize);
                                  memcpy(pCvMat.data, out_buffer, size);//拷贝
                                  frame_count++;
                                  LOGI("解码第%d帧",frame_count);
                              }
                          }
                          av_free_packet(packet);
                      }
                  av_free(out_buffer);
                  av_free(pFrameBGR);
                  av_free(pAvFrame);
                  avcodec_close(pCodecCtx);
                  avformat_close_input(&pFormatCtx);
                  sws_freeContext(img_convert_ctx);
                  return 0;
    }