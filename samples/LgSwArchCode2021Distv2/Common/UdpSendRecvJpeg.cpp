//------------------------------------------------------------------------------------------------
// File: UdpSendRecvJpeg.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Send and receives OpenCV Mat Images in a UDP message commpressed as Jpeg images 
//------------------------------------------------------------------------------------------------
#include <opencv2/highgui/highgui.hpp>
#include "UdpSendRecvJpeg.h"
static  int init_values[2] = { cv::IMWRITE_JPEG_QUALITY,80 }; //default(95) 0-100
static  std::vector<int> param (&init_values[0], &init_values[0]+2);
static  std::vector<uchar> sendbuff;//buffer for coding

//-----------------------------------------------------------------
// UdpSendImageAsJpeg - Sends a Open CV Mat Image commressed as a 
// jpeg image in side a UDP Message on the specified UDP local port
// and Destination. return bytes sent on success and -1 on failure
//-----------------------------------------------------------------
int UdpSendImageAsJpeg(TUdpLocalPort * UdpLocalPort,TUdpDest *dest,cv::Mat Image)
{
    
    cv::imencode(".jpg", Image, sendbuff, param);
    return(SendUDPMsg(UdpLocalPort,dest,sendbuff.data(), sendbuff.size()));
}

//-----------------------------------------------------------------
// END UdpSendImageAsJpeg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// UdpRecvImageAsJpeg - Sends a Open CV Mat Image commressed as a 
// jpeg image in side a UDP Message on the specified UDP local port
// returns true on success and false on failure
//-----------------------------------------------------------------
bool UdpRecvImageAsJpeg(TUdpLocalPort * UdpLocalPort,cv::Mat *Image,struct sockaddr *src_addr, socklen_t *addrlen)
{
  #define BUFSIZE (1024*64)
  unsigned char buf[BUFSIZE];	/* receive buffer */   
  int recvlen;
  
  if((recvlen=RecvUDPMsg(UdpLocalPort,buf,BUFSIZE,src_addr,addrlen))>0)
   {
     cv::imdecode(cv::Mat(recvlen,1,CV_8UC1,buf), cv::IMREAD_COLOR, Image );
     if (!(*Image).empty()) return true;
     else return false;
   }
   return false;
}

//-----------------------------------------------------------------
// END UdpRecvImageAsJpeg
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
