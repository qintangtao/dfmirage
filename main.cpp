
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <tchar.h>
#include "Decoder/H264Encoder.h"
#include "libyuv/yuv_util.h"
#include "EasyIPCameraAPI.h"
#include "MirrorDriverClient.h"
#include "StopWatch.h"
#include "AutoLock.h"
#include "LocalMutex.h"

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
}

//exe名称固定（EasyIPCamera_RTSP） KEY_EASYIPCAMERA和名称绑定了
//#define KEY_EASYIPCAMERA "6D72754B7A4969576B5A75413362465A706B3337634F704659584E35535642445957316C636D4666556C52545543356C654756584446616732504467523246326157346D516D466962334E68514449774D545A4659584E355247467964326C75564756686257566863336B3D"
// EasyScreenCapture.exe
//#define KEY_EASYIPCAMERA "6D72754B7A4969576B5A746637685A6A6E4F6A716B65704659584E3555324E795A57567551324677644856795A53356C6547566959677842325044765A475A7461584A685A3256414D54597A4C6D4E766257566863336B3D"
#define KEY_EASYIPCAMERA "6D72754B7A4969576B5A75394578646A6E4F6A464D2B704659584E3555324E795A57567551324677644856795A53356C654756592B726C42325044765A475A7461584A685A3256414D54597A4C6D4E766257566863336B3D"

#ifdef _WIN32
    #define USleep(x) ::Sleep((x / 1000) + 1)
#else
    #define USleep(x) usleep(x)
#endif

typedef struct tagCursorShape
{
    int x;
    int y;
    int width;
    int height;
    int widthBytes;
    void *buffer;
} CursorShape;

typedef struct tagSOURCE_CHANNEL_T
{
    int		id;
    char		name[36];
    char		source_uri[128];
    int		pushStream;

    EASY_MEDIA_INFO_T	mediaInfo;
    void*	anyHandle;

    HANDLE thread;
    HANDLE thread_mouse;
    bool		bThreadLiving;

    H264Encoder *encoder;

    int width;
    int height;
    int fps;

    bool draw_mouse;
    CursorShape  cursorShape;
    Lockable        *cursorMutex;
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
        USleep(1000);
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
        fprintf(stdout, "%s", line);
    } else{
        fprintf(stderr, "%s", line);
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
        printf("[channel %d] StopStream...\n", channelId);
        pChannel[channelId].pushStream = 0x00;
    }

    return 0;
}

static void paint_mouse_pointer(HDC dest_hdc)
{
    CURSORINFO ci = {0};
    ci.cbSize = sizeof(ci);

    if (GetCursorInfo(&ci)) {
        HCURSOR icon = CopyCursor(ci.hCursor);
        ICONINFO info;
        POINT pos;
        info.hbmMask = NULL;
        info.hbmColor = NULL;

        if (ci.flags != CURSOR_SHOWING)
            return;

        if (!icon) {
            /* Use the standard arrow cursor as a fallback.
             * You'll probably only hit this in Wine, which can't fetch
             * the current system cursor. */
            icon = CopyCursor(LoadCursor(NULL, IDC_ARROW));
        }

        if (!GetIconInfo(icon, &info)) {
            printf("Could not get icon info");
            goto icon_error;
        }

        //that would keep the correct location of mouse with hidpi screens
        pos.x = ci.ptScreenPos.x  - info.xHotspot;
        pos.y = ci.ptScreenPos.y  - info.yHotspot;

        //printf("Cursor pos (%li,%li) -> (%li,%li)\n", ci.ptScreenPos.x, ci.ptScreenPos.y, pos.x, pos.y);
#if 1
        DrawIconEx(dest_hdc, pos.x, pos.y, icon, 0, 0, 0, NULL, DI_NORMAL);
#else
        if (!DrawIcon(dest_hdc, pos.x, pos.y, icon))
            printf("Couldn't draw icon");
#endif

icon_error:
        if (info.hbmMask)
            DeleteObject(info.hbmMask);
        if (info.hbmColor)
            DeleteObject(info.hbmColor);
        if (icon)
            DestroyCursor(icon);
    } else {
        printf("Couldn't get cursor info");
    }
}

unsigned int _stdcall  CaptureMouseThread(void* lParam)
{
    SOURCE_CHANNEL_T* channelInfo = (SOURCE_CHANNEL_T*)lParam;
    if (!channelInfo)
    {
        return -1;
    }

    int                 channelId       = channelInfo->id;
    int                 fps                 = channelInfo->fps;
    CursorShape  *cursorShape = &channelInfo->cursorShape;
    Lockable        *cursorMutex = channelInfo->cursorMutex;
    double          frametime      = 1000000.0f / fps; // 每一帧时间（微秒）

    bool isColorShape;
    int x;
    int y;
    int width;
    int height;
    int widthBytes;

    HCURSOR hCursor;
    HCURSOR lastHCursor = NULL;
    ICONINFO iconInfo;
    CURSORINFO cursorInfo;
    BITMAP bmMask;

    HDC                                 hdc = NULL;
    BITMAPINFO                     bmi;
    VOID                                *pBits ;
    HBITMAP                          hbmp = NULL;
    HBITMAP                          hbmOld = NULL;

    printf("[channel %d] Mouse Start\n", channelId);
    while (channelInfo->bThreadLiving)
    {
        ::Sleep(20);
        if (channelInfo->pushStream == 0) {
            continue;
        }

        cursorInfo.cbSize = sizeof(CURSORINFO);
        if (GetCursorInfo(&cursorInfo) == 0) {
            fprintf(stderr, "[channel %d] GetCursorInfo Failed.\n", channelId);
            continue;
        }

        hCursor = cursorInfo.hCursor;
        if (hCursor == 0) {
            fprintf(stderr, "[channel %d] hCursor is null.\n", channelId);
            continue;
        }

        if (!GetIconInfo(hCursor, &iconInfo)) {
            fprintf(stderr, "[channel %d] GetIconInfo Failed.\n", channelId);
            continue;
        }

        // 1. 位置
        x = cursorInfo.ptScreenPos.x  - iconInfo.xHotspot;
        y = cursorInfo.ptScreenPos.y  - iconInfo.yHotspot;

        if (x != cursorShape->x || y != cursorShape->y)
        {
            //加锁
            AutoLock lock(cursorMutex);

            cursorShape->x = x;
            cursorShape->y = y;
#ifdef _DEBUG
            fprintf(stderr, "[channel %d] update cursor pos. x:%d,  y:%d .\n", channelId, x, y);
#endif
        }

        // 2. 形状
        if (lastHCursor == hCursor) {
            goto mouse_error;
        }

        if (iconInfo.hbmMask == NULL) {
            fprintf(stderr, "[channel %d] hbmMask is null.\n", channelId);
            goto mouse_error;
        }

        isColorShape = (iconInfo.hbmColor != NULL);

        if (!GetObject(iconInfo.hbmMask, sizeof(BITMAP), (LPVOID)&bmMask)) {
            fprintf(stderr, "[channel %d] Get BITMAP Failed.\n", channelId);
            goto mouse_error;
        }

        if (bmMask.bmPlanes != 1 || bmMask.bmBitsPixel != 1) {
            fprintf(stderr, "[channel %d] bmMask.bmPlanes!=1 or bmMask.bmBitsPixel != 1  .\n", channelId);
            goto mouse_error;
        }

        width = bmMask.bmWidth;
        height = isColorShape ? bmMask.bmHeight : bmMask.bmHeight/2;
        widthBytes = bmMask.bmWidthBytes;

        // 绘制
        hdc = CreateCompatibleDC(NULL);
        if (!hdc) {
            fprintf(stderr, "[channel %d] CreateCompatibleDC Failed.\n", channelId);
            goto mouse_error;
        }

        bmi.bmiHeader.biSize                        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth                    = width;
        bmi.bmiHeader.biHeight                   = height;
        bmi.bmiHeader.biPlanes                   = 1;
        bmi.bmiHeader.biBitCount                = 32;
        bmi.bmiHeader.biCompression         = BI_RGB;
        bmi.bmiHeader.biSizeImage             = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * bmi.bmiHeader.biBitCount / 8;
        bmi.bmiHeader.biXPelsPerMeter       = 0;
        bmi.bmiHeader.biYPelsPerMeter       = 0;
        bmi.bmiHeader.biClrUsed                 = 0;
        bmi.bmiHeader.biClrImportant          = 0;

        hbmp = ::CreateDIBSection (hdc, (BITMAPINFO *)  &bmi, DIB_RGB_COLORS, &pBits, NULL, 0) ;
        if (!hbmp) {
            fprintf(stderr, "[channel %d] Creating DIB Section Failed.\n", channelId);
            goto mouse_error;
        }

        hbmOld = (HBITMAP)SelectObject(hdc, hbmp);

        DrawIconEx(hdc, 0, 0, hCursor, 0, 0, 0, NULL, DI_IMAGE);

#if 0
       {
            BITMAPFILEHEADER fileHeader;
           fileHeader.bfType = 0x4d42; // BM
           fileHeader.bfReserved1 = 0;
           fileHeader.bfReserved2 = 0;
           fileHeader.bfSize = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 4 + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
           fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

            FILE *f = fopen("dfmirage_mouse.bmp", "w");
            if (f)
            {
                fwrite(&fileHeader, 1, sizeof(BITMAPFILEHEADER), f);
                fwrite(&bmi.bmiHeader, 1, sizeof(BITMAPINFOHEADER), f);
                fwrite(pBits, 1, bmi.bmiHeader.biSizeImage, f);
                fclose(f);
            }
       }
#endif

        //加锁
        cursorMutex->lock();
        if (cursorShape->buffer && (cursorShape->width * cursorShape->height * 4) < bmi.bmiHeader.biSizeImage)
        {
            free(cursorShape->buffer);
            cursorShape->buffer = 0;
        }
        if (!cursorShape->buffer)
        {
            cursorShape->buffer = malloc(bmi.bmiHeader.biSizeImage);
            if (!cursorShape->buffer)
            {
                fprintf(stderr, "[channel %d] new buffer Failed.\n", channelId);
                //解锁
                cursorMutex->unlock();
                goto mouse_error;
            }
        }

        memcpy(cursorShape->buffer, pBits, bmi.bmiHeader.biSizeImage);

        cursorShape->width = width;
        cursorShape->height = height;
        cursorShape->widthBytes = widthBytes;
        cursorMutex->unlock();

#ifdef _DEBUG
        fprintf(stderr, "[channel %d] update cursor shape. width:%d,  height:%d .\n", channelId, width, height);
#endif

        // 更新
        lastHCursor = hCursor;

mouse_error:
        if (hbmOld) {
            SelectObject(hdc, hbmOld);
            hbmOld = NULL;
        }
        if (hbmp) {
            DeleteObject(hbmp);
            hbmp = NULL;
        }
        if (hdc) {
            DeleteDC(hdc);
            hdc = NULL;
        }
        if (iconInfo.hbmMask) {
            DeleteObject(iconInfo.hbmMask);
            iconInfo.hbmMask = NULL;
        }
        if (iconInfo.hbmColor) {
            DeleteObject(iconInfo.hbmColor);
            iconInfo.hbmColor = NULL;
        }
    }

    printf("[channel %d] Mouse Stop\n", channelId);

    return 0;
}

unsigned int _stdcall  CaptureScreenThread(void* lParam)
{
    SOURCE_CHANNEL_T* pChannelInfo = (SOURCE_CHANNEL_T*)lParam;
    if (!pChannelInfo)
    {
        return -1;
    }

    MirrorDriverClient              *pClient             = (MirrorDriverClient*)pChannelInfo->anyHandle;
    int                                     nChannelId       = pChannelInfo->id;
    int                                     fps                    = pChannelInfo->fps;
    bool                                  draw_mouse      = pChannelInfo->draw_mouse;
    int                                     width                = pChannelInfo->width;
    int                                     height               = pChannelInfo->height;
    CursorShape                     *cursorShape     = &pChannelInfo->cursorShape;
    Lockable                           *cursorMutex    = pChannelInfo->cursorMutex;

    uint8_t                             *src_data[4];
    int                                     src_linesize[4];
    uint8_t                             *dst_data[4];
    int                                     dst_linesize[4];

    enum AVPixelFormat         srcFormat = AV_PIX_FMT_RGB32;
    enum AVPixelFormat         dstFormat = AV_PIX_FMT_YUV420P;

    SwsContext                      *sws_ctx = NULL;

#ifdef ENABLED_FFMPEG_ENCODER
    const AVCodec                *codec;
    AVCodecContext             *c= NULL;
    AVPacket                         *pkt = NULL;
    AVFrame                         *frame = NULL;
#else
    int                                   yuvSize = width*height*1.5;
    int                                   datasize;
    bool                                keyframe;
    uint8_t                            *encode_data;
#endif

    int                                   ret;
    int64_t                             pts = 0;
    double                            frametime = 1000000.0f / fps; // 每一帧时间（微秒）
    double                            frameclock = 0;  // 时间基准（按fps计算，每次累加每帧时间，然后修复）
    StopWatch                      watch;

#ifdef ENABLED_TEST_FPS
    int                                    index = 0;
    StopWatch                        watchFPS;
    StopWatch                        watchMouse;
#endif

#ifdef DIRECT_DRAW_MOUSE
    HDC                                 dest_hdc = NULL;
    BITMAPINFO                     bmi;
    VOID                                *pBits ;
    HBITMAP                          hbmp = NULL;
#endif

    do
    {
        ret = av_image_alloc(src_data, src_linesize, width, height, srcFormat, 1);
        if (ret < 0)
        {
            fprintf(stderr, "[channel %d] Could not allocate buffers..\n", nChannelId);
            break;
        }

        ret = av_image_alloc(dst_data, dst_linesize, width, height, dstFormat, 1);
        if (ret < 0)
        {
            fprintf(stderr, "[channel %d] Could not allocate buffers..\n", nChannelId);
            break;
        }

        sws_ctx = sws_getContext(width,height,srcFormat, width,height,dstFormat,SWS_BICUBIC,NULL,NULL,NULL);
        if (sws_ctx == NULL) {
            fprintf(stderr, "[channel %d] Could not sws_getContext.\n", nChannelId);
            break;
        }

#ifdef ENABLED_FFMPEG_ENCODER
        // codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        //codec = avcodec_find_encoder_by_name("libx264");
        //codec = avcodec_find_encoder_by_name("h264_qsv");
        codec = avcodec_find_encoder_by_name("h264_nvenc");
        if (NULL == codec)
        {
            fprintf(stderr, "[channel %d] Could not avcodec_find_encoder.\n", nChannelId);
            break;
        }

        c = avcodec_alloc_context3(codec);

        /* put sample parameters */
        //c->bit_rate = 400000;
        /* resolution must be a multiple of two */
        c->frame_number=1;
        c->width = width;
        c->height = height;
        /* frames per second */
        c->time_base = (AVRational){1, fps};
        c->framerate = (AVRational){fps, 1};
        c->max_b_frames = 1;
        c->pix_fmt = dstFormat;
        c->thread_count = 1;

        if (avcodec_open2(c, codec, NULL) < 0) {
            fprintf(stderr, "[channel %d] Could not open codec.\n", nChannelId);
            break;
        }

        // 用来存放编码后的数据（h264)
        pkt = av_packet_alloc();
        if (!pkt) {
            fprintf(stderr, "[channel %d] Could not allocate packet.\n", nChannelId);
            break;
        }

        // 用来存放编码前的数据（yuv）
        frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "[channel %d] Could not allocate frame.\n", nChannelId);
            break;
        }

        // 保证 frame 里就是一帧 yuv 数据
        frame->width = c->width;
        frame->height = c->height;
        frame->format = c->pix_fmt;
        frame->pts = 0;

        // 创建输入缓冲区 方法一
        ret = av_image_alloc(frame->data, frame->linesize, width, height, c->pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "[channel %d] Could not allocate buffers.\n", nChannelId);
            break;
        }

        /*
        // 利用width、height、format创建frame的数据缓冲区，利用width、height、format可以算出一帧大小
        ret = av_frame_get_buffer(frame, 1);
        if (ret < 0) {
            break;
        }
        */

        /*
        // 创建输入缓冲区 方法二
        buf = (uint8_t *)av_malloc(image_size);
        ret = av_image_fill_arrays(frame->data, frame->linesize, buf, in.pixFmt, in.width, in.height, 1);
        if (ret < 0) {
            break;
        }
        */
#endif

#ifdef DIRECT_DRAW_MOUSE
        if (draw_mouse)
        {
            dest_hdc = CreateCompatibleDC(NULL);
            if (!dest_hdc) {
                fprintf(stderr, "[channel %d] CreateCompatibleDC Failed.\n", nChannelId);
                break;
            }

            bmi.bmiHeader.biSize                        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth                    = width;
            bmi.bmiHeader.biHeight                   = height;
            bmi.bmiHeader.biPlanes                   = 1;
            bmi.bmiHeader.biBitCount                = 32;
            bmi.bmiHeader.biCompression         = BI_RGB;
            bmi.bmiHeader.biSizeImage             = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * bmi.bmiHeader.biBitCount / 8;
            bmi.bmiHeader.biXPelsPerMeter       = 0;
            bmi.bmiHeader.biYPelsPerMeter       = 0;
            bmi.bmiHeader.biClrUsed                 = 0;
            bmi.bmiHeader.biClrImportant          = 0;

            hbmp = ::CreateDIBSection (dest_hdc, (BITMAPINFO *)  &bmi, DIB_RGB_COLORS, &pBits, NULL, 0) ;
            if (!hbmp) {
                fprintf(stderr, "[channel %d] Creating DIB Section Failed.\n", nChannelId);
                break;
            }

            SelectObject(dest_hdc, hbmp);
        }
#endif

        printf("[channel %d] Screen Start\n", nChannelId);
        while (pChannelInfo&&pChannelInfo->bThreadLiving)
        {
            if (pChannelInfo->pushStream == 0) {
                USleep(frametime);
                continue;
            }

#ifdef ENABLED_TEST_FPS
            if (index == 0)
                watchFPS.Start();
#endif
            if (INT64_MAX == pts)
            {
                pts = 0;
            }

            if (pts % fps == 0)
            {
                frameclock = 0;
                watch.Start();
            }

            if (draw_mouse)
            {
#ifndef DIRECT_DRAW_MOUSE
                memcpy(src_data[0], pClient->getBuffer(), width*height*4);

                if (NULL !=cursorShape->buffer && cursorShape->x >= 0 && cursorShape->y >= 0)
                {
                    AutoLock lock(cursorMutex);

                    char *pixel;
                    char *pixelCursor;
                    char *pixelsBuffer = (char *)src_data[0];
                    char *pixelsBufferCursor = (char *)cursorShape->buffer;
                    int pixelSize = 4;
                    int iMaxRow = cursorShape->y + cursorShape->height;
                    if (iMaxRow > height) iMaxRow = height;
                    int iMaxCol = cursorShape->x + cursorShape->width;
                    if (iMaxCol > width) iMaxCol = width;

#ifdef _DEBUG
                    printf("[channel %d] draw mouse x:%d, y:%d, w:%d, h:%d, w:%d, h:%d\n",
                           nChannelId, cursorShape->x, cursorShape->y, cursorShape->width, cursorShape->height, width, height);
#endif

                    for (int iRow = cursorShape->y; iRow < iMaxRow; iRow++) {
                      for (int iCol = cursorShape->x; iCol < iMaxCol; iCol++) {
                        pixel = pixelsBuffer + (iRow * width + iCol) * pixelSize;
                        pixelCursor = pixelsBufferCursor + ((cursorShape->height - 1 - (iRow - cursorShape->y)) * cursorShape->width + (iCol - cursorShape->x)) * pixelSize;
#if 1
                        // Alpha
                        if (pixelCursor[3] == 0)
                            continue;
#else
                        if (pixelMouse[0] == 0 &&
                                pixelMouse[1] == 0 &&
                                pixelMouse[2] == 0 &&
                                pixelMouse[3] == 0)
                            continue;
#endif

                        memcpy(pixel, pixelCursor, pixelSize);
                      }
                    }

                }

                //watchMouse.Start();
                //paint_mouse_pointer2((char *)src_data[0], width, height);
                //watchMouse.End();
                //printf("[channel %d] paint mouse time:%f\n", nChannelId, watchMouse.costTime);
#else
                // 由于绘制的鼠标是倒置的，底图需要倒置拷贝
                for (int i = 0; i < height; i++) {
                    memcpy((char *)pBits + (i * width * 4), (char *)pClient->getBuffer() + ((height - i - 1) * width * 4), width*4);
                }

                //watchMouse.Start();
                paint_mouse_pointer(dest_hdc);
                //watchMouse.End();
                //printf("[channel %d] paint mouse time:%f\n", nChannelId, watchMouse.costTime);

                // 翻转底图
                for (int i = 0; i < height; i++) {
                    memcpy((char *)src_data[0] + ((height - i - 1) * width * 4), (char *)pBits + (i * width * 4), width*4);
                }
#endif
            }
            else
            {
                memcpy(src_data[0], pClient->getBuffer(), width*height*4);
            }

#ifdef ENABLED_FFMPEG_ENCODER

#ifdef ENABLED_LIBYUV_CONVERT
            ARGBToI420((const uint8_t *)src_data[0], width, height, (uint8_t *)frame->data[0]);
#else
            sws_scale(sws_ctx,src_data,src_linesize,0,height, frame->data,frame->linesize);
#endif
            ret = avcodec_send_frame(c, frame);
            if (ret < 0) {
                fprintf(stderr, "[channel %d] Could not send frame.\n", nChannelId);
                continue;
            }

            while (true) {
                // 从编码器中获取编码后的数据
                ret = avcodec_receive_packet(c, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    //fprintf(stderr, "[channel %d] Could not receive packet (%d).\n", nChannelId, ret);
                    break;
                } else if (ret < 0) {
                    fprintf(stderr, "[channel %d] Could not receive packet.\n", nChannelId);
                    break;
                }

                EASY_AV_Frame	frame;
                frame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
                frame.pBuffer = (Easy_U8*)pkt->data+4;
                frame.u32AVFrameLen =  pkt->size-4;
                frame.u32VFrameType   =  ( pkt->flags && AV_PKT_FLAG_KEY ? EASY_SDK_VIDEO_FRAME_I : EASY_SDK_VIDEO_FRAME_P);
                frame.u32TimestampSec = 0;
                frame.u32TimestampUsec = 0;
                EasyIPCamera_PushFrame(nChannelId,  &frame);

                av_packet_unref(pkt);
            }

            frame->pts++;

#else
#ifdef ENABLED_LIBYUV_CONVERT
            ARGBToI420((const uint8_t *)src_data[0], width, height, (uint8_t *)dst_data[0]);
#else
            sws_scale(sws_ctx,src_data,src_linesize,0,height, dst_data, dst_linesize);
#endif

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
#endif

            pts++;

            watch.End();

            //到当前帧理论时间
            frameclock += frametime;
            //到当前帧实际时间 watch.costTime
            //printf("[channel %d] pts:%d, frameclock:%f, totle:%f\n", nChannelId, frame->pts % fps, frameclock, watch.costTime);
            if (frameclock > watch.costTime)
            {
                double dealy= (frameclock - watch.costTime) / 1000;
                if (dealy > 0)
                    ::MsgWaitForMultipleObjectsEx(0, NULL, dealy, QS_ALLPOSTMESSAGE, MWMO_INPUTAVAILABLE);
            }

#ifdef ENABLED_TEST_FPS
            index++;
            if (index>= 1000)
            {
                watchFPS.End();
                double fps = (index*1000000.0f)/watchFPS.costTime;
                printf("[channel %d] pts:%d, Time:%f, FPS:%f, \n", nChannelId, pts, watchFPS.costTime,  fps);
                index = 0;
            }
#endif
        }

    } while(false);

#ifdef ENABLED_FFMPEG_ENCODER
    avcodec_free_context(&c);
    av_packet_free(&pkt);
    av_frame_free(&frame);
#endif

    // 崩溃，暂时注释掉
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);

#ifdef DIRECT_DRAW_MOUSE
    if (dest_hdc)
        DeleteDC(dest_hdc);
    if (hbmp)
        DeleteObject(hbmp);
#endif

    printf("[channel %d] Screen Stop\n", nChannelId);

    return 0;
}

int main(int argc, char *argv[])
{
    int                     fps = 60;
    bool                  draw_mouse = false;
    int                     OutputCount = 1;
    char                  *ConfigName=	"channel";

    char                   szIP[16] = {0};
    int                      nServerPort = 554;

    uint8_t                 sps[100];
    uint8_t                 pps[100];
    long                    spslen;
    long                    ppslen;

    enum AVHWDeviceType type;

#ifdef ENABLED_FFMPEG_LOG
    av_log_set_level(AV_LOG_TRACE);
    av_log_set_callback(__FFmpegLog_Callback);
#endif

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <fps(20-120)> <draw_mouse(1/0)>\n", argv[0]);
        getchar();
        return -1;
    }

    fps = atoi(argv[1]);
    draw_mouse= atoi(argv[2]) != 0;

    fprintf(stdout, "fps:%d, draw_mouse:%d\n", fps, draw_mouse);

    fprintf(stdout, "Available device types:");
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        fprintf(stdout, " %s", av_hwdevice_get_type_name(type));
    fprintf(stdout, "\n");

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

    nServerPort = GetAvaliblePort(nServerPort);

    //int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    //int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    SOURCE_CHANNEL_T*	channels = new SOURCE_CHANNEL_T[OutputCount];
    memset(&channels[0], 0x00, sizeof(SOURCE_CHANNEL_T) * OutputCount);
    char rtsp_url[128];
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

        channels[i].draw_mouse = draw_mouse;

        channels[i].encoder = new H264Encoder();
        channels[i].encoder->Init(width, height, fps, 30, 4096, 1, 20, 0);
        channels[i].encoder->GetSPSAndPPS(sps, spslen, pps, ppslen);

        channels[i].cursorShape.x = 0;
        channels[i].cursorShape.y = 0;
        channels[i].cursorShape.width = 0;
        channels[i].cursorShape.height = 0;
        channels[i].cursorShape.widthBytes = 0;
        channels[i].cursorShape.buffer = 0;

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

        channels[i].cursorMutex = new LocalMutex();

        channels[i].thread = (HANDLE)_beginthreadex(NULL, 0, CaptureScreenThread, (void*)&channels[i],0,0);

#ifndef DIRECT_DRAW_MOUSE
        channels[i].thread_mouse = (HANDLE)_beginthreadex(NULL, 0, CaptureMouseThread, (void*)&channels[i],0,0);
#endif

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

     ::Sleep(1000);
    printf("Press Enter exit...\n");
    getchar();
    getchar();
    getchar();

    for(int nI=0; nI<OutputCount; nI++)
    {
        channels[nI].bThreadLiving = false;
    }
    ::Sleep(3000);

    EasyIPCamera_Shutdown();

    delete[] channels;
    delete[] liveChannels;

    return 0;
}
