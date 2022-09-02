#ifndef H264ENCODER_H
#define H264ENCODER_H

// x264中使用了stdint中的类型
#include "stdint.h"
extern "C" {
    #include "x264.h"
}

#define ZH264_DEFAULT 0
#define ZH264_ULTRAFAST 1
#define ZH264_SUPERFAST 2
#define ZH264_VERYFAST 3
#define ZH264_FASTER 4
#define ZH264_FAST 5
#define ZH264_MEDIUM 6
#define ZH264_SLOW 7
#define ZH264_SLOWER 8
#define ZH264_VERYSLOW 9

class H264Encoder
{
public:
    H264Encoder();
    ~H264Encoder();

    static	void X264_CONFIG_SET(x264_param_t*param,int mode);
    unsigned char*	Encoder(unsigned char *indata,int inlen,int &outlen, bool& bIsKeyFrame);
    int Clean();
    int Init(int width,int height,int fps,int keyframe,int bitrate,int level,int qp,int nUseQP);
    void GetSPSAndPPS(unsigned char*sps,long&spslen,unsigned char*pps,long&ppslen);

private:
    x264_t*m_hx264;
    x264_param_t m_x264_param;
    x264_picture_t *m_x264_picin;
    x264_picture_t m_x264_picout;
    x264_nal_t*	m_x264_nal;
    int m_nwidth;
    int m_nheight;
    bool m_bIsworking;
    int m_nEncoderIndex;
    int m_ncheckyuvsize;
};

#endif // H264ENCODER_H
