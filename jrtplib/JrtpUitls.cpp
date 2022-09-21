#include "JrtpUitls.h"



JrtpUitls::JrtpUitls(uint16_t localport) 
	: m_maxRTPPackLen(1360)
	, m_portbase(localport)
{
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	int wsa = WSAStartup(MAKEWORD(2, 2), &dat);
	if (0 != wsa) {
		fprintf(stderr, "WSAStartup failed [%d]\n", wsa);
		exit(-1);
	}
#endif // RTP_SOCKETTYPE_WINSOCK

    transparams.SetPortbase(m_portbase);

    sessparams.SetOwnTimestampUnit(1.0 / 90000.0);
    // sessparams.SetAcceptOwnPackets(true);

    int status = this->Create(sessparams, &transparams);
    checkerror(status);

    this->SetDefaultPayloadType(96);
    this->SetDefaultMark(false);
    // this->SetDefaultTimestampIncrement(90000.0 / 30.0);
    this->SetDefaultTimestampIncrement(10);
    
}

JrtpUitls::~JrtpUitls()
{
    this->BYEDestroy(RTPTime(10, 0), 0, 0);
}

int JrtpUitls::addDestination(std::string ipstr, uint16_t port)
{
    uint32_t destip = inet_addr(ipstr.c_str());
    if (destip == INADDR_NONE)
    {
		std::cerr << "Bad IP address specified" << std::endl;
        return -1;
    }
    destip = ntohl(destip);
    uint16_t destport = port;
    RTPIPv4Address addr(destip, destport);
    int status = this->AddDestination(addr);
    checkerror(status);
}

void JrtpUitls::setMaxRTPPackLen(int len)
{
    m_maxRTPPackLen = len;
}

void JrtpUitls::sendH264Nalu(unsigned char *h264Buf, int buflen,bool isMask)
{
    //unsigned char *pSendbuf; //发送数据指针
    //pSendbuf = h264Buf;
    // printf("send packet length %d \n", buflen);
    //char sendbuf[1430]; //发送的数据缓冲
    //memset(sendbuf, 0, 1430);
    int status;

    // for(int y = 0; y < buflen; y++)
    // {
    //     printf("%02x ",h264Buf[y]);
    // }
    // printf("\n");

    if (buflen <= m_maxRTPPackLen)
    {      
        this->SetDefaultMark(isMask);
#if 1
		status = this->SendPacket((void *)h264Buf, buflen);
#else
        memcpy(sendbuf, pSendbuf, buflen);
        status = this->SendPacket((void *)sendbuf, buflen);
#endif
        checkerror(status);
    }
    else
    {
        int k = 0, l = 0, j = 0, sendlen = 0;
        k = buflen / m_maxRTPPackLen;
        l = buflen % m_maxRTPPackLen;
        j = l > 0 ? k + 1 : k;
        
        // printf("j=%d\r\n",j);
#if 0
        this->SetDefaultMark(false);
        sendlen = m_maxRTPPackLen;
        
        for (int i = 0; i < j; i++)
        {
            if (i == (j - 1))
            {      
                this->SetDefaultMark(isMask);
                if (l > 0)
                    sendlen = buflen - i * m_maxRTPPackLen;
            }

            memcpy(sendbuf, &pSendbuf[i * m_maxRTPPackLen], sendlen);
            status = this->SendPacket((void *)sendbuf, sendlen);
            checkerror(status);
        }
#else
        for (int i = 0; i < j; i++)
        {
            if (i < (j - 1))
            {
                this->SetDefaultMark(false);
                sendlen = m_maxRTPPackLen;
            }
            else
            {      
                this->SetDefaultMark(isMask);
                if (l > 0)
                    sendlen = buflen - i * m_maxRTPPackLen;
                else
                    sendlen = m_maxRTPPackLen;
            }
			
#if 1
			status = this->SendPacket((void *)(h264Buf + i * m_maxRTPPackLen), sendlen);
#else
			std::cout << "sendH264Nalu1: " << i << "  " << j << "  " << sendlen << "  " << i * m_maxRTPPackLen << std::endl;
			memcpy(sendbuf, &pSendbuf[i * m_maxRTPPackLen], sendlen);
			std::cout << "sendH264Nalu2: " << i << "  " << j << "  " << sendlen << std::endl;
            status = this->SendPacket((void *)sendbuf, sendlen);
#endif
			//std::cout << "sendH264Nalu3: " << i << "  " << j << "  " << sendlen << "  " <<  status << std::endl;
            checkerror(status);
        }
#endif        
    }
}

void JrtpUitls::checkerror(int rtperr)
{
    if (rtperr < 0)
    {
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
        exit(-1);
    }
}
