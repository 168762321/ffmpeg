#include <iostream>
#include <cmath>
extern "C"
{
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

using namespace std;

int main()
{
    FILE *nf = fopen("/home/zjw/workspace/myffmpeg/relase.yuv", "w+b");
    if (nf == NULL)
    {
        cout << "打不开文件" << endl;
        return -1;
    }

    char filePath[] = "/home/zjw/workspace/myffmpeg/jay.mp4"; //文件地址
    int videoStreamIndex = -1;                                //视频流所在流序列中的索引
    int ret = 0;                                              //默认返回值

    //需要的变量名并初始化
    AVFormatContext *fmtCtx = NULL;
    AVPacket *pkt = NULL;
    AVCodecContext *codecCtx = NULL;
    AVCodecParameters *avCodecPara = NULL;
    AVCodec *codec = NULL;
    AVFrame *yuvFrame = av_frame_alloc();

    fmtCtx = avformat_alloc_context();
    ret = avformat_open_input(&fmtCtx, filePath, NULL, NULL);
    if (ret != 0)
    {
        cout << "创建fmt上下文失败" << endl;
        return -1;
    }
    ret = avformat_find_stream_info(fmtCtx, NULL);
    if (ret < 0)
    {
        cout << "没有找到文件流信息" << endl;
        return -1;
    }

    //获得视频索引
    for (int i = 0; i < fmtCtx->nb_streams; i++)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    //判断视频索引
    if (videoStreamIndex == -1)
    {
        cout << "没有找到视频索引，终止程序" << endl;
        return -1;
    }

    //输出比特流
    av_dump_format(fmtCtx, NULL, filePath, 0);

    //查找解码器参数
    avCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;
   
    codec = avcodec_find_decoder(avCodecPara->codec_id);

    if (codec == NULL)
    {
        cout << "找不到解码器" << endl;
        return -1;
    }
 //根据解码器参数来创建解码器内容
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, avCodecPara);
    if (codecCtx == NULL) {
        cout<<"Cannot alloc context."<<endl;;
        return -1;
    }
    
    //打开解码器
    ret = avcodec_open2(codecCtx, codec, NULL);
    
    if (ret < 0)
    {
        cout << "打不开解码器" << endl;
        return -1;
    }
    int w = codecCtx->width;  //视频宽
    int h = codecCtx->height; //视频高

    //创建packet
    pkt = av_packet_alloc();
    if (!pkt)
    {
        cout << "pkt 创建失败" << endl;
        return -1;
    }

    if (av_new_packet(pkt, w * h) != 0)
    {
        cout << "创建新packet失败" << endl;
        return -1;
    }

    //读取视频文件
    while (av_read_frame(fmtCtx, pkt) >= 0)
    {
        if (pkt->stream_index == videoStreamIndex)
        {
            if (avcodec_send_packet(codecCtx, pkt) == 0)
            {
                while (avcodec_receive_frame(codecCtx, yuvFrame) == 0)
                {
                    fwrite(yuvFrame->data[0], 1, w * h, nf);
                    fwrite(yuvFrame->data[1], 1, w * h / 4, nf);
                    fwrite(yuvFrame->data[2], 1, w * h / 4, nf);
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_parameters_free(&avCodecPara);
    //avformat_close_input(&fmtCtx);
    //avformat_free_context(fmtCtx);
    av_frame_free(&yuvFrame);
    cout<<"完成yuv转换"<<endl;
    return ret;
}
