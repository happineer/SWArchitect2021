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


//#define DEBUG_PRINT_ON
#ifdef DEBUG_PRINT_ON
	#define DEBUG(fmt, args...)		fprintf(stdout, "[DEBUG]: " fmt, ## args)
#else
	#define DEBUG(fmt, args...)
#endif

static int config_face_detection = 1;
static int config_face_recognition = 1;
static bool usecamera = false;

static char *video_path;

enum TID_NAME {
	TID_CAPTURE = 0,
	TID_FACE_DETECT,

	TID_FACE_RECOGNIZE1,
	TID_FACE_RECOGNIZE2,
	TID_FACE_RECOGNIZE3,

	TID_SENDER,
	TID_NR,
};

static pthread_t tids[TID_NR];

void *video_task_reader(void *args);

unsigned int FrameCount=0;
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

#define MJPEG_OUT_BUF_NR		8
#define MJPEG_OUT_BUF_LOW		1
#define MJPEG_PRE_BUF_NR		2

static int buffer_count = MJPEG_OUT_BUF_NR;

struct video_buffer {
	void *output;
	cv::Mat *origin_cpu;
	cv::cuda::GpuMat *imgRGB_gpu;
	uchar* rgb_gpu;
    uchar* rgb_cpu;
    uchar* cropped_buffer_gpu[2];
    uchar* cropped_buffer_cpu[2];
	std::vector<struct Bbox> *detections;
	TTcpConnectedPort *TcpConnectedPort;

	int num_dets;
	std::vector<cv::Rect> *rects;
    std::vector<float*> *keypoints;
    std::vector<matrix<rgb_pixel>> *faces;                                   
    std::vector<matrix<float,0,1>> *face_embeddings;
    std::vector<double> *face_labels;
	unsigned int frame_number;
};

typedef struct {
    ifstream    mpegfile;
    int         width;
    int         height;
    void        *inputImgGPU;
    struct video_buffer buffer[MJPEG_OUT_BUF_NR];
    imageFormat inputFormat;
    size_t      inputImageSize;

} TMotionJpegFileDesc;

struct task_info {
	int tid;
	void* (*task_func)(void *);
	void (*do_work)(struct task_info *, struct video_buffer *);
	int running;
	mtcnn *finder;
	face_embedder *embedder;                         // deserialize recognition network 
    face_classifier *classifier;          // train OR deserialize classification SVM's 
    std::vector<std::string> *labels;
};

struct __attribute__((packed)) cmd_msg {
	__u32 type;
	__u32 value;
};


static gstCamera* g_camera = NULL;
static TMotionJpegFileDesc MotionJpegFd;
static int imgWidth;
static int imgHeight;

static int video_fps;
static int video_frames;


struct ipc {
	int sock[2];
};

struct ipc ipcs[TID_NR];

#define IPC_SEND	0
#define IPC_RECV	1

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
	memset(FileDesc->buffer, 0, sizeof(FileDesc->buffer));

    // allocate CUDA buffer for the image
    const size_t imgSize = (FileDesc->width * FileDesc->height*(sizeof(float4) * 8))/8;

    // convert from uint8 to float

    if( !cudaAllocMapped(&FileDesc->inputImgGPU, FileDesc->inputImageSize) )
    {
        printf("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", FileDesc->inputImageSize);
        return false;
    }

	for (int i = 0; i < MJPEG_OUT_BUF_NR; i++) {
		struct video_buffer *vp = &FileDesc->buffer[i];
	    if( !cudaAllocMapped(&vp->output, imgSize) )
    	{
        	LogError(LOG_IMAGE "loadImage() -- failed to allocate %zu bytes for image \n", imgSize);
	        return false;
    	}

        vp->origin_cpu = new cv::Mat(imgHeight, imgWidth, CV_32FC4, vp->output);

	    cudaAllocMapped( (void**) &vp->rgb_cpu, (void**) &vp->rgb_gpu, imgWidth*imgHeight*3*sizeof(uchar) );
	    cudaAllocMapped( (void**) &vp->cropped_buffer_cpu[0], (void**) &vp->cropped_buffer_gpu[0], 150*150*3*sizeof(uchar) );
    	cudaAllocMapped( (void**) &vp->cropped_buffer_cpu[1], (void**) &vp->cropped_buffer_gpu[1], 150*150*3*sizeof(uchar) );
  
        // create GpuMat form the same image thanks to shared memory
        vp->imgRGB_gpu = new cv::cuda::GpuMat(imgHeight, imgWidth, CV_8UC3, vp->rgb_gpu);

		vp->detections = new std::vector<struct Bbox>(10);

		vp->num_dets = 0;
		vp->rects = new std::vector<cv::Rect>(10);
		vp->keypoints = new std::vector<float*>(10);
		vp->faces = new std::vector<matrix<rgb_pixel>>(10);
		vp->face_embeddings = new std::vector<matrix<float,0,1>>(10);
		vp->face_labels = new std::vector<double>(10);
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
	    // TODO CHECK(cudaFreeHost(FileDesc->output[i]))
	}

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

static bool LoadMotionJpegFrame(TMotionJpegFileDesc *FileDesc, struct video_buffer **output)
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

	struct video_buffer *vp = &FileDesc->buffer[out_idx];

    if( CUDA_FAILED(cudaConvertColor(FileDesc->inputImgGPU, FileDesc->inputFormat, vp->output, IMAGE_RGBA32F, FileDesc->width, FileDesc->height)) )
    {
        printf("LOG_IMAGE loadImage() -- failed to convert image \n");
        return false;

    }

	cudaRGBA32ToRGB8((float4*)vp->output, (uchar3*)vp->rgb_gpu, imgWidth, imgHeight);  
    *output = vp;
    
	out_idx = (out_idx + 1) % MJPEG_OUT_BUF_NR;

	vp->frame_number = FrameCount;
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

	n += sprintf(buf, "DURATION %d %d", count, ndets);
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

void do_capture(struct task_info *task, struct video_buffer *buffer)
{
	static TTcpConnectedPort *TcpConnectedPort = NULL;
	float* imgOrigin = NULL;	// camera image 

	if (buffer->output == NULL) {
		printf("[%s]: start .....\n", __func__);
		TcpConnectedPort = buffer->TcpConnectedPort;
	}
	else {
		buffer_count++;
	}

	if (usecamera)
    {
        if( !g_camera->CaptureRGBA(&imgOrigin, 1000, true))
            printf("failed to capture RGBA image from camera\n");
    }
    else
    {
		while (buffer_count > MJPEG_OUT_BUF_LOW) {
            if (!LoadMotionJpegFrame(&MotionJpegFd, &buffer)) {
				printf("Load Failed JPEG.. Maybe EOF\n");
				assert(0);
			}
			buffer->TcpConnectedPort = TcpConnectedPort;
			int nwritten = write(ipcs[task->tid].sock[IPC_SEND], &buffer, sizeof(buffer));
			buffer_count--;
			DEBUG("[%s] write next ipc [0] = %d, buffer_count : %d\n", __func__, nwritten, buffer_count);
			usleep(1000);
		}
    }
}

void *video_task_reader(void *args)
{
	struct task_info *task = (struct task_info *)args;
	int tid_prev = TID_SENDER;
	int tid = TID_CAPTURE;

	struct pollfd fds[1];

	memset(fds, 0, sizeof(fds));
	fds[0].fd = ipcs[tid_prev].sock[IPC_RECV];
	fds[0].events = POLLIN;
#if 0
	if (!usecamera) {
		for (int i = 0; i < MJPEG_PRE_BUF_NR; i++) {
        	LoadMotionJpegFrame(&MotionJpegFd, &buffer);
			int nwritten = write(ipcs[tid].sock[IPC_SEND], &buffer, sizeof(buffer));
		}
	}
#endif

	int ret;
	while(1) {
		ret = poll(fds, 1, 5000);
		if (ret <= 0) {
			printf("[%s] ret = %d\n", __func__, ret);
			continue;
		}

		int nread;
		struct video_buffer *buffer;
		nread = read(fds[0].fd, &buffer, sizeof(buffer));
		assert(nread == sizeof(buffer));

		do_capture(task, buffer);
	}
	return NULL;
}

void do_face_detect(struct task_info *task, struct video_buffer *buffer)
{
	mtcnn *finder = task->finder;
	// pass the image to the MTCNN and get face detections
	buffer->detections->clear();
	if (config_face_detection) {
		finder->findFace(*buffer->imgRGB_gpu, buffer->detections);
	}
}

void *video_task_face_detect(void *args)
{
	struct task_info *task = (struct task_info *)args;
	int tid = task->tid;
	int tid_prev = (task->tid + TID_NR - 1) % TID_NR;

	struct pollfd fds[1];

	memset(fds, 0, sizeof(fds));
	fds[0].fd = ipcs[tid_prev].sock[IPC_RECV];
	fds[0].events = POLLIN;

	int ret;
	while(1) {
		ret = poll(fds, 1, 5000);
		if (ret <= 0) {
			printf("[%s] TID:%d PREV:%d ret = %d\n", __func__, tid, tid_prev, ret);
			continue;
		}

		int nread;
		struct video_buffer *buffer;
		nread = read(fds[0].fd, &buffer, sizeof(buffer));
		assert(nread == sizeof(buffer));

		if (task->do_work)
			task->do_work(task, buffer);
		
		int nwritten = write(ipcs[tid].sock[IPC_SEND], &buffer, sizeof(buffer));
		DEBUG("[%s] TID: %d, write next ipc fd = %d, buffer: %p\n", __func__, tid, ipcs[tid].sock[IPC_SEND], buffer);
	}
	return NULL;
}

void do_face_crop_and_align(struct task_info *task, struct video_buffer *buffer)
{
	buffer->num_dets = 0;
	buffer->rects->clear();
	buffer->keypoints->clear();
	buffer->faces->clear();
	buffer->face_embeddings->clear();
	buffer->face_labels->clear();

    buffer->num_dets = get_detections(*buffer->origin_cpu, buffer->detections, buffer->rects, buffer->keypoints);
	DEBUG("[%s]: Frame # : %d FACE # : %d\n", __func__, buffer->frame_number, buffer->num_dets);
	if (buffer->num_dets <= 0) {
		DEBUG("[%s]: Frame # : %d  NO FACE : %d\n", __func__, buffer->frame_number, buffer->num_dets);
		return;
	}

    // crop and align the faces. Get faces to format for "dlib_face_recognition_model" to create embeddings
    crop_and_align_faces(*buffer->imgRGB_gpu, buffer->cropped_buffer_gpu, buffer->cropped_buffer_cpu, buffer->rects, buffer->faces, buffer->keypoints);
}

void do_face_embede(struct task_info *task, struct video_buffer *buffer)
{
	if (buffer->num_dets <= 0) {
		DEBUG("[%s]: Frame # : %d  NO FACE : %d\n", __func__, buffer->frame_number, buffer->num_dets);
		return;
	}

	// generate face embeddings from the cropped faces and store them in a vector
    task->embedder->embeddings(buffer->faces, buffer->face_embeddings);
}

void do_face_predict(struct task_info *task, struct video_buffer *buffer)
{
	if (buffer->num_dets <= 0) {
		DEBUG("[%s]: Frame # : %d  NO FACE : %d\n", __func__, buffer->frame_number, buffer->num_dets);
		return;
	}

	// feed the embeddings to the pretrained SVM's. Store the predicted labels in a vector
	task->classifier->prediction(buffer->face_embeddings, buffer->face_labels);

    // draw bounding boxes and labels to the original image 
    draw_detections(*buffer->origin_cpu, buffer->rects, buffer->face_labels, task->labels);
}

void do_face_recognize(struct task_info *task, struct video_buffer *buffer)
{
	// crop and align the faces. Get faces to format for "dlib_face_recognition_model" to create embeddings
	do_face_crop_and_align(task, buffer);

	// generate face embeddings from the cropped faces and store them in a vector
	do_face_embede(task, buffer);

    // feed the embeddings to the pretrained SVM's. Store the predicted labels in a vector
    // draw bounding boxes and labels to the original image 
    do_face_predict(task, buffer);
}

static void do_send_video(struct task_info *task, struct video_buffer *buffer)
{
	int nread;
	char str[256];

	sprintf(str, "TensorRT  %d FPS", video_fps);
    cv::putText(*buffer->origin_cpu, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
    cv::putText(*buffer->origin_cpu, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
		
	//Render captured image
	if (TcpSendImageAsJpeg(buffer->TcpConnectedPort, buffer->origin_cpu) < 0) {
		assert(0);
	}
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

static void handle_client_msg(TTcpConnectedPort *tcpConnectedPort)
{
	struct cmd_msg msg;
	int ret;

	ret = ReadDataTcp(tcpConnectedPort, (unsigned char *)&msg, sizeof(msg));
	assert(ret == sizeof(msg));

	printf("[%s] client msg type : 0x%08X, value : 0x%08X\n", __func__, msg.type, msg.value);
}

static struct task_info g_task_info[TID_NR] = {
	{ TID_CAPTURE,         video_task_reader,      /*do_capture*/ },
  	{ TID_FACE_DETECT,     video_task_face_detect, do_face_detect, },
	{ TID_FACE_RECOGNIZE1, video_task_face_detect, do_face_crop_and_align, },
	{ TID_FACE_RECOGNIZE2, video_task_face_detect, do_face_embede, },
	{ TID_FACE_RECOGNIZE3, video_task_face_detect, do_face_predict, },
	{ TID_SENDER,          video_task_face_detect, do_send_video, }
};

// perform face recognition with Raspberry Pi camera
int camera_face_recognition(int argc, char *argv[])
{
    TTcpListenPort    *TcpListenPort;
    TTcpConnectedPort *TcpConnectedPort;
    struct sockaddr_in cli_addr;
    socklen_t          clilen;
    short              listen_port;

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

    mtcnn finder(imgHeight, imgWidth);              // build OR deserialize TensorRT detection network

    // malloc shared memory for images for access with cpu and gpu without copying data
    // cudaAllocMapped is used from jetson-inference
#if 0
    uchar* rgb_gpu = NULL;
    uchar* rgb_cpu = NULL;
    cudaAllocMapped( (void**) &rgb_cpu, (void**) &rgb_gpu, imgWidth*imgHeight*3*sizeof(uchar) );
    uchar* cropped_buffer_gpu[2] = {NULL,NULL};
    uchar* cropped_buffer_cpu[2] = {NULL,NULL};
    cudaAllocMapped( (void**) &cropped_buffer_cpu[0], (void**) &cropped_buffer_gpu[0], 150*150*3*sizeof(uchar) );
    cudaAllocMapped( (void**) &cropped_buffer_cpu[1], (void**) &cropped_buffer_gpu[1], 150*150*3*sizeof(uchar) );
#endif

    // calculate fps
    double fps = 0.0;
    clock_t clk;

    // Detection vars
    std::vector<std::string> label_encodings;       // vector for the real names of the classes/persons

    // get the possible class names
    classifier.get_label_encoding(&label_encodings);

	for (int i = 0; i < TID_NR; i++) {
		comm_socket_init(ipcs[i].sock);
	}

	for (int i = 0; i < TID_NR; i++) {
		struct task_info *task = &g_task_info[i];
		task->classifier = &classifier;
		task->embedder = &embedder;
		task->finder = &finder;
		task->labels = &label_encodings;

		pthread_create(&tids[i], NULL, task->task_func, task);
	}

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

	struct video_buffer *buffer;
	buffer = (struct video_buffer *)malloc(sizeof(struct video_buffer));
	memset(buffer, 0, sizeof(buffer));
	buffer->TcpConnectedPort = TcpConnectedPort;
	write(ipcs[TID_SENDER].sock[IPC_SEND], &buffer, sizeof(buffer));
	printf("Triger reading fd: %d\n", ipcs[TID_SENDER].sock[IPC_SEND]);
	
	struct pollfd fds[2];
	int timerfd;
	int ret;
	
	timerfd = timerfd_open(1, 1);
	
	memset(fds, 0, sizeof(fds));
	fds[0].fd = timerfd;
	fds[0].events = POLLIN;

	fds[1].fd = TcpConnectedPort->ConnectedFd;
	fds[1].events = POLLIN;

    while(!user_quit) {
		ret = poll(fds, 2, 5000);
		if (ret <= 0) {
			DEBUG("[%s] ret = %d\n", __func__, ret);
			continue;
		}

		if (fds[0].revents & POLLIN) {
			handle_timer(fds[0].fd);
		}

		if (fds[1].revents & POLLIN) {
			handle_client_msg(TcpConnectedPort);
		}
		
        //fps = (0.90 * fps) + (0.1 * (1 / ((double)(clock()-clk)/CLOCKS_PER_SEC)));    
    }   

    SAFE_DELETE(g_camera);
    //TODO : CHECK(cudaFreeHost(rgb_cpu));
    //TODO : CHECK(cudaFreeHost(cropped_buffer_cpu[0]));
    //TODO : CHECK(cudaFreeHost(cropped_buffer_cpu[1]));
    CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
    CloseTcpListenPort(&TcpListenPort);  // Close listen port
    return 0;
}



static int set_rt_policy(int pid)
{
	struct sched_param sparam;
    int policy = SCHED_RR/*SCHED_FIFO*/;

	nice(-20);
	sparam.sched_priority = 50;
	if (sched_setscheduler(pid, policy, &sparam)) {
		perror("setscheduler");
		return -1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
    int state = 0;

	set_rt_policy(getpid());

    state = camera_face_recognition( argc, argv );

    if(state == 1) cout << "Restart is required! Please type ./main again." << endl;

    return 0;
}