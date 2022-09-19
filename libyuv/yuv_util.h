//
// Created by Administrator on 2022/4/14.
//

#ifndef NDKDEMO_YUV_UTIL_H
#define NDKDEMO_YUV_UTIL_H

#include <stdint.h>

int NV21ToI420(const uint8_t *src_nv21, int width, int height, uint8_t *dst_i420);

int I420Scale(const uint8_t *src_i420, int width, int height, uint8_t *dst_i420, int dst_width, int dst_height, int mode);

int I420ToRGB565(const uint8_t *src_i420, int width, int height, uint8_t *dst_rgb565);

int I420ToRGB24(const uint8_t *src_i420, int width, int height, uint8_t *dst_rgb24);

int I420ToABGR(const uint8_t *src_i420, int width, int height, uint8_t *dst_rgba);

int ABGRToI420(const uint8_t *src_abgr, int width, int height, uint8_t *dst_i420);

int RGBAToI420(const uint8_t *src_rgba, int width, int height, uint8_t *dst_i420);

int ARGBToI420(const uint8_t *src_argb, int width, int height, uint8_t *dst_i420);

int saveBMP(const char* name, uint8_t * data, int width, int height, int pixelCount);

#endif //NDKDEMO_YUV_UTIL_H
