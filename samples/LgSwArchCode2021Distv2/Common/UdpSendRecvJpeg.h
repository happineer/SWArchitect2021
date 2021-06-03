//------------------------------------------------------------------------------------------------
// File: UdpSendRecvJpeg.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and receive jpeg images
//------------------------------------------------------------------------------------------------
#ifndef UdpSendRecvJpegH
#define UdpSendRecvJpegH

#include <opencv2/core/core.hpp>
#include "NetworkUDP.h"

int UdpSendImageAsJpeg(TUdpLocalPort * UdpLocalPort,TUdpDest *dest, cv::Mat Image);
bool UdpRecvImageAsJpeg(TUdpLocalPort * UdpLocalPort,cv::Mat *Image,struct sockaddr *src_addr, socklen_t *addrlen);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
