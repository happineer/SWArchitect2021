//------------------------------------------------------------------------------------------------
// File: NetworkTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive TCP byte streams for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#include <iostream>
#include <new>
#include <stdio.h>
#include <string.h>
#include "NetworkTCP.h"
//-----------------------------------------------------------------
// OpenTCPListenPort - Creates a Listen TCP port to accept
// connection requests
//-----------------------------------------------------------------
TTcpListenPort *OpenTcpListenPort(short localport)
{
  TTcpListenPort *TcpListenPort;
  struct sockaddr_in myaddr;

  TcpListenPort= new (std::nothrow) TTcpListenPort;  
  
  if (TcpListenPort==NULL)
     {
      fprintf(stderr, "TUdpPort memory allocation failed\n");
      return(NULL);
     }
  TcpListenPort->ListenFd=BAD_SOCKET_FD;
  #if  defined(_WIN32) || defined(_WIN64)
  WSADATA wsaData;
  int     iResult;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) 
    {
     delete TcpListenPort;
     printf("WSAStartup failed: %d\n", iResult);
     return(NULL);
    }
#endif
  // create a socket
  if ((TcpListenPort->ListenFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
     {
      CloseTcpListenPort(&TcpListenPort);
      perror("socket failed");
      return(NULL);  
     }
  int option = 1; 

   if(setsockopt(TcpListenPort->ListenFd,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(option)) < 0)
     {
      CloseTcpListenPort(&TcpListenPort);
      perror("setsockopt failed");
      return(NULL);
     }

  // bind it to all local addresses and pick any port number
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(localport);

  if (bind(TcpListenPort->ListenFd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
      CloseTcpListenPort(&TcpListenPort);
      perror("bind failed");
      return(NULL); 
    }
   
 
  if (listen(TcpListenPort->ListenFd,5)< 0)
  {
      CloseTcpListenPort(&TcpListenPort);
      perror("bind failed");
      return(NULL);	  
  }
  return(TcpListenPort);
}
//-----------------------------------------------------------------
// END OpenTCPListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpListenPort - Closes the specified TCP listen port
//-----------------------------------------------------------------
void CloseTcpListenPort(TTcpListenPort **TcpListenPort)
{
  if ((*TcpListenPort)==NULL) return;
  if ((*TcpListenPort)->ListenFd!=BAD_SOCKET_FD)  
     {
      CLOSE_SOCKET((*TcpListenPort)->ListenFd);
      (*TcpListenPort)->ListenFd=BAD_SOCKET_FD;
     }
   delete (*TcpListenPort);
  (*TcpListenPort)=NULL;
#if  defined(_WIN32) || defined(_WIN64)
   WSACleanup();
#endif
}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// AcceptTcpConnection -Accepts a TCP Connection request from a 
// Listening port
//-----------------------------------------------------------------
TTcpConnectedPort AcceptTcpConnection(TTcpListenPort *TcpListenPort, 
                       struct sockaddr_in *cli_addr,socklen_t *clilen)
{
  TTcpConnectedPort TcpConnectedPort = -1;
  
  TcpConnectedPort= accept(TcpListenPort->ListenFd,
                      (struct sockaddr *) cli_addr,clilen);
					  
  if (TcpConnectedPort== BAD_SOCKET_FD) 
  {
	perror("ERROR on accept");
	return -1;
  }
  
 int bufsize = 200 * 1024;
 if (setsockopt(TcpConnectedPort, SOL_SOCKET, 
                 SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(-1);
	}
 if (setsockopt(TcpConnectedPort, SOL_SOCKET, 
                 SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(-1);
	}


 return TcpConnectedPort;
}
//-----------------------------------------------------------------
// END AcceptTcpConnection
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// OpenTCPConnection - Creates a TCP Connection to a TCP port
// accepting connection requests
//-----------------------------------------------------------------
TTcpConnectedPort OpenTcpConnection(const char *remotehostname, const char * remoteportno)
{
  TTcpConnectedPort TcpConnectedPort;
  struct sockaddr_in myaddr;
  int                s;
  struct addrinfo   hints;
  struct addrinfo   *result = NULL;
 
  TcpConnectedPort=BAD_SOCKET_FD;
 
  // create a socket
   memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  
  s = getaddrinfo(remotehostname, remoteportno, &hints, &result);
  if (s != 0) 
    {
      fprintf(stderr, "getaddrinfo: Failed\n");
      return(-1);
    }
  if ( result==NULL)
    {
      fprintf(stderr, "getaddrinfo: Failed\n");
      return(-1);
    }
  if ((TcpConnectedPort = socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
     {
      CloseTcpConnectedPort(TcpConnectedPort);
	  freeaddrinfo(result);
      perror("socket failed");
      return(-1);  
     }

  int bufsize = 200 * 1024;
  if (setsockopt(TcpConnectedPort, SOL_SOCKET,
                 SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(-1);
	}
  if (setsockopt(TcpConnectedPort, SOL_SOCKET, 
                 SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
         CloseTcpConnectedPort(TcpConnectedPort);
         perror("setsockopt SO_SNDBUF failed");
         return(-1);
	}
	 
  if (connect(TcpConnectedPort,result->ai_addr,result->ai_addrlen) < 0) 
          {
	    CloseTcpConnectedPort(TcpConnectedPort);
	    freeaddrinfo(result);
            perror("connect failed");
            return(-1);
	  }
  freeaddrinfo(result);	 
  return(TcpConnectedPort);
}
//-----------------------------------------------------------------
// END OpenTcpConnection
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpConnectedPort - Closes the specified TCP connected port
//-----------------------------------------------------------------
void CloseTcpConnectedPort(TTcpConnectedPort TcpConnectedPort)
{
  if ((TcpConnectedPort)!=BAD_SOCKET_FD)  
     {
      CLOSE_SOCKET((TcpConnectedPort));
      (TcpConnectedPort)=BAD_SOCKET_FD;
     }
}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// ReadDataTcp - Reads the specified amount TCP data 
//-----------------------------------------------------------------
ssize_t ReadDataTcp(TTcpConnectedPort TcpConnectedPort,unsigned char *data, size_t length)
{
 ssize_t bytes;
 
 for (size_t i = 0; i < length; i += bytes)
    {
      if ((bytes = recv(TcpConnectedPort, (char *)(data+i), length  - i,0)) == -1) 
      {
       	return (-1);
      }
	  if (bytes == 0) {
		 printf("[%s]: diconnected\n", __func__);
		 return 0;
	  }
    }
  return(length);
}
//-----------------------------------------------------------------
// END ReadDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// WriteDataTcp - Writes the specified amount TCP data 
//-----------------------------------------------------------------
ssize_t WriteDataTcp(TTcpConnectedPort TcpConnectedPort,unsigned char *data, size_t length)
{
  ssize_t total_bytes_written = 0;
  ssize_t bytes_written;
  while (total_bytes_written != length)
    {
     bytes_written = send(TcpConnectedPort,
	                               (char *)(data+total_bytes_written),
                                  length - total_bytes_written,0);
     if (bytes_written == -1)
       {
	   	printf("[%s]: errno: %d %s\n", __func__, errno, strerror(errno));
       return(-1);
      }
     total_bytes_written += bytes_written;
   }
   return(total_bytes_written);
}
//-----------------------------------------------------------------
// END WriteDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------


