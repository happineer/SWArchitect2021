//------------------------------------------------------------------------------------------------
// File: NetworkTCP.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive UDP Packets for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#ifndef NetworkTCPH
#define NetworkTCPH


#if  defined(_WIN32) || defined(_WIN64)
#pragma comment (lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
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
 SOCKET_FD_TYPE ListenFd;
} TTcpListenPort;

typedef struct
{
 SOCKET_FD_TYPE ConnectedFd;
} TTcpConnectedPort;

//------------------------------------------------------------------------------------------------
//  Function Prototypes 
//------------------------------------------------------------------------------------------------
TTcpListenPort *OpenTcpListenPort(short localport);
void CloseTcpListenPort(TTcpListenPort **TcpListenPort);
TTcpConnectedPort *AcceptTcpConnection(TTcpListenPort *TcpListenPort, 
                       struct sockaddr_in *cli_addr,socklen_t *clilen);
TTcpConnectedPort *OpenTcpConnection(const char *remotehostname, const char * remoteportno);
void CloseTcpConnectedPort(TTcpConnectedPort **TcpConnectedPort);
ssize_t ReadDataTcp(TTcpConnectedPort *TcpConnectedPort,unsigned char *data, size_t length);
ssize_t WriteDataTcp(TTcpConnectedPort *TcpConnectedPort,unsigned char *data, size_t length);
#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------




