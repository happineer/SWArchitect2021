//------------------------------------------------------------------------------------------------
// File: RecvImageTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// This program receives a jpeg image via a TCP Stream and displays it. 
//----------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"


using namespace cv;
using namespace std;
//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

int main(int argc, char *argv[])
{
 TTcpConnectedPort *TcpConnectedPort=NULL;
 bool retvalue;

   if (argc !=3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

  if  ((TcpConnectedPort=OpenTcpConnection(argv[1],argv[2]))==NULL)  // Open UDP Network port
     {
       printf("OpenTcpConnection\n");
       return(-1); 
     }

  namedWindow( "Server", WINDOW_AUTOSIZE );// Create a window for display.
 
  Mat Image;
do {
    retvalue=TcpRecvImageAsJpeg(TcpConnectedPort,&Image);
   
    if( retvalue) imshow( "Server", Image ); // If a valid image is received then display it
    else break;

   } while (waitKey(10) != 'q'); // loop until user hits quit

 CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
 return 0; 
}
//-----------------------------------------------------------------
// END main
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
