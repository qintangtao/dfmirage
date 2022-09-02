#include <QCoreApplication>
#include <QDebug>
#include <QImage>
#include <QThread>
#include <windows.h>
#include <winuser.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <mmsystem.h>
#include "Decoder/H264Encoder.h"
#include "EasyIPCameraAPI.h"
#include "MirrorDriverClient.h"

extern "C"
{
    #include "libavcodec/codec.h"
    #include "libavcodec/avcodec.h"
    #include "libavcodec/avfft.h"
    #include "libavdevice/avdevice.h"
    #include "libswscale/swscale.h"
    #include "libavformat/avformat.h"
    #include "libavformat/avio.h"
    #include "libavutil/common.h"
    #include "libavutil/avstring.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/time.h"
    #include "libavutil/frame.h"
    #include "libswresample/swresample.h"
    #include "libavfilter/avfilter.h"
    #include "libavutil/avassert.h"
    #include "libavutil/avstring.h"
    #include "libavutil/avutil.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/intreadwrite.h"
    #include "libavutil/fifo.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/opt.h"
    #include "libavutil/parseutils.h"
    #include "libavutil/pixdesc.h"
    #include "libavutil/pixfmt.h"
    #include "libavutil/hwcontext.h"
}

//#define ENABLED_FFMPEG_ENCODER

using namespace std;

//exe名称固定（EasyIPCamera_RTSP） KEY_EASYIPCAMERA和名称绑定了
#define KEY_EASYIPCAMERA "6D72754B7A4969576B5A75413362465A706B3337634F704659584E35535642445957316C636D4666556C52545543356C654756584446616732504467523246326157346D516D466962334E68514449774D545A4659584E355247467964326C75564756686257566863336B3D"

typedef struct tagSOURCE_CHANNEL_T
{
    int		id;
    char		name[36];
    char		source_uri[128];
    int		pushStream;

    EASY_MEDIA_INFO_T	mediaInfo;
    void*	anyHandle;

    HANDLE thread;
    bool		bThreadLiving;

    H264Encoder *encoder;

    int width;
    int height;
    int fps;
}SOURCE_CHANNEL_T;

static bool GetLocalIP(char* ip)
{
    if ( !ip )
    {
        return false;
    }
#ifdef _WIN32
    HOSTENT* host;
    WSADATA wsaData;
    int ret=WSAStartup(MAKEWORD(2,2),&wsaData);
    if (ret!=0)
    {
        return false;
    }
#else
    struct hostent *host;
#endif

    char hostname[256];
    ret=gethostname(hostname,sizeof(hostname));
    if (ret==-1)
    {
        return false;
    }

    host=gethostbyname(hostname);
    if (host==0)
    {
        return false;
    }
    strcpy(ip,inet_ntoa(*(in_addr*)*host->h_addr_list));

#ifdef _WIN32
        WSACleanup( );
#endif

    return true;
}

static int GetAvaliblePort(int nPort)
{
#ifdef _WIN32
    HOSTENT* host;
    WSADATA wsaData;
    int ret=WSAStartup(MAKEWORD(2,2),&wsaData);
    if (ret!=0)
    {
        return -1;
    }
#endif

    int port = nPort;
    int fd = 0;
    sockaddr_in addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
#ifdef _WIN32
    addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
#endif
    while(bind(fd, (sockaddr *)(&addr), sizeof(sockaddr_in)) < 0)
    {
        printf("port %d has been used.\n", port);
        port++;
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        fd = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
#ifdef _WIN32
        addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
        inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
#endif
        Sleep(1);
    }
#ifdef _WIN32
    closesocket(fd);
    WSACleanup( );
#else
    close(fd);
#endif
    return port;
}

static void __FFmpegLog_Callback(void* ptr, int level, const char* fmt, va_list vl)
{
    va_list vl2;
    char *line = (char*)malloc(1024 * sizeof(char));
    static int print_prefix = 1;
    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, 1024, &print_prefix);
    va_end(vl2);
    line[1023] = '\0';
    if (level > AV_LOG_WARNING) {
        qDebug("%s", line);
    } else{
        qWarning("%s", line);
    }
    free(line);
}

static int __EasyIPCamera_Callback(Easy_I32 channelId, EASY_IPCAMERA_STATE_T channelState, EASY_MEDIA_INFO_T *mediaInfo, void *userPtr)
{
    // 	1. 调用 EasyIPCamera_Startup 设置监听端口、回调函数和自定义数据指针
    // 		2. 启动后，程序进入监听状态
    // 		2.1		接收到客户端请求, 回调 状态:EASY_IPCAMERA_STATE_REQUEST_MEDIA_INFO          上层程序在填充完mediainfo后，返回0, 则EasyIpCamera响应客户端ok
    // 		2.2		EasyIPCamera回调状态 EASY_IPCAMERA_STATE_REQUEST_PLAY_STREAM , 则表示rtsp交互完成, 开始发送流, 上层程序调用EasyIpCamera_PushFrame 发送帧数据
    // 		2.3		EasyIPCamera回调状态 EASY_IPCAMERA_STATE_REQUEST_STOP_STREAM , 则表示客户端已发送teaardown, 要求停止发送帧数据
    // 		3.	调用 EasyIpCamera_Shutdown(), 关闭EasyIPCamera，释放相关资源

    SOURCE_CHANNEL_T	*pChannel = (SOURCE_CHANNEL_T *)userPtr;
    if (channelId < 0)		return -1;

    if (channelState == EASY_IPCAMERA_STATE_REQUEST_MEDIA_INFO)
    {
        printf("[channel %d] Get media info...\n", channelId);
        memcpy(mediaInfo, &pChannel[channelId].mediaInfo, sizeof(EASY_MEDIA_INFO_T));
    }
    else if (channelState == EASY_IPCAMERA_STATE_REQUEST_PLAY_STREAM)
    {
        printf("[channel %d] PlayStream...\n", channelId);
        pChannel[channelId].pushStream = 0x01;
    }
    else if (channelState == EASY_IPCAMERA_STATE_REQUEST_STOP_STREAM)
    {
        printf("channel[%d] StopStream...\n", channelId);
        pChannel[channelId].pushStream = 0x00;
    }

    return 0;
}

unsigned int _stdcall  CaptureScreenThread(void* lParam)
{
    SOURCE_CHANNEL_T* pChannelInfo = (SOURCE_CHANNEL_T*)lParam;
    if (!pChannelInfo)
    {
        return -1;
    }

    MirrorDriverClient *pClient = (MirrorDriverClient*)pChannelInfo->anyHandle;
    int nChannelId = pChannelInfo->id;

    uint8_t *src_data[4];
    int src_linesize[4];

    uint8_t *dst_data[4];
    int dst_linesize[4];

    SwsContext *sws_ctx;

    enum AVPixelFormat srcFormat = AV_PIX_FMT_RGB32;
    enum AVPixelFormat dstFormat = AV_PIX_FMT_YUV420P;

    int width = pChannelInfo->width;
    int height = pChannelInfo->height;

    int ret;
    int index = 0;
    int yuvSize = width*height*1.5;
    int dealy = 1000000 / pChannelInfo->fps / 3;

    int datasize;
    bool keyframe;

    uchar *encode_data;

    DWORD Start;
    DWORD Stop;

    av_image_alloc(src_data, src_linesize, width, height, srcFormat, 1);
    av_image_alloc(dst_data, dst_linesize, width, height, dstFormat, 1);

    sws_ctx = sws_getContext(width,height,srcFormat, width,height,dstFormat,SWS_BICUBIC,NULL,NULL,NULL);

    src_data[0] = (uint8_t *)pClient->getBuffer();

    while (pChannelInfo&&pChannelInfo->bThreadLiving)
    {
#ifdef ENABLED_FFMPEG_ENCODER
#else
        QThread::usleep(dealy);
#endif
        if (pChannelInfo->pushStream == 0) {
            QThread::usleep(dealy);
            continue;
        }

        if (index == 0)
            Start=GetTickCount();

        sws_scale(sws_ctx,src_data,src_linesize,0,height, dst_data, dst_linesize);

        datasize=0;
        keyframe=false;

        encode_data = pChannelInfo->encoder->Encoder((unsigned char *)dst_data[0], yuvSize, datasize, keyframe);

        if ((NULL != encode_data) && (datasize > 0)) {
            EASY_AV_Frame	frame;
            frame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
            frame.pBuffer = (Easy_U8*)encode_data+4;
            frame.u32AVFrameLen =  datasize-4;
            frame.u32VFrameType   = ( keyframe ? EASY_SDK_VIDEO_FRAME_I : EASY_SDK_VIDEO_FRAME_P);
            frame.u32TimestampSec = 0;
            frame.u32TimestampUsec = 0;
            EasyIPCamera_PushFrame(nChannelId,  &frame);
        }

        if (++index>= 600)
        {
            DWORD Stop=GetTickCount();
            printf("ChannelId:%d, Start:%d, Stop:%d, Diff:%d, Count:%d, FPS:%d, \r\n", nChannelId, Start, Stop, (Stop-Start)/1000,index, (index/((Stop-Start)/1000)));
            index = 0;
        }
    }

    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);

    return 0;
}


int main(int argc, char *argv[])
{
    int fps = 60;
    int OutputCount = 1;
    char* ConfigName=	"channel";

    int isActivated = 0 ;
    char szIP[16] = {0};
    int ch;

    uchar  sps[100];
    uchar  pps[100];
    long spslen;
    long ppslen;

    enum AVHWDeviceType type;

    av_log_set_level(AV_LOG_TRACE);
    av_log_set_callback(__FFmpegLog_Callback);
    fprintf(stderr, "Available device types:");
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
    fprintf(stderr, "\n");

    MirrorDriverClient client;
    if (client.getBuffer() == NULL)
    {
        printf("MirrorDriverClient open failed\n");
        printf("Press Enter exit...\n");
        getchar();
        exit(1);
    }

    int activate_ret = EasyIPCamera_Activate(KEY_EASYIPCAMERA);
    if (activate_ret < 0)
    {
        printf("EasyIPCamera_Activate: %d\n", activate_ret);
        printf("Press Enter exit...\n");
        getchar();
        exit(1);
    }

   bool bSuc = GetLocalIP(szIP);
    if (!bSuc)
    {
        printf("GetLocalIP failed\n");
        printf("Press Enter exit...\n");
        getchar();
        return 0;
    }

    int nServerPort = GetAvaliblePort(554);

    //int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    //int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    SOURCE_CHANNEL_T*	channels = new SOURCE_CHANNEL_T[OutputCount];
    memset(&channels[0], 0x00, sizeof(SOURCE_CHANNEL_T) * OutputCount);
    char rtsp_url[128];
    char sFileName[256] = {0,};
    for (int i=0; i<OutputCount; i++)
    {
        channels[i].id = i;
        sprintf(channels[i].name, "%s%d",  ConfigName, i);
        sprintf(rtsp_url, "rtsp://%s:%d/%s", szIP, nServerPort, channels[i].name);

        strcpy(channels[i].source_uri, rtsp_url);

        channels[i].width = width;
        channels[i].height = height;
        channels[i].fps = fps;

        channels[i].anyHandle = &client;

        channels[i].encoder = new H264Encoder();
        channels[i].encoder->Init(width, height, fps, 30, 4096, 1, 20, 0);
        channels[i].encoder->GetSPSAndPPS(sps, spslen, pps, ppslen);

        memset(&channels[i].mediaInfo, 0x00, sizeof(EASY_MEDIA_INFO_T));
        channels[i].mediaInfo.u32VideoCodec =   EASY_SDK_VIDEO_CODEC_H264;
        channels[i].mediaInfo.u32VideoFps = fps;
        channels[i].mediaInfo.u32AudioCodec = EASY_SDK_AUDIO_CODEC_AAC;
        channels[i].mediaInfo.u32AudioChannel = 2;
        channels[i].mediaInfo.u32AudioSamplerate = 44100;//44100;//
        channels[i].mediaInfo.u32AudioBitsPerSample = 16;
        channels[i].mediaInfo.u32VpsLength = 0;// Just H265 need Vps
        channels[i].mediaInfo.u32SeiLength = 0;//no sei
        channels[i].mediaInfo.u32SpsLength = spslen;			/* 视频sps帧长度 */
        channels[i].mediaInfo.u32PpsLength = ppslen;			/* 视频pps帧长度 */
        memcpy(channels[i].mediaInfo.u8Sps, sps,  spslen);			/* 视频sps帧内容 */
        memcpy(channels[i].mediaInfo.u8Pps, pps, ppslen);				/* 视频sps帧内容 */

        channels[i].thread = (HANDLE)_beginthreadex(NULL, 0, CaptureScreenThread, (void*)&channels[i],0,0);
        channels[i].bThreadLiving		= true;

        printf("rtspserver output stream url:	   %s\n", rtsp_url);
    }

    LIVE_CHANNEL_INFO_T*	liveChannels = new LIVE_CHANNEL_INFO_T[OutputCount];
    memset(&liveChannels[0], 0x00, sizeof(LIVE_CHANNEL_INFO_T)*OutputCount);
    for (int i=0; i<OutputCount; i++)
    {
        liveChannels[i].id = channels[i].id;
        strcpy(liveChannels[i].name, channels[i].name);
    }
   EasyIPCamera_Startup(nServerPort, AUTHENTICATION_TYPE_BASIC,"", (unsigned char*)"", (unsigned char*)"", __EasyIPCamera_Callback, (void *)&channels[0], &liveChannels[0], OutputCount);

    printf("Press Enter exit...\n");
    getchar();
    getchar();
    getchar();
    getchar();
    getchar();
    getchar();

    for(int nI=0; nI<OutputCount; nI++)
    {
        channels[nI].bThreadLiving = false;
    }
    Sleep(200);

    EasyIPCamera_Shutdown();

    delete[] channels;
    delete[] liveChannels;
    return 0;
}


#if 0

static void encode(MirrorDriverClient *client)
{
#if 0
    DWORD Start=GetTickCount();
    int idx = 0;
    while(idx++ < 60)
    {
        QImage image((uchar *)m_screenBuffer, width, height, QImage::Format_RGB32);
        image.save(QString("aaa_%1.png").arg(idx++));
    }
    DWORD Stop=GetTickCount();
    printf("Start:%d, Stop:%d, Diff:%d\r\n", Start, Stop, (Stop-Start)/1000);

#else

    uint8_t *YUV_BUF_420[4];
    int linesizes[4];

    uint8_t *RGB32_BUF[4];
    int rgbLinesize[4];

    enum AVPixelFormat srcFormat = AV_PIX_FMT_RGB32;
    enum AVPixelFormat dstFormat = AV_PIX_FMT_YUV420P;
    //enum AVPixelFormat dstFormat = AV_PIX_FMT_NV12;

    av_image_alloc(RGB32_BUF,rgbLinesize,width,height,srcFormat,1);
    av_image_alloc(YUV_BUF_420,linesizes,width,height,dstFormat,1);


    SwsContext *m_pImageConvertCtx = sws_getContext(width,height,srcFormat, width,height,dstFormat,SWS_BICUBIC,NULL,NULL,NULL);

    unsigned char *yuvBuf = (uint8_t *)malloc(width*height*1.5);
    int yuvSize = width*height*1.5;
    int ySize = width*height;
    int uSize = ySize/4;

    EASY_AV_Frame	easyFrame;
    memset(&easyFrame, 0x00, sizeof(EASY_AV_Frame));
    easyFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;

#ifdef ENABLED_FFMPEG_ENCODER
     // AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    const AVCodec *pCodec = avcodec_find_encoder_by_name("libx264");
    //const AVCodec *pCodec = avcodec_find_encoder_by_name("h264_qsv");
     //AVCodec *pCodec = avcodec_find_encoder_by_name("h264_d3d11va");
    if (NULL == pCodec)
        return;

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);

#if 1
    /* put sample parameters */
        //pCodecCtx->bit_rate = 400000;
        /* resolution must be a multiple of two */
        pCodecCtx->frame_number=1;
        pCodecCtx->width = width;
        pCodecCtx->height = height;
        /* frames per second */
        pCodecCtx->time_base = (AVRational){1, fps};
        pCodecCtx->framerate = (AVRational){fps, 1};

        /* emit one intra frame every ten frames
         * check frame pict_type before passing frame
         * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
         * then gop_size is ignored and the output of encoder
         * will always be I frame irrespective to gop_size
         */
        //pCodecCtx->gop_size = 10;
        pCodecCtx->max_b_frames = 1;
        pCodecCtx->pix_fmt = pix_fmt;
        pCodecCtx->thread_count = 1;
#else
    pCodecCtx->bit_rate =3000000;   //初始化为0
    pCodecCtx->frame_number=1;  //每包一个视频帧
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = pix_fmt;
    pCodecCtx->time_base = {1, fps};
    //pCodecCtx->max_b_frames=1;
    pCodecCtx->thread_count = 1;
#endif

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        qDebug() << "Could not open codec.\n";
        return;
    }

    qDebug() << "Success open codec.\n";

    // 用来存放编码后的数据（h264)
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "could not allocate audio packet";
        return;
    }

    // 用来存放编码前的数据（yuv）
    AVFrame * frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "could not allocate audio frame";
        return;
    }

    // 保证 frame 里就是一帧 yuv 数据
    frame->width = pCodecCtx->width;
    frame->height = pCodecCtx->height;
    frame->format = pCodecCtx->pix_fmt;
    frame->pts = 0;

    // 创建输入缓冲区 方法一
    int ret = av_image_alloc(frame->data, frame->linesize, width, height, pCodecCtx->pix_fmt, 1);
    if (ret < 0) {
        qDebug() << "could not allocate audio data buffers: ";
        return;
    }

    /*
    // 利用width、height、format创建frame的数据缓冲区，利用width、height、format可以算出一帧大小
    ret = av_frame_get_buffer(frame, 1);
    if (ret < 0) {
    ERRBUF(ret);
    qDebug() << "could not allocate audio data buffers: " << errbuf;
    goto end;
    }
    */

    /*
    // 创建输入缓冲区 方法二
    buf = (uint8_t *)av_malloc(image_size);
    ret = av_image_fill_arrays(frame->data, frame->linesize, buf, in.pixFmt, in.width, in.height, 1);
    if (ret < 0) {
    ERRBUF(ret);
    qDebug() << "could not allocate audio data buffers: " << errbuf;
    goto end;
    }
    */
#endif

    qDebug() << "Success.\n";

    static int g_index = 0;

    int nWidhtHeightBuf = (width*height*3)>>1;
    uchar *pDataBuffer = new uchar[nWidhtHeightBuf];
    DWORD Start=GetTickCount();
    int dealy = 1000000 / fps / 3;
    while (true)
    {
#ifdef ENABLED_FFMPEG_ENCODER
#else
        QThread::usleep(dealy);
#endif
        if (!m_isSendFrame) {
            QThread::usleep(dealy);
            continue;
        }

        if (g_index == 0)
            Start=GetTickCount();

        RGB32_BUF[0] = (uint8_t *)client->getBuffer();//

#ifdef ENABLED_FFMPEG_ENCODER
        sws_scale(m_pImageConvertCtx,RGB32_BUF,rgbLinesize,0,height, frame->data,frame->linesize);

        int ret = avcodec_send_frame(pCodecCtx, frame);
        if (ret < 0) {
            qDebug() << "error sending the frame to the codec: ";
            continue;
        }

        while (true) {
            // 从编码器中获取编码后的数据
            ret = avcodec_receive_packet(pCodecCtx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                qDebug() << "AVERROR_EOF " << ret << AVERROR_EOF << AVERROR(EAGAIN);
                break;
            } else if (ret < 0) {
                qDebug() << "error encode audio frame: ";
                break;
            }
            //outFile.write((const char *)pkt->data, pkt->size);

            {
                easyFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
                easyFrame.pBuffer = (Easy_U8*)pkt->data+4;
                easyFrame.u32AVFrameLen =  pkt->size-4;
                easyFrame.u32VFrameType   = ( pkt->flags && AV_PKT_FLAG_KEY ? EASY_SDK_VIDEO_FRAME_I : EASY_SDK_VIDEO_FRAME_P);
                EasyIPCamera_PushFrame(0,  &easyFrame);
            }

            av_packet_unref(pkt);
        }

        frame->pts++;
#else
        sws_scale(m_pImageConvertCtx,RGB32_BUF,rgbLinesize,0,height, YUV_BUF_420, linesizes);

        int datasize=0;
        bool keyframe=false;
        uchar *pH264Data = m_pEncoder->Encoder((unsigned char *)YUV_BUF_420[0], yuvSize, datasize, keyframe);

        if ((NULL != pH264Data) && (datasize > 0)) {
            easyFrame.pBuffer = (Easy_U8*)pH264Data+4;
            easyFrame.u32AVFrameLen =  datasize-4;
            easyFrame.u32VFrameType   = ( keyframe ? EASY_SDK_VIDEO_FRAME_I : EASY_SDK_VIDEO_FRAME_P);
            EasyIPCamera_PushFrame(0,  &easyFrame);
        }
#endif

        if (++g_index>= 600)
        {
            DWORD Stop=GetTickCount();
            printf("Start:%d, Stop:%d, Diff:%d, Count:%d, FPS:%d, \r\n", Start, Stop, (Stop-Start)/1000,g_index, (g_index/((Stop-Start)/1000)));
            g_index = 0;
        }
    }

    // free i420 data buffer
    delete pDataBuffer;
    pDataBuffer = NULL;

#endif

}

#endif
