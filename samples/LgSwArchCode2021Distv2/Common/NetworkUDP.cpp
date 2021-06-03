//------------------------------------------------------------------------------------------------
// File: NetworkUDP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive UDP Packets for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#include <iostream>
#include <new>
#include <stdio.h>
#include <string.h>
#include "NetworkUDP.h"
//-----------------------------------------------------------------
// OpenUdpPort - Open the specified local UDP port on a socket,
// increases the internal send/receive buffer sizes and then 
// returns a handle to the port on success and NULL on failure
//-----------------------------------------------------------------
TUdpLocalPort* OpenUdpPort(short localport)
{
  TUdpLocalPort *UdpLocalPort;
  struct sockaddr_in myaddr;

  UdpLocalPort= new (std::nothrow) TUdpLocalPort;  
  
  if (UdpLocalPort==NULL)
     {
      fprintf(stderr, "TUdpPort memory allocation failed\n");
      return(NULL);
     }
  UdpLocalPort->UdpFd=BAD_SOCKET_FD;
#if  defined(_WIN32) || defined(_WIN64)
  WSADATA wsaData;
  int     iResult;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) 
    {
     delete UdpLocalPort;
     printf("WSAStartup failed: %d\n", iResult);
     return(NULL);
    }
#endif
  // create a socket
  if ((UdpLocalPort->UdpFd= socket(AF_INET, SOCK_DGRAM, 0)) == BAD_SOCKET_FD)
     {
      CloseUdpPort(&UdpLocalPort);
      perror("socket failed");
      return(NULL);  
     }
  // bind it to all local addresses and pick any port number
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(localport);

  if (bind(UdpLocalPort->UdpFd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
      CloseUdpPort(&UdpLocalPort);
      perror("bind failed");
      return(NULL); 
    }

  int bufsize = 200 * 1024;
  if (setsockopt(UdpLocalPort->UdpFd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseUdpPort(&UdpLocalPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}
  if (setsockopt(UdpLocalPort->UdpFd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseUdpPort(&UdpLocalPort);
         perror("setsockopt SO_SNDBUF failed");
         return(NULL);
	}

  return(UdpLocalPort);
}
//-----------------------------------------------------------------
// END OpenUdpPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseUdpPort - Closes the specified local UDP port
//-----------------------------------------------------------------
void CloseUdpPort(TUdpLocalPort **UdpLocalPort)
{
  if ((*UdpLocalPort)==NULL) return;
  if ((*UdpLocalPort)->UdpFd!=BAD_SOCKET_FD)  
     {
      CLOSE_SOCKET((*UdpLocalPort)->UdpFd);
      (*UdpLocalPort)->UdpFd=BAD_SOCKET_FD;
     }
   delete (*UdpLocalPort);
  (*UdpLocalPort)=NULL;
#if  defined(_WIN32) || defined(_WIN64)
   WSACleanup();
#endif
}
//-----------------------------------------------------------------
// END CloseUdpPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// GetUdpDest - Provides a handle the remote jhost and port on
// success and NULL on failure
//-----------------------------------------------------------------
TUdpDest * GetUdpDest(const char *remotehostname, const char * remoteportno)
{
   TUdpDest * dest;
   int                s;
   struct addrinfo   hints;
   struct addrinfo   *result = NULL;
   dest= new (std::nothrow) TUdpDest;  

  if (dest==NULL)
     {
      fprintf(stderr, "TUdpDest memory allocation failed\n");
      return(NULL);
     }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  
  s = getaddrinfo(remotehostname, remoteportno, &hints, &result);
  if (s != 0) 
    {
      fprintf(stderr, "getaddrinfo: Failed\n");
      delete dest;
      return(NULL);
    }
  if ( result==NULL)
    {
      fprintf(stderr, "getaddrinfo: Failed\n");
      delete dest;
      return(NULL);
    }

  memmove(&dest->remaddr, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);	
  return(dest);

}
//-----------------------------------------------------------------
// END GetUdpDest
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// DeleteUdpDest - Deletes the handle to the remote host and port
//-----------------------------------------------------------------
void DeleteUdpDest(TUdpDest **dest)
{
  if ((*dest)==NULL) return;
   delete (*dest);
   (*dest)=NULL;
}
//-----------------------------------------------------------------
// END DeleteUdpDest
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// SendUDPMsg - Sends a UDP Message on the specified UDP local port
// and Destination. return bytes sent on success and -1 on failure
//-----------------------------------------------------------------
int SendUDPMsg(TUdpLocalPort * UdpLocalPort,TUdpDest *dest, unsigned char *msg, size_t length)
{
  int  slen;
  int  retval;
  if ((dest==NULL) || (UdpLocalPort==NULL)) return(-1);

  slen = sizeof(dest->remaddr);
  if (UdpLocalPort->UdpFd== BAD_SOCKET_FD) return(-1);
  if ((retval=sendto(UdpLocalPort->UdpFd, (const char *)msg, (int)length, 0, (struct sockaddr *)&dest->remaddr, slen)) == -1)
     {
      perror("sendto");
      return(-1);
     }
  return(retval);
}
//-----------------------------------------------------------------
// END SendUDPMsg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// RecvUDPMsg - Receives a UDP Message on the specified UDP local port
// return bytes received on success and -1 on failure
//-----------------------------------------------------------------
int RecvUDPMsg(TUdpLocalPort * UdpLocalPort, unsigned char *msg, size_t length,struct sockaddr *src_addr, socklen_t *addrlen)
{
  int  recvlen;
  if (UdpLocalPort==NULL) return(-1);
  if (UdpLocalPort->UdpFd== BAD_SOCKET_FD) return(-1);

  if ((recvlen = recvfrom(UdpLocalPort->UdpFd, (char *)msg, length, 0, src_addr, addrlen)) == -1)
     {
      perror("recvfrom");
      return(-1);
     }
  return(recvlen);
}
//-----------------------------------------------------------------
// END  RecvUDPMsg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------

