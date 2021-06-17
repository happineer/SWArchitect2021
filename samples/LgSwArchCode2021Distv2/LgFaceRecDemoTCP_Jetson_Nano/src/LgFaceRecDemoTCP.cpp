#include <time.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <dlib/svm_threaded.h>
#include <dlib/svm.h>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/timerfd.h>

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

#define USE_MULTI_THREAD

static int config_face_detection = 1;
static int config_face_recognition = 1;
static bool usecamera = false;

static char *video_path;

enum TID_NAME {
	TID_CAPTURE = 1,
	TID_FACE_DETECT,
	TID_FACE_RECOGNIZE,
	TID_SENDER,
};

static pthread_t tids[4];

void *video_task_reader(void *args);
void *video_task_detector(void *args);
void *video_task_recognizer(void *args);
void *video_task_sender(void *args);


unsigned int FrameCount=0;
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

#define MJPEG_OUT_BUF_NR		4
typedef struct {
    ifstream    mpegfile;
    int         width;
    int         height;
    void        *inputImgGPU;
    void        *output[MJPEG_OUT_BUF_NR];
    imageFormat inputFormat;
    size_t      inputImageSize;

} TMotionJpegFileDesc;

struct task_info {
	int running;
};

static struct task_info g_task_info;
static gstCamera* g_camera = NULL;
static TMotionJpegFileDesc MotionJpegFd;
static int imgWidth;
static int imgHeight;

static int video_fps;
static int video_frames;


static int capture_sock[2];
static int sender_sock[2];


#define handle_error(msg) \
		do { perror(msg); exit(EXIT_FAILURE); } while (0)


int timerfd_disarm(int fd)
{
	struct itimerspec new_value;

	if (fd == -1)
		return -1;

	memset(&new_value, 0, sizeof(new_value));

	if (timerfd_settime(fd, 0, &new_value, NULL) == -1)
		handle_error("timerfd_settime disarm");

//	fprintf(stdout, "[%s] timerfd : %d\n", __func__, fd);

	return 0;
}

int timerfd_mod(int fd, int period_sec)
{
	struct itimerspec mod_val;
	struct timespec now;
	if (fd < 0)
		return -1;

	if (timerfd_disarm(fd) < 0)
		return -1;

	if (period_sec) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		mod_val.it_value.tv_sec = now.tv_sec + period_sec;
		mod_val.it_value.tv_nsec = now.tv_nsec;
		mod_val.it_interval.tv_sec = period_sec;
		mod_val.it_interval.tv_nsec = 0;

		if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &mod_val, NULL) < 0)
			return -1;
	}

	return 0;
}

int timerfd_open(int init_sec, int period_sec)
{
	int fd;
	struct itimerspec time_val;
	struct timespec now;

	fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
	if (fd < 0) {
		return -1;
	}

	if (init_sec || period_sec) {
		clock_gettime(CLOCK_MONOTONIC, &now);

		time_val.it_value.tv_sec = now.tv_sec + init_sec;
		time_val.it_value.tv_nsec = now.tv_nsec;
		time_val.it_interval.tv_sec = period_sec;
		time_val.it_interval.tv_nsec = 0;

		if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &time_val, NULL) < 0) {
			close(fd);
			return -1;
		}
	}

	return fd;
}



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
	memset(FileDesc->output, 0, sizeof(FileDesc->output));

    // allocate CUDA buffer for the image
    const size_t imgSize = (FileDesc->width * FileDesc->height*(sizeof(float4) * 8))/8;

    // convert from uint8 to float

    if( !cudaAllocMapped(&FileDesc->inputImgGPU, FileDesc->inputImageSize) )
    {
        printf("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", FileDesc->inputImageSize);
        return false;
    }

	for (int i = 0; i < MJPEG_OUT_BUF_NR; i++) {
	    if( !cudaAllocMapped(&FileDesc->output[i], imgSize) )
    	{
        	LogError(LOG_IMAGE "loadImage() -- failed to allocate %zu bytes for image \n", imgSize);
	        return false;
    	}
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
	for (int i = 0; i < MJPEG_OUT_BUF_NR; i++) {
	    CHECK(cudaFreeHost(FileDesc->output[i]))
	}

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool LoadMotionJpegFrame(TMotionJpegFileDesc *FileDesc, float4**  output)
{
    unsigned int imagesize;
    unsigned char* buff;
	static unsigned int out_idx;

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

    if( CUDA_FAILED(cudaConvertColor(FileDesc->inputImgGPU, FileDesc->inputFormat, FileDesc->output[out_idx], IMAGE_RGBA32F, FileDesc->width, FileDesc->height)) )
    {
        printf("LOG_IMAGE loadImage() -- failed to convert image \n");
        return false;

    }

    *output=(float4*)FileDesc->output[out_idx];
	out_idx = (out_idx + 1) % MJPEG_OUT_BUF_NR;

    FrameCount++;
    printf("FrameCount %d\n",FrameCount);

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

void compute_duration(struct timespec *specs, int count, int ndets)
{
	int i;
	int n = 0;
	int diff;
	char buf[256];

	n += sprintf(buf, "JHH %d %d", count, ndets);
	for (i = 0; i < count - 1; i++) {
		diff = (specs[i + 1].tv_sec - specs[i].tv_sec) * 1000000000ULL;
		diff += specs[i + 1].tv_nsec - specs[i].tv_nsec;
		diff /= 1000000;
		n += sprintf(buf + n, " %d", diff);
	}
	printf("%s\n", buf);
}

static void comm_socket_init(int *sock)
{
	int s[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) == 0) {
		sock[0] = s[0];
		sock[1] = s[1];

		if (fcntl(s[0], F_SETFD, FD_CLOEXEC) < 0) {
			fprintf(stderr, "[%s]: FD_CLOEXEC errno: %d\n", __func__, errno);
		}
		if (fcntl(s[0], F_SETFL, O_NONBLOCK) < 0) {
			fprintf(stderr, "[%s]: O_NONBLOCK errno: %d\n", __func__, errno);
		}
		if (fcntl(s[1], F_SETFD, FD_CLOEXEC) < 0) {
			fprintf(stderr, "[%s]: FD_CLOEXEC errno: %d\n", __func__, errno);
		}
		if (fcntl(s[1], F_SETFL, O_NONBLOCK) < 0) {
			fprintf(stderr, "[%s]: O_NONBLOCK errno: %d\n", __func__, errno);
		}
	} else {
		fprintf(stderr, "[%s]: socketpair() error!!\n", __func__);
	}
}

void *video_task_reader(void *args)
{
#ifdef USE_MULTI_THREAD
	struct pollfd fds[1];
    float* imgOrigin = NULL;    // camera image  

	memset(fds, 0, sizeof(fds));
	fds[0].fd = capture_sock[0];
	fds[0].events = POLLIN;

	if (!usecamera) {
		for (int i = 0; i < MJPEG_OUT_BUF_NR - 1; i++) {
        	LoadMotionJpegFrame(&MotionJpegFd, (float4**)&imgOrigin);
			int nwritten = write(capture_sock[0], &imgOrigin, sizeof(imgOrigin));
		}
	}

	int ret;
	while(1) {
		ret = poll(fds, 1, 1000);
		if (ret <= 0) {
			printf("[%s] ret = %d\n", __func__, ret);
			continue;
		}

		int tmp, nread;
		nread = read(capture_sock[0], &tmp, sizeof(tmp));
		assert(nread == sizeof(tmp));

		if (usecamera)
        {
            if( !g_camera->CaptureRGBA(&imgOrigin, 1000, true))                                   
                printf("failed to capture RGBA image from camera\n");
        }
        else
        {
            if (!LoadMotionJpegFrame(&MotionJpegFd, (float4**)&imgOrigin)) {
				printf("Load Failed\n");
				assert(0);
            }
			int nwritten = write(capture_sock[0], &imgOrigin, sizeof(imgOrigin));
			printf("[%s] write capture_sock[0] = %d\n", __func__, nwritten);
        }
	}
#endif
	return NULL;
}

void *video_task_detector(void *args)
{
	return NULL;
}

void *video_task_recognizer(void *args)
{
	return NULL;
}

struct send_msg {
	TTcpConnectedPort *TcpConnectedPort;
	cv::Mat *Image;
};

static void send_jpeg(void)
{
	int nread;
	struct send_msg msg;

	nread = read(sender_sock[1], &msg, sizeof(msg));
	assert(nread == sizeof(msg));
		
	//Render captured image
	if (TcpSendImageAsJpeg(msg.TcpConnectedPort, *msg.Image) < 0) {
		assert(0);
	}
	delete msg.Image;
	video_frames++;
}

static void handle_timer(int fd)
{
	static int old_frames = 0;
	uint64_t val[1];
	int idx;

	lseek(fd, 0, SEEK_SET);
	if (read(fd, &val, sizeof(uint64_t)) <= 0)
		return;

	video_fps = video_frames - old_frames;
	old_frames = video_frames;
	printf("[%s] video fps = %d\n", __func__, video_fps);
}

void *video_task_sender(void *args)
{
#ifdef USE_MULTI_THREAD
	struct pollfd fds[2];
	int timerfd;

	timerfd = timerfd_open(1, 1);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sender_sock[1];
	fds[0].events = POLLIN;

	fds[1].fd = timerfd;
	fds[1].events = POLLIN;

	int ret;

	while (1) {
		ret = poll(fds, 2, 1000);
		if (ret <= 0) {
			printf("[%s] ret = %d\n", __func__, ret);
			continue;
		}
	
		if (fds[0].revents & POLLIN) {
			send_jpeg();
		}

		if (fds[1].revents & POLLIN) {
			handle_timer(fds[1].fd);
		}
	}
	
#endif
	return NULL;
}


// perform face recognition with Raspberry Pi camera
int camera_face_recognition(int argc, char *argv[])
{
    TTcpListenPort    *TcpListenPort;
    TTcpConnectedPort *TcpConnectedPort;
    struct sockaddr_in cli_addr;
    socklen_t          clilen;
    short              listen_port;
	struct task_info *task = &g_task_info;


    // -------------- Initialization -------------------

    face_embedder embedder;                         // deserialize recognition network 
    face_classifier classifier(&embedder);          // train OR deserialize classification SVM's 
    if(classifier.need_restart() == 1) return 1;    // small workaround - if svms were trained theres some kind of memory problem when generate mtcnn

    bool user_quit = false;

    if (argc <2) 
    {
        fprintf(stderr,"usage %s [port] [filename]\n", argv[0]);
        exit(0);
    }

    listen_port =atoi(argv[1]);

    if (argc==2)
		usecamera = true;
	else
		video_path = argv[2];

    if (usecamera)
    {
        printf("video\n");

        g_camera = getCamera();                // create jetson camera - PiCamera. USB-Cam needs different operations in Loop!! not implemented!

        if( !g_camera )    
        {
            printf("load camera failed\n");
            return -1;
        }

        imgWidth = g_camera->GetWidth();
        imgHeight = g_camera->GetHeight();
    }
    else
    {
        if (!OpenMotionJpegFile(&MotionJpegFd, video_path, &imgWidth, &imgHeight))
        {
            printf("ERROR! Unable to open file %s\n",video_path);
            return -1;
        }
    }

#ifdef USE_MULTI_THREAD
	comm_socket_init(capture_sock);
	pthread_create(&tids[TID_CAPTURE], NULL, video_task_reader, NULL);

	comm_socket_init(sender_sock);
	pthread_create(&tids[TID_SENDER], NULL, video_task_sender, NULL);
#endif

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
    struct timespec tspecs[10];

#ifdef USE_MULTI_THREAD
	struct pollfd fds[1];
	memset(fds, 0, sizeof(fds));
	fds[0].fd = capture_sock[1];
	fds[0].events = POLLIN;
	int ret;
#endif

    while(!user_quit){
		int count = 0;
	    float* imgOrigin = NULL;    // camera image  

        clk = clock();              // fps clock
        clock_gettime(CLOCK_MONOTONIC, &tspecs[count++]);

#ifdef USE_MULTI_THREAD
		ret = poll(fds, 1, 1000);
		if (ret <= 0) {
			printf("[%s] ret = %d\n", __func__, ret);
			continue;
		}

		int nread = read(capture_sock[1], &imgOrigin, sizeof(imgOrigin));
		assert(nread == sizeof(imgOrigin));
		printf("[%s] read capture_sock[1] = %d\n", __func__, nread);
		write(capture_sock[1], &nread, sizeof(nread));
#endif
        //cv::Mat   imgOriginMjpeg;

        // the 2nd arg 1000 defines timeout, true is for the "zeroCopy" param what means the image will be stored to shared memory    

#ifndef USE_MULTI_THREAD
        if (usecamera)
        {
            if( !g_camera->CaptureRGBA(&imgOrigin, 1000, true))                                   
                printf("failed to capture RGBA image from camera\n");
        }
        else
        {
            if (!LoadMotionJpegFrame(&MotionJpegFd, (float4**)&imgOrigin))
				printf("Load Failed\n");
        }
#endif

		clock_gettime(CLOCK_MONOTONIC, &tspecs[count++]);
        //since the captured image is located at shared memory, we also can access it from cpu n
        // here I define a cv::Mat for it to draw onto the image from CPU without copying data -- TODO: draw from CUDA
        if (usecamera) cudaRGBA32ToBGRA32(  (float4*)imgOrigin,  (float4*)imgOrigin, imgWidth, imgHeight); //ADDED DP
        cv::Mat origin_cpu(imgHeight, imgWidth, CV_32FC4, imgOrigin);
        //cv::Mat *origin_cpu = new cv::Mat(imgHeight, imgWidth, CV_32FC4, imgOrigin);

        // the mtcnn pipeline is based on GpuMat 8bit values 3 channels while the captured image is RGBA32
        // i use a kernel from jetson-inference to remove the A-channel and float to uint8
        cudaRGBA32ToRGB8( (float4*)imgOrigin, (uchar3*)rgb_gpu, imgWidth, imgHeight );      

        // create GpuMat form the same image thanks to shared memory
        cv::cuda::GpuMat imgRGB_gpu(imgHeight, imgWidth, CV_8UC3, rgb_gpu);

        // pass the image to the MTCNN and get face detections
        std::vector<struct Bbox> detections;
		if (config_face_detection) {
        	finder.findFace(imgRGB_gpu, &detections);
		}
		clock_gettime(CLOCK_MONOTONIC, &tspecs[count++]);

        // check if faces were detected, get face locations, bounding boxes and keypoints
        std::vector<cv::Rect> rects;
        std::vector<float*> keypoints;
        num_dets = get_detections(origin_cpu, &detections, &rects, &keypoints);               
        // if faces detected
        if(num_dets > 0 && config_face_recognition){
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
		clock_gettime(CLOCK_MONOTONIC, &tspecs[count++]);
        char str[256];
        sprintf(str, "TensorRT  %.0f FPS", fps);               // print the FPS to the bar

#ifdef USE_MULTI_THREAD
		sprintf(str, "TensorRT  %d FPS", video_fps);
#endif

        cv::putText(origin_cpu, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
        cv::putText(origin_cpu, str, cv::Point(0, 20),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);

		 //Render captured image
#ifdef USE_MULTI_THREAD
		 struct send_msg msg;
		 cv::Mat *img = new cv::Mat;
		 *img = origin_cpu.clone();
		 //delete origin_cpu;

		 msg.TcpConnectedPort = TcpConnectedPort;
		 msg.Image = img;
		 write(sender_sock[0], &msg, sizeof(msg));
#else
        if (TcpSendImageAsJpeg(TcpConnectedPort, origin_cpu) < 0)  break;
		clock_gettime(CLOCK_MONOTONIC, &tspecs[count++]);
#endif

		compute_duration(tspecs, count, num_dets);

        // smooth FPS to make it readable
        fps = (0.90 * fps) + (0.1 * (1 / ((double)(clock()-clk)/CLOCKS_PER_SEC)));    
    }   

    SAFE_DELETE(g_camera);
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
