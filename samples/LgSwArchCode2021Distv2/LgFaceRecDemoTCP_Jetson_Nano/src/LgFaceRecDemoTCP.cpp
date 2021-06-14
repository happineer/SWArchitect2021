#include <time.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <dlib/svm_threaded.h>
#include <dlib/svm.h>
#include <vector>

#include "mtcnn.h"
#include "kernels.h"

#include "gstCamera.h"
#include "glDisplay.h"
#include "loadImage.h"
#include "cudaRGB.h"
#include "cudaMappedMemory.h"

#include "face_embedder.h"
#include "face_classifier.h"
#include "alignment.h"
#include "videoSource.h"

#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"
#include "cudaMappedMemory.h"
#include "cudaColorspace.h"
#include <memory>

unsigned int FrameCount=0;
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

typedef struct {
 ifstream    mpegfile;
 int         width;
 int         height;
 void        *inputImgGPU;
 void        *output;
 imageFormat inputFormat;
 size_t      inputImageSize;

} TMotionJpegFileDesc;

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool OpenMotionJpegFile(TMotionJpegFileDesc *FileDesc,char * Filename, int *Width, int *Height)
{
  if ((!FileDesc) || (!Width) ||(!Height))  return false;

  FileDesc->mpegfile.open(Filename, ios::in | ios::binary);
  if (!FileDesc->mpegfile.is_open())
     {
      
      return false;
     }

 FileDesc->mpegfile.read((char*)&FileDesc->width, sizeof(FileDesc->width));
 if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->width)) 
 {
   FileDesc->mpegfile.close();
   return(false);
 }


 FileDesc->mpegfile.read((char*)&FileDesc->height, sizeof(FileDesc->height));
 if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->height)) 
 {
   FileDesc->mpegfile.close();
   return(false);
 }


 FileDesc->width=ntohl(FileDesc->width);
 FileDesc->height=ntohl(FileDesc->height);
 *Height=FileDesc->height;
 *Width=FileDesc->width;
 FileDesc->inputFormat = IMAGE_RGB8;
 FileDesc->inputImageSize = (FileDesc->width * FileDesc->height*(sizeof(uchar3) * 8))/8;
 FileDesc->inputImgGPU = NULL;
 FileDesc->output= NULL;

 // allocate CUDA buffer for the image
 const size_t imgSize = (FileDesc->width * FileDesc->height*(sizeof(float4) * 8))/8;

 // convert from uint8 to float

 if( !cudaAllocMapped(&FileDesc->inputImgGPU, FileDesc->inputImageSize) )
        {
	 printf("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", FileDesc->inputImageSize);
	 return false;
        }

 if( !cudaAllocMapped(&FileDesc->output, imgSize) )
	{
	 LogError(LOG_IMAGE "loadImage() -- failed to allocate %zu bytes for image \n", imgSize);
	 return false;
	}

 printf("Open width %d height %d\n",*Width,*Height);
 return true;

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool CloseMotionJpegFile(TMotionJpegFileDesc *FileDesc)
{
 if (!FileDesc)  return false;
 FileDesc->mpegfile.close();

 CUDA(cudaFreeHost(FileDesc->inputImgGPU));
 CHECK(cudaFreeHost(FileDesc->output))

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool LoadMotionJpegFrame(TMotionJpegFileDesc *FileDesc, float4**  output)
{
        unsigned int imagesize;
        unsigned char* buff;

	// validate parameters
	if( !FileDesc)
	{
		printf("LOG_IMAGE LoadMJpegFrame() - invalid parameter(s)\n");
		return false;
	}
	
        FileDesc->mpegfile.read((char*)&imagesize, sizeof(imagesize));
        if (FileDesc->mpegfile.gcount() != sizeof(imagesize)) return(0);
        imagesize = ntohl(imagesize);
        buff = new (std::nothrow) unsigned char[imagesize];
        if (buff == NULL) return 0;
        FileDesc->mpegfile.read((char*)buff, imagesize);
        if (FileDesc->mpegfile.gcount() != imagesize)
        {
         delete[] buff;
         return false;
        }

      cv::Mat img;
      cv::imdecode(cv::Mat(imagesize, 1, CV_8UC1, buff), cv::IMREAD_COLOR, &img);
      delete[] buff;
      if (img.empty()) 
             {
              printf("cv::imdecode failed\n");
              return(false);
             }


        memcpy(FileDesc->inputImgGPU, img.data, imageFormatSize(FileDesc->inputFormat, FileDesc->width, FileDesc->height));

	if( CUDA_FAILED(cudaConvertColor(FileDesc->inputImgGPU, FileDesc->inputFormat, FileDesc->output, IMAGE_RGBA32F, FileDesc->width, FileDesc->height)) )
		{
			printf("LOG_IMAGE loadImage() -- failed to convert image \n");
			return false;

		}

         *output=(float4*)FileDesc->output;

	return true;

}
/***********************************************************************************************/
/***********************************************************************************************/


// create high performace jetson camera - loads the image directly to gpu memory or shared memory
gstCamera* getCamera(){
    gstCamera* camera = gstCamera::Create(gstCamera::DefaultWidth, gstCamera::DefaultHeight, NULL);
	if( !camera ){
		printf("\nfailed to initialize camera device\n");
	}else{
        printf("\nsuccessfully initialized camera device\n");
        printf("    width:  %u\n", camera->GetWidth());
        printf("   height:  %u\n", camera->GetHeight());
            //start streaming
	    if( !camera->Open() ){
            printf("failed to open camera for streaming\n");
	    }else{
            printf("camera open for streaming\n");
        }
    }
    return camera;
}


// perform face recognition with Raspberry Pi camera
int camera_face_recognition(int argc, char *argv[])
  {
   TTcpListenPort    *TcpListenPort;
   TTcpConnectedPort *TcpConnectedPort;
   struct sockaddr_in cli_addr;
   socklen_t          clilen;
   short              listen_port;
   bool               usecamera=false;

    
    // -------------- Initialization -------------------
    
    face_embedder embedder;                         // deserialize recognition network 
    face_classifier classifier(&embedder);          // train OR deserialize classification SVM's 
    if(classifier.need_restart() == 1) return 1;    // small workaround - if svms were trained theres some kind of memory problem when generate mtcnn

    gstCamera* camera=NULL;
    TMotionJpegFileDesc MotionJpegFd;
    bool user_quit = false;
    int imgWidth ;
    int imgHeight ;

   if (argc <2) 
    {
       fprintf(stderr,"usage %s [port] [filename]\n", argv[0]);
       exit(0);
    }

    listen_port =atoi(argv[1]);
 
    if (argc==2) usecamera=true;

    if (usecamera)
      {
        printf("video\n");

        camera = getCamera();                // create jetson camera - PiCamera. USB-Cam needs different operations in Loop!! not implemented!

        if( !camera )    
         {
          printf("load camera failed\n");
          return -1;
         }

        imgWidth = camera->GetWidth();
        imgHeight = camera->GetHeight();
       }
    else
       {
        if (!OpenMotionJpegFile(&MotionJpegFd,argv[2], &imgWidth, &imgHeight))
          {
            printf("ERROR! Unable to open file %s\n",argv[2]);
            return -1;
          }

       }
    
     mtcnn finder(imgHeight, imgWidth);              // build OR deserialize TensorRT detection network

    // malloc shared memory for images for access with cpu and gpu without copying data
    // cudaAllocMapped is used from jetson-inference
    uchar* rgb_gpu = NULL;
    uchar* rgb_cpu = NULL;
    cudaAllocMapped( (void**) &rgb_cpu, (void**) &rgb_gpu, imgWidth*imgHeight*3*sizeof(uchar) );
    uchar* cropped_buffer_gpu[2] = {NULL,NULL};
    uchar* cropped_buffer_cpu[2] = {NULL,NULL};
    cudaAllocMapped( (void**) &cropped_buffer_cpu[0], (void**) &cropped_buffer_gpu[0], 150*150*3*sizeof(uchar) );
    cudaAllocMapped( (void**) &cropped_buffer_cpu[1], (void**) &cropped_buffer_gpu[1], 150*150*3*sizeof(uchar) );
    
    // calculate fps
    double fps = 0.0;
    clock_t clk;
    
    // Detection vars
    int num_dets = 0;
    std::vector<std::string> label_encodings;       // vector for the real names of the classes/persons

    // get the possible class names
    classifier.get_label_encoding(&label_encodings);
    
    
    

   if  ((TcpListenPort=OpenTcpListenPort(listen_port))==NULL)  // Open TCP Network port
     {
       printf("OpenTcpListenPortFailed\n");
       return(-1); 
     }

    
   clilen = sizeof(cli_addr);
    
   printf("Listening for connections\n");

   if  ((TcpConnectedPort=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen))==NULL)
     {  
       printf("AcceptTcpConnection Failed\n");
       return(-1); 
     }

   printf("Accepted connection Request\n");

// ------------------ "Detection" Loop -----------------------
    while(!user_quit){

        clk = clock();              // fps clock

	float* imgOrigin = NULL;    // camera image  

         cv::Mat   imgOriginMjpeg;

        // the 2nd arg 1000 defines timeout, true is for the "zeroCopy" param what means the image will be stored to shared memory    
   
        if (usecamera)
         {

           if( !camera->CaptureRGBA(&imgOrigin, 1000, true))                                   
			printf("failed to capture RGBA image from camera\n");
         }
        else
         {
   

            if (!LoadMotionJpegFrame(&MotionJpegFd, (float4**)&imgOrigin)) printf("Load Failed\n");
            FrameCount++;
            printf("FrameCount %d\n",FrameCount);

         }
   
        //since the captured image is located at shared memory, we also can access it from cpu n
        // here I define a cv::Mat for it to draw onto the image from CPU without copying data -- TODO: draw from CUDA
        if (usecamera) cudaRGBA32ToBGRA32(  (float4*)imgOrigin,  (float4*)imgOrigin, imgWidth, imgHeight); //ADDED DP
        cv::Mat origin_cpu(imgHeight, imgWidth, CV_32FC4, imgOrigin);
           

        // the mtcnn pipeline is based on GpuMat 8bit values 3 channels while the captured image is RGBA32
        // i use a kernel from jetson-inference to remove the A-channel and float to uint8
        cudaRGBA32ToRGB8( (float4*)imgOrigin, (uchar3*)rgb_gpu, imgWidth, imgHeight );      
        
        // create GpuMat form the same image thanks to shared memory
        cv::cuda::GpuMat imgRGB_gpu(imgHeight, imgWidth, CV_8UC3, rgb_gpu);                

        // pass the image to the MTCNN and get face detections
        std::vector<struct Bbox> detections;
        finder.findFace(imgRGB_gpu, &detections);

        // check if faces were detected, get face locations, bounding boxes and keypoints
        std::vector<cv::Rect> rects;
        std::vector<float*> keypoints;
        num_dets = get_detections(origin_cpu, &detections, &rects, &keypoints);               
        // if faces detected
        if(num_dets > 0){
            // crop and align the faces. Get faces to format for "dlib_face_recognition_model" to create embeddings
            std::vector<matrix<rgb_pixel>> faces;                                   
            crop_and_align_faces(imgRGB_gpu, cropped_buffer_gpu, cropped_buffer_cpu, &rects, &faces, &keypoints);
            
            // generate face embeddings from the cropped faces and store them in a vector
            std::vector<matrix<float,0,1>> face_embeddings;
            embedder.embeddings(&faces, &face_embeddings);                        
           
            // feed the embeddings to the pretrained SVM's. Store the predicted labels in a vector
            std::vector<double> face_labels;
            classifier.prediction(&face_embeddings, &face_labels);                 

            // draw bounding boxes and labels to the original image 
            draw_detections(origin_cpu, &rects, &face_labels, &label_encodings);    
        }
            char str[256];
            sprintf(str, "TensorRT  %.0f FPS", fps);               // print the FPS to the bar

            cv::putText(origin_cpu, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
            cv::putText(origin_cpu, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
        //Render captured image
        if (TcpSendImageAsJpeg(TcpConnectedPort,origin_cpu)<0)  break;

        // smooth FPS to make it readable
        fps = (0.90 * fps) + (0.1 * (1 / ((double)(clock()-clk)/CLOCKS_PER_SEC)));    
    }   

    SAFE_DELETE(camera);
    CHECK(cudaFreeHost(rgb_cpu));
    CHECK(cudaFreeHost(cropped_buffer_cpu[0]));
    CHECK(cudaFreeHost(cropped_buffer_cpu[1]));
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port
    return 0;
}


 

int main(int argc, char *argv[])
{

    int state = 0;

    state = camera_face_recognition( argc, argv );
        
   
    if(state == 1) cout << "Restart is required! Please type ./main again." << endl;
    
    return 0;
}
