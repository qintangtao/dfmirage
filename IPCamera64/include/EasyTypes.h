/*
	Copyright (c) 2013-2015 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
#ifndef _Easy_Types_H
#define _Easy_Types_H

#ifdef _WIN32
#define Easy_API  __declspec(dllexport)
#define Easy_APICALL  __stdcall
#define WIN32_LEAN_AND_MEAN
#else
#define Easy_API
#define Easy_APICALL 
#endif

// Handle Type
#define Easy_RTSP_Handle void*
#define Easy_Pusher_Handle void*
#define Easy_HLS_Handle void*

typedef int						Easy_I32;

typedef unsigned char           Easy_U8;
typedef unsigned char           Easy_UChar;
typedef unsigned short          Easy_U16;
typedef unsigned int            Easy_U32;
typedef unsigned char			Easy_Bool;

enum
{
    Easy_NoErr						= 0,
    Easy_RequestFailed				= -1,
    Easy_Unimplemented				= -2,
    Easy_RequestArrived				= -3,
    Easy_OutOfState					= -4,
    Easy_NotAModule					= -5,
    Easy_WrongVersion				= -6,
    Easy_IllegalService				= -7,
    Easy_BadIndex					= -8,
    Easy_ValueNotFound				= -9,
    Easy_BadArgument				= -10,
    Easy_ReadOnly					= -11,
	Easy_NotPreemptiveSafe			= -12,
    Easy_NotEnoughSpace				= -13,
    Easy_WouldBlock					= -14,
    Easy_NotConnected				= -15,
    Easy_FileNotFound				= -16,
    Easy_NoMoreData					= -17,
    Easy_AttrDoesntExist			= -18,
    Easy_AttrNameExists				= -19,
    Easy_InstanceAttrsNotAllowed	= -20,
	Easy_InvalidSocket				= -21,
	Easy_MallocError				= -22,
	Easy_ConnectError				= -23,
	Easy_SendError					= -24
};
typedef int Easy_Error;


typedef enum __EASY_ACTIVATE_ERR_CODE_ENUM
{
	EASY_ACTIVATE_INVALID_KEY		=		-1,			//ÎÞÐ§Key
	EASY_ACTIVATE_TIME_ERR			=		-2,			//Ê±¼ä´íÎó
	EASY_ACTIVATE_PROCESS_NAME_LEN_ERR	=	-3,			//½ø³ÌÃû³Æ³¤¶È²»Æ¥Åä
	EASY_ACTIVATE_PROCESS_NAME_ERR	=		-4,			//½ø³ÌÃû³Æ²»Æ¥Åä
	EASY_ACTIVATE_VALIDITY_PERIOD_ERR=		-5,			//ÓÐÐ§ÆÚÐ£Ñé²»Ò»ÖÂ
	EASY_ACTIVATE_PLATFORM_ERR		=		-6,			//Æ½Ì¨²»Æ¥Åä
	EASY_ACTIVATE_COMPANY_ID_LEN_ERR=		-7,			//ÊÚÈ¨Ê¹ÓÃÉÌ²»Æ¥Åä
	EASY_ACTIVATE_SUCCESS			=		0,			//¼¤»î³É¹¦

}EASY_ACTIVATE_ERR_CODE_ENUM;


/* ÊÓÆµ±àÂë */
#define EASY_SDK_VIDEO_CODEC_H264	0x1C		/* H264  */
#define EASY_SDK_VIDEO_CODEC_H265	0x48323635	/* 1211250229 */
#define	EASY_SDK_VIDEO_CODEC_MJPEG	0x08		/* MJPEG */
#define	EASY_SDK_VIDEO_CODEC_MPEG4	0x0D		/* MPEG4 */

/* ÒôÆµ±àÂë */
#define EASY_SDK_AUDIO_CODEC_AAC	0x15002		/* AAC */
#define EASY_SDK_AUDIO_CODEC_G711U	0x10006		/* G711 ulaw*/
#define EASY_SDK_AUDIO_CODEC_G711A	0x10007		/* G711 alaw*/
#define EASY_SDK_AUDIO_CODEC_G726	0x1100B		/* G726 */


/* ÒôÊÓÆµÖ¡±êÊ¶ */
#define EASY_SDK_VIDEO_FRAME_FLAG	0x00000001		/* ÊÓÆµÖ¡±êÖ¾ */
#define EASY_SDK_AUDIO_FRAME_FLAG	0x00000002		/* ÒôÆµÖ¡±êÖ¾ */
#define EASY_SDK_EVENT_FRAME_FLAG	0x00000004		/* ÊÂ¼þÖ¡±êÖ¾ */
#define EASY_SDK_RTP_FRAME_FLAG		0x00000008		/* RTPÖ¡±êÖ¾ */
#define EASY_SDK_SDP_FRAME_FLAG		0x00000010		/* SDPÖ¡±êÖ¾ */
#define EASY_SDK_MEDIA_INFO_FLAG	0x00000020		/* Ã½ÌåÀàÐÍ±êÖ¾*/

/* ÊÓÆµ¹Ø¼ü×Ö±êÊ¶ */
#define EASY_SDK_VIDEO_FRAME_I		0x01		/* IÖ¡ */
#define EASY_SDK_VIDEO_FRAME_P		0x02		/* PÖ¡ */
#define EASY_SDK_VIDEO_FRAME_B		0x03		/* BÖ¡ */
#define EASY_SDK_VIDEO_FRAME_J		0x04		/* JPEG */

/* 连接类型 */
typedef enum __EASY_RTP_CONNECT_TYPE
{
	EASY_RTP_OVER_TCP	=	0x01,		/* RTP Over TCP */
	EASY_RTP_OVER_UDP					/* RTP Over UDP */
}EASY_RTP_CONNECT_TYPE;

/* Ã½ÌåÐÅÏ¢ */
typedef struct __EASY_MEDIA_INFO_T
{
	Easy_U32 u32VideoCodec;				/* ÊÓÆµ±àÂëÀàÐÍ */
	Easy_U32 u32VideoFps;				/* ÊÓÆµÖ¡ÂÊ */

	Easy_U32 u32AudioCodec;				/* ÒôÆµ±àÂëÀàÐÍ */
	Easy_U32 u32AudioSamplerate;		/* ÒôÆµ²ÉÑùÂÊ */
	Easy_U32 u32AudioChannel;			/* ÒôÆµÍ¨µÀÊý */
	Easy_U32 u32AudioBitsPerSample;		/* ÒôÆµ²ÉÑù¾«¶È */

	Easy_U32 u32VpsLength;			/* ÊÓÆµvpsÖ¡³¤¶È */
	Easy_U32 u32SpsLength;			/* ÊÓÆµspsÖ¡³¤¶È */
	Easy_U32 u32PpsLength;			/* ÊÓÆµppsÖ¡³¤¶È */
	Easy_U32 u32SeiLength;			/* ÊÓÆµseiÖ¡³¤¶È */
	Easy_U8	 u8Vps[255];			/* ÊÓÆµvpsÖ¡ÄÚÈÝ */
	Easy_U8	 u8Sps[255];			/* ÊÓÆµspsÖ¡ÄÚÈÝ */
	Easy_U8	 u8Pps[128];				/* ÊÓÆµspsÖ¡ÄÚÈÝ */
	Easy_U8	 u8Sei[128];				/* ÊÓÆµseiÖ¡ÄÚÈÝ */
}EASY_MEDIA_INFO_T;

/* Ö¡ÐÅÏ¢ */
typedef struct 
{
	unsigned int	codec;				/* ÒôÊÓÆµ¸ñÊ½ */

	unsigned int	type;				/* ÊÓÆµÖ¡ÀàÐÍ */
	unsigned char	fps;				/* ÊÓÆµÖ¡ÂÊ */
	unsigned short	width;				/* ÊÓÆµ¿í */
	unsigned short  height;				/* ÊÓÆµ¸ß */

	unsigned int	reserved1;			/* ±£Áô²ÎÊý1 */
	unsigned int	reserved2;			/* ±£Áô²ÎÊý2 */

	unsigned int	sample_rate;		/* ÒôÆµ²ÉÑùÂÊ */
	unsigned int	channels;			/* ÒôÆµÉùµÀÊý */
	unsigned int	bits_per_sample;	/* ÒôÆµ²ÉÑù¾«¶È */

	unsigned int	length;				/* ÒôÊÓÆµÖ¡´óÐ¡ */
	unsigned int    timestamp_usec;		/* Ê±¼ä´Á,Î¢Ãî */
	unsigned int	timestamp_sec;		/* Ê±¼ä´Á Ãë */
	
	float			bitrate;			/* ±ÈÌØÂÊ */
	float			losspacket;			/* ¶ª°üÂÊ */
}RTSP_FRAME_INFO;

#endif
