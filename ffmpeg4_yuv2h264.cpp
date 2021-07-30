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

    AVFormatContext *fmtCtx = NULL;
    AVOutputFormat *outfmt = NULL;
    AVStream *vstream = NULL;
    AVCodecContext *codecCtx = NULL;
    AVCodec *codec = NULL;
    AVPacket *pkt = av_packet_alloc();
    uint8_t *picture_buf = NULL;
    AVFrame *picframe = NULL;
    size_t size;
    int ret = 1;

    FILE *fp = fopen("/home/zjw/workspace/myffmpeg/relase.yuv", "rb");
    if (fp == NULL)
    {
        cout << "文件打开失败" << endl;
        return -1;
    }

    do
    {
        int in_w = 1920, in_h = 1080, frameCnt = 490;
        const char *of = "/home/zjw/workspace/myffmpeg/relase.h264";
        if (avformat_alloc_output_context2(&fmtCtx, NULL, NULL, of) < 0)
        {
            cout<<"不能创建一个输出文件上下文"<<endl;
            break;
        }
        outfmt=fmtCtx->oformat;
        if(avio_open(&fmtCtx->pb,of,AVIO_FLAG_READ_WRITE)<0){
            cout<<"输出文件打开失败"<<endl;
            break;
        }

        //创建h264视频流，并设置参数
        vstream = avformat_new_stream(fmtCtx,codec);
        if(vstream ==NULL){
            printf("failed create new video stream.\n");
            break;
        }

        vstream->time_base.den=25;
        vstream->time_base.num=1;

        AVCodecParameters *codecPara = fmtCtx->streams[vstream->index]->codecpar;
        codecPara->codec_type=AVMEDIA_TYPE_VIDEO;
        codecPara->width=in_w;
        codecPara->height=in_h;

        codec = avcodec_find_encoder(outfmt->video_codec);
        if(codec==NULL){
            cout<<"找不到其他解码"<<endl;
            break;
        }

        //设置编码器内容
        codecCtx=avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx,codecPara);
        if(codecCtx==NULL){
            cout<<"不能申请解码器空间"<<endl;
            break;
        }

        codecCtx->codec_id = outfmt->video_codec;
        codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        codecCtx->width         = in_w;
        codecCtx->height        = in_h;
        codecCtx->time_base.num = 1;
        codecCtx->time_base.den = 25;
        codecCtx->bit_rate      = 400000;
        codecCtx->gop_size      = 12;

        if (codecCtx->codec_id == AV_CODEC_ID_H264) {
            codecCtx->qmin      = 10;
            codecCtx->qmax      = 51;
            codecCtx->qcompress = (float)0.6;
        }

        if (codecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO){
            codecCtx->max_b_frames = 2;
        }
            
        if (codecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO){
            codecCtx->mb_decision = 2;
        }
            
        //打开编码器
        if(avcodec_open2(codecCtx,codec,NULL)<0){
            cout<<"打开编码器失败"<<endl;
        }
        //打印
        av_dump_format(fmtCtx,0,of,1);

        //初始化帧
        picframe         = av_frame_alloc();
        picframe->width  = codecCtx->width;
        picframe->height = codecCtx->height;
        picframe->format = codecCtx->pix_fmt;
        size            = (size_t)av_image_get_buffer_size(codecCtx->pix_fmt,codecCtx->width,codecCtx->height,1);
        picture_buf     = (uint8_t *)av_malloc(size);
        av_image_fill_arrays(picframe->data,picframe->linesize,
                             picture_buf,codecCtx->pix_fmt,
                             codecCtx->width,codecCtx->height,1);

        //写文件头
        ret = avformat_write_header(fmtCtx,NULL);

        int y_size = codecCtx->width*codecCtx->height;
        av_new_packet(pkt,(int)(size*3));

        for(int i=0;i<frameCnt;i++){
            //读入yuv
            if(fread(picture_buf,1,(unsigned long)(y_size*3/2),fp)<=0){
                cout<<"读文件失败"<<endl;
                return -1;
            }else if(feof(fp)){
                break;
            }

            picframe->data[0]=picture_buf;
            picframe->data[1]=picture_buf+y_size;
            picframe->data[2]=picture_buf+y_size*5/4;
            picframe->pts = i;

            if(avcodec_send_frame(codecCtx,picframe)>=0){
                while (avcodec_receive_packet(codecCtx,pkt)>=0)
                {
                    printf("encoder success!\n");
                    // parpare packet for muxing
                    pkt->stream_index = vstream->index;
                    av_packet_rescale_ts(pkt, codecCtx->time_base, vstream->time_base);
                    pkt->pos = -1;
                    ret     = av_interleaved_write_frame(fmtCtx, pkt);
                    cout<<ret<<endl;
                    
                    if(ret<0){
                        // printf("error is: %s.\n",av_err2str(ret));
                    }
                    av_packet_unref(pkt);//刷新缓存
                }
            }
        }
        // ret = flush_encoder(fmtCtx,codecCtx,vstream->index);
        // if(ret<0){
        //     cout<<"解码失败"<<endl;
        //     break;
        // }
        av_write_trailer(fmtCtx);


    } while (0);
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    av_free(picframe);
    av_free(picture_buf);
    if(fmtCtx){
        avio_close(fmtCtx->pb);
        avformat_free_context(fmtCtx);
    }

    fclose(fp);
    return 0;
}
