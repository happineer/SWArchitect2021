//------------------------------------------------------------------------------------------------
// File: SendImageUDP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// This program Sends a jpeg image From the Camera via a UDP Message to a remote destination. 
// The jpeg image from the camera must fit within one UDP message and be less than 64KB
//----------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "NetworkUDP.h"
#include "UdpSendRecvJpeg.h"


#define WIDTH 640/2
#define HEIGHT 480/2



using namespace cv;
using namespace std;
//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//---------------------------------------------------------------

int main(int argc, char *argv[])
{ 
  Mat            image;          // camera image in Mat format 
  VideoCapture   capture;
  int deviceID = 0;             // 0 = open default camera
  int apiID = cv::CAP_ANY;      // 0 = autodetect default API
  TUdpLocalPort *UdpLocalPort=NULL;
  TUdpDest      *UdpDest=NULL;
  int            AWidth;
  int            AHeight;

    if (argc !=3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }


    if  ((UdpLocalPort=OpenUdpPort(0))==NULL)  // Open UDP Network port
     {
       printf("OpenUdpPort Failed\n");
       return(-1); 
     }

    if  ((UdpDest=GetUdpDest(argv[1],argv[2]))==NULL)  // Setup remote network destination to send images
     {
       printf("GetUdpDest Failed\n");
       return(-1); 
     }

    // open selected camera using selected API
    capture.open(deviceID + apiID);
    // check if we succeeded
   if (!capture.isOpened()) 
    {
        printf("ERROR! Unable to open camera\n");
        return -1;
    }

   if (capture.set(CAP_PROP_FRAME_WIDTH,WIDTH)==0) // Set camera width 
     {
      printf("capture.set(CAP_PROP_FRAME_WIDTH,WIDTH) Failed)\n");
     }

   if (capture.set(CAP_PROP_FRAME_HEIGHT,HEIGHT)==0) // Set camera height
     {
      printf("capture.set(CAP_PROP_FRAME_HEIGHT,HEIGHT) Failed)\n");
     }

   AWidth=capture.get(CAP_PROP_FRAME_WIDTH);
   AHeight=capture.get(CAP_PROP_FRAME_HEIGHT);

   printf("Width = %d\n",AWidth );
   printf("Height = %d\n", AHeight);

  do
   {
    // wait for a new frame from camera and store it into 'frame'
    capture.read(image);
    // check if we succeeded
    if (image.empty()) 
      {
       printf("ERROR! blank frame grabbed\n");
       continue;
      }

    UdpSendImageAsJpeg(UdpLocalPort,UdpDest,image);   // Send processed UDP image to detination

   } while (waitKey(10) != 'q'); // loop until user hits quit

 CloseUdpPort(&UdpLocalPort); // Close network port;
 return 0; 
}
//-----------------------------------------------------------------
// END main
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
