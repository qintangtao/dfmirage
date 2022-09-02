#ifndef __PIXELFORMAT_H__
#define __PIXELFORMAT_H__

struct PixelFormat
{
  PixelFormat();
  // Initializes the bigEndian field by native machine value.
  void initBigEndianByNative();

  bool isEqualTo(const PixelFormat *pf) const;

  unsigned short bitsPerPixel;
  unsigned short colorDepth;

  unsigned short redMax;
  unsigned short greenMax;
  unsigned short blueMax;

  unsigned short redShift;
  unsigned short greenShift;
  unsigned short blueShift;

  bool bigEndian;
};

#endif // __PIXELFORMAT_H__
