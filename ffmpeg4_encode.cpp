#include <iostream>
#include <cmath>
extern "C"{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
/*
    解码视频流，获得视频的帧数
*/



using namespace std;

int main()
{
    char filePath[]       = "/home/zjw/workspace/myffmpeg/jay.mp4";//文件地址
    int  videoStreamIndex = -1;//视频流所在流序列中的索引
    int ret=0;//默认返回值

    //需要的变量名并初始化
    AVFormatContext *fmtCtx=NULL;
    AVPacket *pkt =NULL;
    AVCodecContext *codecCtx=NULL;
    AVCodecParameters *avCodecPara=NULL;
    AVCodec *codec=NULL;

    
    //创建AVFormatcontext结构体
    fmtCtx=avformat_alloc_context();
    if(avformat_open_input(&fmtCtx,filePath,NULL,NULL)<0)
    {
        cout<<"文件流创建失败"<<endl;
        
    }
    //获得视频流
    if (avformat_find_stream_info(fmtCtx,NULL)<0)
    {
        cout<<"没有获得视频流"<<endl;
       
    }
    
    for(int i=0;i<fmtCtx->nb_streams;i++){
        if (fmtCtx->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;//找到视频流就退出
        }
    }

    //如果videoStream为-1 说明没有找到视频流
    if (videoStreamIndex == -1) {
        printf("找不到视频流");
    
    }

    //打印输入和输出信息：长度 比特率 流格式等
    av_dump_format(fmtCtx, 0, filePath, 0);


    //查找解码器
    avCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;
    codec = avcodec_find_decoder(avCodecPara->codec_id);
    if(codec==NULL){
        cout<<"没有找到对应的解码器"<<endl;
    
    }

    //创建解码器内容
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx,avCodecPara);
    if(codecCtx == NULL){
        cout<<"创建解码器空间失败"<<endl;
        
    }

    //打开解码器
    if((ret=avcodec_open2(codecCtx,codec,NULL))<0){
        cout<<"不能打开解码器"<<endl;
    }

    //分配AVPacket
    int i=0;
    pkt=av_packet_alloc();//分配packet
    av_new_packet(pkt,codecCtx->width*codecCtx->height); //调整packet的数据

    //读取视频信息
    while(av_read_frame(fmtCtx,pkt)>=0){
        if(pkt->stream_index==videoStreamIndex){
            i++;
        }
        av_packet_unref(pkt);//重置pkt内容
    }
    cout<<"输出i："<<i<<endl;

    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_parameters_free(&avCodecPara);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);

    av_free(codec);

    return ret;
}
