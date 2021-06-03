//------------------------------------------------------------------------------------------------
// File: NetworkUDP.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive UDP Packets for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#ifndef NetworkUDPH
#define NetworkUDPH


#if  defined(_WIN32) || defined(_WIN64)
#pragma comment (lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>
#define  CLOSE_SOCKET closesocket
#define  SOCKET_FD_TYPE SOCKET
#define  BAD_SOCKET_FD INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <unistd.h>
#define  CLOSE_SOCKET close
#define  SOCKET_FD_TYPE int
#define  BAD_SOCKET_FD  -1
#endif

//------------------------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------------------------

typedef struct
{
 SOCKET_FD_TYPE UdpFd;
} TUdpLocalPort;

typedef struct
{
 struct sockaddr_in remaddr;
} TUdpDest;

//------------------------------------------------------------------------------------------------
//  Function Prototypes 
//------------------------------------------------------------------------------------------------
TUdpLocalPort* OpenUdpPort(short localport);

void CloseUdpPort(TUdpLocalPort **UdpLocalPort);

TUdpDest * GetUdpDest(const char *remotehostname, const char * remoteportno);

void DeleteUdpDest(TUdpDest **dest);

int SendUDPMsg(TUdpLocalPort * UdpLocalPort,TUdpDest *dest, unsigned char *msg, size_t length);

int RecvUDPMsg(TUdpLocalPort * UdpLocalPort, unsigned char *msg, size_t length,struct sockaddr *src_addr, socklen_t *addrlen);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
