
#ifndef __EasyIPCamera_H__
#define __EasyIPCamera_H__

#include "EasyTypes.h"

#define RTSP_SERVER_NAME	"EasyIPCamera v1.2.16.1115"


typedef enum _AUTHENTICATION_TYPE_ENUM
{
	AUTHENTICATION_TYPE_BASIC		=	0x00,
	AUTHENTICATION_TYPE_DIGEST,
}AUTHENTICATION_TYPE_ENUM;

typedef struct __LIVE_CHANNEL_INFO_T
{
	int		id;
	char	name[64];
}LIVE_CHANNEL_INFO_T;

typedef struct __EASY_AV_Frame
{
    Easy_U32    u32AVFrameFlag;		/* 帧标志  视频 or 音频 */
    Easy_U32    u32AVFrameLen;		/* 帧的长度 */
    Easy_U32    u32VFrameType;		/* 视频的类型，I帧或P帧 */
    Easy_U8     *pBuffer;			/* 数据 */
    Easy_U32	u32TimestampSec;	/* 时间戳(秒)*/
    Easy_U32	u32TimestampUsec;	/* 时间戳(微秒) */
}EASY_AV_Frame;


typedef enum __EASY_IPCAMERA_STATE_T
{
	EASY_IPCAMERA_STATE_ERROR						=		-1,		//内部错误
       EASY_IPCAMERA_STATE_REQUEST_MEDIA_INFO			=		1,		//新连接,请求media info
	EASY_IPCAMERA_STATE_REQUEST_PLAY_STREAM,						//开始发送流
	EASY_IPCAMERA_STATE_REQUEST_STOP_STREAM,						//停止流
}EASY_IPCAMERA_STATE_T;

/* 回调函数定义 userptr表示用户自定义数据 */
typedef Easy_I32 (*EasyIPCamera_Callback)(Easy_I32 channelId, EASY_IPCAMERA_STATE_T channelState, EASY_MEDIA_INFO_T *mediaInfo, void *userPtr);

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ANDROID
	Easy_API int Easy_APICALL EasyIPCamera_Activate(char *license, char* userptr);
#else
	Easy_API int Easy_APICALL EasyIPCamera_Activate(char *license);
#endif
	/* 启动 Rtsp Server */
	/*设置监听端口, 回调函数及自定义数据 */
	Easy_API Easy_I32 Easy_APICALL EasyIPCamera_Startup(Easy_U16 listenport, AUTHENTICATION_TYPE_ENUM authType, char *realm, Easy_U8 *username, Easy_U8 *password, EasyIPCamera_Callback callback, void *userptr, LIVE_CHANNEL_INFO_T *channelInfo, Easy_U32 channelNum);
	
	/* 终止 Rtsp Server */
	Easy_API Easy_I32 Easy_APICALL EasyIPCamera_Shutdown();


	/* frame:  具体发送的帧数据 */
	Easy_API Easy_I32 Easy_APICALL EasyIPCamera_PushFrame(Easy_I32 channelId, EASY_AV_Frame* frame );


	Easy_API Easy_I32 Easy_APICALL EasyIPCamera_ResetChannel(Easy_I32 channelId);
#ifdef __cplusplus
}
#endif


/*
流程:
1. 调用 EasyIPCamera_Startup 设置监听端口、回调函数和自定义数据指针
2. 启动后，程序进入监听状态
	2.1		接收到客户端请求, 回调 状态:EASY_IPCAMERA_STATE_REQUEST_MEDIA_INFO          上层程序在填充完mediainfo后，返回0, 则EasyIpCamera响应客户端ok
	2.2		EasyIPCamera回调状态 EASY_IPCAMERA_STATE_REQUEST_PLAY_STREAM , 则表示rtsp交互完成, 开始发送流, 上层程序调用EasyIpCamera_PushFrame 发送帧数据
	2.3		EasyIPCamera回调状态 EASY_IPCAMERA_STATE_REQUEST_STOP_STREAM , 则表示客户端已发送teaardown, 要求停止发送帧数据
3.	调用 EasyIpCamera_Shutdown(), 关闭EasyIPCamera，释放相关资源

*/


#endif
