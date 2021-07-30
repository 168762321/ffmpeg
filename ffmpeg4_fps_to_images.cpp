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
/*
    解码视频流，获得视频的帧数保存图片
*/

using namespace std;

void saveFrame(AVFrame *pfname, int w, int h, int fnum)
{
    FILE * pfile;
    char szFilename[50];
    int y;

    //打开文件
    sprintf(szFilename, "/home/zjw/workspace/myffmpeg/images/frame%d.ppm", fnum);
    pfile = fopen(szFilename,"wb");
    if (pfile == NULL)
    {
        return;
    }

    //写入文件头
    // PPM图像格式分为两部分，分别为头部分和图像数据部分。
    // 头部分：由3部分组成，通过换行或空格进行分割，一般PPM的标准是空格。
    // 第1部分：P3或P6，指明PPM的编码格式，
    // 第2部分：图像的宽度和高度，通过ASCII表示，1个空格分割开，
    // 第3部分：最大像素值，0-255字节表示。
    fprintf(pfile, "P6\n%d %d\n255\n", w, h);
    //写入像素
    for (y = 0; y < h; y++)
    {
        fwrite(pfname->data[0] + y * pfname->linesize[0], 1, w * 3, pfile);
    }

    //关闭文件
    fclose(pfile);
    cout<<"第"<<fnum<<"张图"<<endl;
}

int main()
{
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
    AVFrame *rgbFrame = av_frame_alloc();

    //创建AVFormatcontext结构体
    fmtCtx = avformat_alloc_context();

    //打开文件
    if (avformat_open_input(&fmtCtx, filePath, NULL, NULL) < 0)
    {
        cout << "avformatcontext结构体创建失败" << endl;
    }

    //获得视频流
    if (avformat_find_stream_info(fmtCtx, NULL) < 0)
    {
        cout << "没有获得视频流" << endl;
    }

    for (int i = 0; i < fmtCtx->nb_streams; i++)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break; //找到视频流就退出
        }
    }

    //如果videoStream为-1 说明没有找到视频流
    if (videoStreamIndex == -1)
    {
        printf("找不到视频流");
    }

    //打印输入和输出信息：长度 比特率 流格式等
    av_dump_format(fmtCtx, 0, filePath, 0);

    //查找解码器
    avCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;
    codec = avcodec_find_decoder(avCodecPara->codec_id);
    if (codec == NULL)
    {
        cout << "没有找到对应的解码器" << endl;
    }

    //创建解码器内容
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, avCodecPara);
    if (codecCtx == NULL)
    {
        cout << "创建解码器空间失败" << endl;
    }

    //打开解码器
    if ((ret = avcodec_open2(codecCtx, codec, NULL)) < 0)
    {
        cout << "不能打开解码器" << endl;
    }

    //设置数据转换参数
    struct SwsContext *img_ctx = sws_getContext(
        codecCtx->width, codecCtx->height, AV_PIX_FMT_YUVJ420P, //源地址长宽以及数据格式
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB32,    //目的地址长宽以及数据格式
        SWS_FAST_BILINEAR, NULL, NULL, NULL);                   //算法类型  AV_PIX_FMT_YUVJ420P   AV_PIX_FMT_BGR24

    //分配空间
    //一帧图像数据大小
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height, 1);
    unsigned char *out_buffer = (unsigned char *)av_malloc(numBytes * sizeof(unsigned char));

    //分配AVPacket
    int i = 0;
    pkt = av_packet_alloc();                                //分配packet
    av_new_packet(pkt, codecCtx->width * codecCtx->height); //调整packet的数据

    //会将pFrameRGB的数据按RGB格式自动"关联"到buffer  即pFrameRGB中的数据改变了
    //out_buffer中的数据也会相应的改变
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, out_buffer, AV_PIX_FMT_RGB32,
                         codecCtx->width, codecCtx->height, 1);

    //读取视频信息
    while (av_read_frame(fmtCtx, pkt) >= 0)
    {
        if (pkt->stream_index == videoStreamIndex)
        {
            if(avcodec_send_packet(codecCtx,pkt)==0){
                while (avcodec_receive_frame(codecCtx,yuvFrame)==0)
                {
                    if (++i <= 50 ){
                        sws_scale(img_ctx,
                                (const uint8_t* const*)yuvFrame->data,
                                yuvFrame->linesize,
                                0,
                                codecCtx->height,
                                rgbFrame->data,
                                rgbFrame->linesize);
                        saveFrame(rgbFrame, codecCtx->width, codecCtx->height, i);
                    }
                }
                
            }
        }
        av_packet_unref(pkt); //重置pkt内容
    }

    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_parameters_free(&avCodecPara);
    avformat_close_input(&fmtCtx);
    av_frame_free(&yuvFrame);
    av_frame_free(&rgbFrame);


    return ret;
}
