//
// Created by Administrator on 2022/4/14.
//

#include "yuv_util.h"
#include <stdio.h>

extern "C"
{
    #include "libyuv.h"
}

int NV21ToI420(const uint8_t *src_nv21, int width, int height, uint8_t *dst_i420)
{
    const int src_y_size = width * height;
    const int src_u_size = (width >> 1) * (height >> 1);
    const uint8_t *src_y = src_nv21;
    const uint8_t *src_vu = src_nv21 + src_y_size;

    uint8_t *dst_y = dst_i420;
    uint8_t *dst_u = dst_i420 + src_y_size;
    uint8_t *dst_v = dst_i420 + src_y_size + src_u_size;

    return libyuv::NV21ToI420(src_y, width,
            src_vu, width,
            dst_y, width,
            dst_u, width >> 1,
            dst_v, width >> 1,
            width, height);

}

int I420Scale(const uint8_t *src_i420, int width, int height, uint8_t *dst_i420, int dst_width, int dst_height, int mode)
{
    const int src_y_size = width * height;
    const int src_u_size = (width >> 1) * (height >> 1);
    const uint8_t *src_y = src_i420;
    const uint8_t *src_u = src_i420 + src_y_size;
    const uint8_t *src_v = src_i420 + src_y_size + src_u_size;

    const int dst_y_size = dst_width * dst_height;
    const int dst_u_size = (dst_width >> 1) * (dst_height >> 1);
    uint8_t *dst_y = dst_i420;
    uint8_t *dst_u = dst_i420 + dst_y_size;
    uint8_t *dst_v = dst_i420 + dst_y_size + dst_u_size;

    return libyuv::I420Scale(src_y, width,
                      src_u, width >> 1,
                      src_v, width >> 1,
                      width, height,
                      dst_y, dst_width,
                      dst_u, dst_width >> 1,
                      dst_v, dst_width >> 1,
                      dst_width, dst_height,
                      (libyuv::FilterMode)mode);
}

int I420ToRGB565(const uint8_t *src_i420, int width, int height, uint8_t *dst_rgb565)
{
    const int src_y_size = width * height;
    const int src_u_size = (width >> 1) * (height >> 1);

    const uint8_t *src_y = src_i420;
    const uint8_t *src_u = src_i420 + src_y_size;
    const uint8_t *src_v = src_i420 + src_y_size + src_u_size;

    return libyuv::I420ToRGB565(src_y, width,
                         src_u, width >> 1,
                         src_v, width >> 1,
                         dst_rgb565,
                         width * 2,
                         width, height);
}

int I420ToRGB24(const uint8_t *src_i420, int width, int height, uint8_t *dst_rgb24)
{
    const int src_y_size = width * height;
    const int src_u_size = (width >> 1) * (height >> 1);

    const uint8_t *src_y = src_i420;
    const uint8_t *src_u = src_i420 + src_y_size;
    const uint8_t *src_v = src_i420 + src_y_size + src_u_size;

    return libyuv::I420ToRGB24(src_y, width,
                        src_u, width >> 1,
                        src_v, width >> 1,
                        dst_rgb24,
                        width * 3,
                        width, height);
}

int I420ToABGR(const uint8_t *src_i420, int width, int height, uint8_t *dst_abgr)
{
    const int src_y_size = width * height;
    const int src_u_size = (width >> 1) * (height >> 1);

    const uint8_t *src_y = src_i420;
    const uint8_t *src_u = src_i420 + src_y_size;
    const uint8_t *src_v = src_i420 + src_y_size + src_u_size;

    return libyuv::I420ToABGR(src_y, width,
                               src_u, width >> 1,
                               src_v, width >> 1,
                               dst_abgr,
                               width * 4,
                               width, height);
}

int ABGRToI420(const uint8_t *src_abgr, int width, int height, uint8_t *dst_i420)
{
    const int dst_y_size = width * height;
    const int dst_u_size =  (width >> 1) * (height >> 1);

    uint8_t *dst_y = dst_i420;
    uint8_t *dst_u = dst_i420 + dst_y_size;
    uint8_t *dst_v = dst_i420 + dst_y_size + dst_u_size;

    return libyuv::ABGRToI420(src_abgr,
                   width * 4,
                   dst_y, width,
                   dst_u, width >> 1,
                   dst_v, width >> 1,
                   width,
                   height);
}

int RGBAToI420(const uint8_t *src_rgba, int width, int height, uint8_t *dst_i420)
{
    const int dst_y_size = width * height;
    const int dst_u_size = (width >> 1) * (height >> 1);

    uint8_t *dst_y = dst_i420;
    uint8_t *dst_u = dst_i420 + dst_y_size;
    uint8_t *dst_v = dst_i420 + dst_y_size + dst_u_size;

    return libyuv::RGBAToI420(src_rgba,
                   width * 4,
                   dst_y, width,
                   dst_u, width >> 1,
                   dst_v, width >> 1,
                   width,
                   height);
}

int ARGBToI420(const uint8_t *src_argb, int width, int height, uint8_t *dst_i420)
{
    const int dst_y_size = width * height;
    const int dst_u_size = (width >> 1) * (height >> 1);

    uint8_t *dst_y = dst_i420;
    uint8_t *dst_u = dst_i420 + dst_y_size;
    uint8_t *dst_v = dst_i420 + dst_y_size + dst_u_size;

    return libyuv::ARGBToI420(src_argb,
                   width * 4,
                   dst_y, width,
                   dst_u, width >> 1,
                   dst_v, width >> 1,
                   width,
                   height);
}

#pragma pack (push ,1)//由于4字节对齐，而信息头大小为54字节，第一部分14字节，
//第二部分40字节，所以会将第一部分补齐为16自己，直接用sizeof，打开图片时就会
//遇到premature end-of-file encountered错误
typedef struct {//位图文件头,14字节
    uint16_t      bfType;   //  指定文件类型，必须是0x424D，即字符串“BM”，也就是说所有.bmp文件的头两个字节都是“BM”。
    uint32_t      bfSize;   //   位图文件的大小，包括这14个字节，以字节为单位
    uint16_t      bfReserved1;   //   位图文件保留字，必须为0
    uint16_t      bfReserved2;   //   位图文件保留字，必须为0
    uint32_t      bfOffBits;   //   位图数据的起始位置，以相对于位图， 文件头的偏移量表示，以字节为单位
} BMPFILEHEADER_T;

typedef struct{//这个结构的长度是固定的，为40个字节,可以自己算一下，DWORD、LONG4个字节，WORD两个字节
    uint32_t       biSize;//指定这个结构的长度，为40
    uint32_t       biWidth;//指定图象的宽度，单位是象素。
    uint32_t       biHeight;//指定图象的高度，单位是象素。
    uint16_t       biPlanes;//必须是1，不用考虑。
    uint16_t       biBitCount;/*指定表示颜色时要用到的位数，常用的值为1(黑白二色图), 4(16色图),
							  8(256色), 24(真彩色图)(新的.bmp格式支持32位色，这里就不做讨论了)。*/
    uint32_t      biCompression;/*指定位图是否压缩，有效的值为BI_RGB，BI_RLE8，BI_RLE4，
								 BI_BITFIELDS(都是一些Windows定义好的常量)。要说明的是，
								 Windows位图可以采用RLE4，和RLE8的压缩格式，但用的不多。
								 我们今后所讨论的只有第一种不压缩的情况，即biCompression为BI_RGB的情况。*/
    uint32_t      biSizeImage;/*指定实际的位图数据占用的字节数，其实也可以从以下的公式中计算出来：
biSizeImage=biWidth’ × biHeight
要注意的是：上述公式中的biWidth’必须是4的整倍数(所以不是biWidth，而是biWidth’，
表示大于或等于biWidth的，最接近4的整倍数。举个例子，如果biWidth=240，则biWidth’=240；
如果biWidth=241，biWidth’=244)。如果biCompression为BI_RGB，则该项可能为零*/
    uint32_t       biXPelsPerMeter;//指定目标设备的水平分辨率，单位是每米的象素个数
    uint32_t       biYPelsPerMeter;//指定目标设备的垂直分辨率，单位同上。
    uint32_t      biClrUsed;//指定本图象实际用到的颜色数，如果该值为零，则用到的颜色数为2的biBitCount指数次幂
    uint32_t      biClrImportant;//指定本图象中重要的颜色数，如果该值为零，则认为所有的颜色都是重要的。
} BMPINFOHEADER_T;
#pragma pack (pop)

int saveBMP(const char* name, uint8_t * data, int width, int height, int pixelCount)
{
    //int widthStep = (((width * 24) + 31) & (~31)) / 8 ; //每行实际占用的大小（每行都被填充到一个4字节边界）
    int size = width*height*pixelCount; // 每个像素点3个字节

    // 位图第一部分，文件信息
    BMPFILEHEADER_T bfh;
    bfh.bfType = 0x4d42;  //bm
    bfh.bfSize = size  // data size
                 + sizeof(BMPFILEHEADER_T) // first section size
                 + sizeof(BMPINFOHEADER_T); // second section size
    bfh.bfReserved1 = 0; // reserved
    bfh.bfReserved2 = 0; // reserved
    bfh.bfOffBits = sizeof(BMPFILEHEADER_T) + sizeof(BMPINFOHEADER_T);

    // 位图第二部分，数据信息
    BMPINFOHEADER_T bih;
    bih.biSize = sizeof(BMPINFOHEADER_T);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = pixelCount * 8;
    bih.biCompression = 0;
    bih.biSizeImage = size;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    FILE * fp = fopen(name,"wb");
    if(!fp) return -1;

    fwrite( &bfh, 1, sizeof(BMPFILEHEADER_T), fp);
    fwrite( &bih, 1, sizeof(BMPINFOHEADER_T), fp);
    fwrite( data, 1, size, fp);

    fclose( fp );

    return 0;
}
