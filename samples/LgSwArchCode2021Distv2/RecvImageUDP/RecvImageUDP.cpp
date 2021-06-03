//------------------------------------------------------------------------------------------------
// File: RecvImageUDP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// This program receives a jpeg image via a UDP Message and displays it. The jpeg image must fit
// within one UDP message and be less than 64KB
//----------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp> // needed for resize
#include <iostream>
#include "NetworkUDP.h"
#include "UdpSendRecvJpeg.h"


using namespace cv;
using namespace std;
//----------------------------------------------------------------
// main - This is the main program for the RecvImageUDP demo 
// program  contains the control loop
//-----------------------------------------------------------------

int main(int argc, char* argv[])
{
    Size size(320 * 2, 240 * 2);//the dst image size,e.g.100x100
    Mat dst;//dst image
    TUdpLocalPort* UdpLocalPort = NULL;
    struct sockaddr_in remaddr;	/* remote address */
    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
    bool retvalue;

    if (argc != 2)
    {
        fprintf(stderr, "usage %s port\n", argv[0]);
        exit(0);
    }

    if ((UdpLocalPort = OpenUdpPort(atoi(argv[1]))) == NULL)  // Open UDP Network port
    {
        printf("OpenUdpPort Failed\n");
        return(-1);
    }

    namedWindow("Server", WINDOW_AUTOSIZE);// Create a window for display.

    Mat Image;
    do {
        retvalue = UdpRecvImageAsJpeg(UdpLocalPort, &Image, (struct sockaddr*) & remaddr, &addrlen);

        if (retvalue)
        {
            resize(Image, dst, size);//resize imag
            imshow("Server", dst); // If a valid image is received then display it
        }

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

