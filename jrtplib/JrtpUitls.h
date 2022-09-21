#ifndef JRTPUITLS_H
#define JRTPUITLS_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtperrors.h>
#include <jrtplib3/rtplibraryversion.h>
#include <jrtplib3/rtppacket.h>

using namespace jrtplib;

class JrtpUitls : public RTPSession
{
public:
    JrtpUitls(uint16_t localport);
    ~JrtpUitls();
    
    int addDestination(std::string ipstr,uint16_t port);
    void setMaxRTPPackLen(int len);
    void sendH264Nalu(unsigned char* h264Buf,int buflen,bool isMask);

private:
    int m_maxRTPPackLen;
    uint16_t m_portbase;
    RTPSessionParams sessparams;
    RTPUDPv4TransmissionParams transparams;

    void checkerror(int rtperr);
};

#endif
