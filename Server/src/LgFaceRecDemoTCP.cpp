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
#include <sys/epoll.h>
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

#include <sys/wait.h>
#include <sys/types.h>


//#define DEBUG_PRINT_ON
#ifdef DEBUG_PRINT_ON
	#define DEBUG(fmt, args...)		fprintf(stdout, "[DEBUG]: " fmt, ## args)
#else
	#define DEBUG(fmt, args...)
#endif


#define VIDEO_WIDTH			960
#define VIDEO_HEIGHT		540

static int config_face_detection = 1;
static int config_face_recognition = 1;
static bool usecamera = true;
static int face_detect_max = 5;
static int face_detect_acc_test_mode;
static int face_detect_measure;
static int face_detect_autostart;
static int face_detect_send_timestamp = 0;

enum INPUT_MODE {
	INPUT_CAMERA = 0,
	INPUT_FILE = 1,
};

static int input_mode = INPUT_CAMERA;

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

#define	TIME_STAMP_NR			14

struct __attribute__((packed)) cmd_msg {
	__u32 type;
	__u32 value;
};


enum CMD_TYPE {
	CMD_TYPE_NORMAL	 	= 0x1,
};

enum CMD_VALUE {
	CMD_PLAY_STOP		= 0x0,
	CMD_RUN_MODE 		= 0x1,
	CMD_TEST_RUN_MODE	= 0x2,
	CMD_TEST_ACC		= 0x3,
};

struct video_buffer {
	void *output;
	struct cmd_msg msg;
	cv::Mat *origin_cpu;
	cv::cuda::GpuMat *imgRGB_gpu;
	uchar* rgb_gpu;
    uchar* rgb_cpu;
    uchar* cropped_buffer_gpu[2];
    uchar* cropped_buffer_cpu[2];
	std::vector<struct Bbox> *detections;

	int num_dets;
	std::vector<cv::Rect> *rects;
    std::vector<float*> *keypoints;
    std::vector<matrix<rgb_pixel>> *faces;                                   
    std::vector<matrix<float,0,1>> *face_embeddings;
    std::vector<double> *face_labels;
	unsigned int frame_number;
	long long timestamp;

	int ntimes;
	struct timespec times[TIME_STAMP_NR];
};

static const int MJPEG_FILE_BUF_SIZE = (512 * 1024);
typedef struct {
    ifstream    mpegfile;
    int         width;
    int         height;
    void        *inputImgGPU;
    imageFormat inputFormat;
    size_t      inputImageSize;
    int         filesize;
	int         pos;
	int         buf_size;
	unsigned char *buff;
} TMotionJpegFileDesc;

typedef void (*work_func)(struct task_info *, struct video_buffer *);
#define WORK_FUNC_NR		3

struct task_info {
	int tid;
	void* (*task_func)(void *);
	work_func do_work[WORK_FUNC_NR];
	int running;
	mtcnn *finder;
	face_embedder *embedder;                         // deserialize recognition network 
    face_classifier *classifier;          // train OR deserialize classification SVM's 
    std::vector<std::string> *labels;
};


static gstCamera* g_camera = NULL;
static TMotionJpegFileDesc MotionJpegFd;
static int imgWidth;
static int imgHeight;

static int video_fps;
static int video_frames;

// calculate fps
static double send_fps = 0.0;
static clock_t pre_clk;


static struct video_buffer ring_buffer[MJPEG_OUT_BUF_NR];
static size_t buf_img_size;
static int ring_index;

static TTcpConnectedPort gTcpConnectedPort = -1;


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


static inline void video_buffer_init(struct video_buffer *buffer)
{
	buffer->ntimes = 0;
	buffer->num_dets = 0;
	buffer->rects->clear();
	buffer->keypoints->clear();
	buffer->faces->clear();
	buffer->face_embeddings->clear();
	buffer->face_labels->clear();
}

static inline void video_buffer_mark_time(struct video_buffer *buffer)
{
	clock_gettime(CLOCK_MONOTONIC, &buffer->times[buffer->ntimes]);
	buffer->ntimes++;
}

static inline void video_buffer_set_timestamp(struct video_buffer *buffer)
{
	struct timespec ts;
	long long timestamp;

	clock_gettime(CLOCK_REALTIME, &ts);
	timestamp = ts.tv_sec * 1000ULL;
	timestamp += ts.tv_nsec / 1000000ULL;

	buffer->timestamp = htobe64(timestamp);
}


static bool video_buffer_alloc(struct video_buffer *vbs, size_t imgSize)
{
	for (int i = 0; i < MJPEG_OUT_BUF_NR; i++) {
		struct video_buffer *vp = &vbs[i];
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
	return true;
}


static inline struct video_buffer *video_buffer_get_next(void)
{
	struct video_buffer *vp = &ring_buffer[ring_index];
	ring_index++;
	ring_index = (ring_index + 1) % MJPEG_OUT_BUF_NR;

	return vp;
}

static inline struct video_buffer *video_buffer_get_current(void)
{
	struct video_buffer *vp = &ring_buffer[ring_index];
	return vp;
}

static inline void video_buffer_next(void)
{
	ring_index = (ring_index + 1) % MJPEG_OUT_BUF_NR;
}


/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
static bool RewindMotionJpegFile(TMotionJpegFileDesc *FileDesc)
{
    if (!FileDesc)
		return false;

	FrameCount = 0;
	face_detect_acc_test_mode = 0;
    FileDesc->mpegfile.seekg(8, FileDesc->mpegfile.beg);
	FileDesc->pos = 8;

	cout << "FileDesc->mpegfile.tellg() : " << FileDesc->mpegfile.tellg() << endl;

    return true;
}


static bool OpenMotionJpegFile(TMotionJpegFileDesc *FileDesc,char * Filename, int *Width, int *Height)
{
    if ((!FileDesc) || (!Width) ||(!Height))  return false;

    FileDesc->mpegfile.open(Filename, ios::in | ios::binary);
    if (!FileDesc->mpegfile.is_open())
    {

        return false;
    }

  // get length of file:
  	FileDesc->pos = 0;
    FileDesc->mpegfile.seekg (0, FileDesc->mpegfile.end);
    FileDesc->filesize = FileDesc->mpegfile.tellg();
    FileDesc->mpegfile.seekg (0, FileDesc->mpegfile.beg);

    FileDesc->mpegfile.read((char*)&FileDesc->width, sizeof(FileDesc->width));
    if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->width)) 
    {
        FileDesc->mpegfile.close();
        return(false);
    }
	FileDesc->pos += sizeof(FileDesc->width);

    FileDesc->mpegfile.read((char*)&FileDesc->height, sizeof(FileDesc->height));
    if (FileDesc->mpegfile.gcount() != sizeof(FileDesc->height)) 
    {
        FileDesc->mpegfile.close();
        return(false);
    }
	FileDesc->pos += sizeof(FileDesc->height);

	FileDesc->buf_size = MJPEG_FILE_BUF_SIZE;
	FileDesc->buff = (unsigned char *)malloc(FileDesc->buf_size);

    FileDesc->width=ntohl(FileDesc->width);
    FileDesc->height=ntohl(FileDesc->height);
    *Height=FileDesc->height;
    *Width=FileDesc->width;
    FileDesc->inputFormat = IMAGE_RGB8;
    FileDesc->inputImageSize = (FileDesc->width * FileDesc->height*(sizeof(uchar3) * 8))/8;
    FileDesc->inputImgGPU = NULL;


    // allocate CUDA buffer for the image
	//buf_img_size = (FileDesc->width * FileDesc->height*(sizeof(float4) * 8))/8;
	//printf("[%s]: buf_img_size = %ld\n", __func__, buf_img_size);

    // convert from uint8 to float

    if( !cudaAllocMapped(&FileDesc->inputImgGPU, FileDesc->inputImageSize) )
    {
        printf("LOG_IMAGE loadImage() -- failed to allocate %zu bytes for image \n", FileDesc->inputImageSize);
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

	if (FileDesc->buff) {
		free(FileDesc->buff);
	}
    CUDA(cudaFreeHost(FileDesc->inputImgGPU));
	for (int i = 0; i < MJPEG_OUT_BUF_NR; i++) {
	    // TODO CHECK(cudaFreeHost(FileDesc->output[i]))
	}

}
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
static inline bool is_acc_test_frame(int frame)
{
#define ACC_FRAME_NR		25
	const static int AccTestFrames[ACC_FRAME_NR] = {
			153, 201, 221, 339, 504, 
            700, 910, 1007, 1173, 1279,
            1355, 1397, 1408, 1470, 1606,
            1687, 1711, 1878, 1895, 1969,
            2256, 2440, 2747, 2966, 3033
	};
	static int acc_idx;

	if (frame < AccTestFrames[acc_idx])
		return false;

	if (frame == AccTestFrames[acc_idx]) {
		printf("[%s] FOUND frame : %d, accFrame : %d acc_idx : %d\n",
			__func__, frame, AccTestFrames[acc_idx], acc_idx);
		acc_idx = (acc_idx + 1) % ACC_FRAME_NR;
		if (acc_idx == 0) {
			printf("[%s] AccTestFrames END frame : %d, acc_idx : %d\n",
				__func__, frame, acc_idx);
		}
		
		return true;
	}

	printf("[%s] ERROR frame : %d, accFrame : %d acc_idx : %d\n",
		__func__, frame, AccTestFrames[acc_idx], acc_idx);

	return false;
}

static inline bool ReadMotionJpeg(TMotionJpegFileDesc *FileDesc, unsigned char *buf, unsigned int &imagesize)
{
	static unsigned int max_imagesize;

	if (FileDesc->pos >= (FileDesc->filesize - 4)) {
		printf("[%s] FILE EOF, FileSize : %d Pos: %d\n", 
			__func__, FileDesc->filesize, FileDesc->pos);
		return false;
	}

	FileDesc->mpegfile.read((char*)&imagesize, sizeof(imagesize));
    if (FileDesc->mpegfile.gcount() != sizeof(imagesize)) return(0);
	FileDesc->pos += sizeof(imagesize);
    imagesize = ntohl(imagesize);
	if (imagesize > max_imagesize) {
		max_imagesize = imagesize;
		printf("LoadMJpegFrame() max_imagesize = %d (%d KB)\n", max_imagesize, max_imagesize / 1024);
	}
    FileDesc->mpegfile.read((char*)buf, imagesize);
    if (FileDesc->mpegfile.gcount() != imagesize)
    {
        return false;
    }
	FileDesc->pos += imagesize;
	
    FrameCount++;

	return true;
}

static bool LoadMotionJpegFrame(TMotionJpegFileDesc *FileDesc, struct video_buffer *&vp)
{
    unsigned int imagesize;

    // validate parameters
    if( !FileDesc)
    {
        printf("LOG_IMAGE LoadMJpegFrame() - invalid parameter(s)\n");
        return false;
    }

	do {
		if (!ReadMotionJpeg(FileDesc, FileDesc->buff, imagesize)) {
			return false;
		}
	} while (face_detect_acc_test_mode && !is_acc_test_frame(FrameCount));

    cv::Mat img;
    cv::imdecode(cv::Mat(imagesize, 1, CV_8UC1, FileDesc->buff), cv::IMREAD_COLOR, &img);
    if (img.empty()) 
    {
        printf("cv::imdecode failed\n");
        return(false);
    }

    memcpy(FileDesc->inputImgGPU, img.data, imageFormatSize(FileDesc->inputFormat, FileDesc->width, FileDesc->height));

    if( CUDA_FAILED(cudaConvertColor(FileDesc->inputImgGPU, FileDesc->inputFormat, vp->output, IMAGE_RGBA32F, FileDesc->width, FileDesc->height)) )
    {
        printf("LOG_IMAGE loadImage() -- failed to convert image \n");
        return false;

    }

	cudaRGBA32ToRGB8((float4*)vp->output, (uchar3*)vp->rgb_gpu, imgWidth, imgHeight);  

	vp->frame_number = FrameCount;
    printf("FrameCount %d\n",FrameCount);

	if (FileDesc->pos >= (FileDesc->filesize - 4)) {
		printf("[%s] FILE EOF, FrameCount : %d rewinding... \n", __func__, FrameCount);
		RewindMotionJpegFile(FileDesc);
	}

    return true;

}
/***********************************************************************************************/
/***********************************************************************************************/


// create high performace jetson camera - loads the image directly to gpu memory or shared memory
gstCamera* getCamera(int width, int height){
    gstCamera* camera = gstCamera::Create(width/*gstCamera::DefaultWidth*/, height/*gstCamera::DefaultHeight*/, NULL);
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

static void compute_duration(struct timespec *specs, int count, int ndets)
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

static void compute_duration(struct video_buffer *buffer)
{
	int i;
	int n = 0;
	int diff;
	char buf[256];

	n += sprintf(buf, "DURATION %d %d %d %d", buffer->ntimes, buffer->frame_number, video_fps, buffer->num_dets);
	for (i = 0; i < buffer->ntimes - 1; i++) {
		diff = (buffer->times[i + 1].tv_sec - buffer->times[i].tv_sec) * 1000000000ULL;
		diff += buffer->times[i + 1].tv_nsec - buffer->times[i].tv_nsec;
		diff /= 1000000;
		n += sprintf(buf + n, " %d", diff);
	}
	n += sprintf(buf + n, " %ld.%09ld", buffer->times[buffer->ntimes - 1].tv_sec, 
		buffer->times[buffer->ntimes - 1].tv_nsec);
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


static void do_send_cmd_msg(struct video_buffer *buffer)
{
	int ret;
	ret = write(ipcs[TID_SENDER].sock[IPC_SEND], &buffer, sizeof(buffer));
	printf("[%s] fd: %d, ret = %d\n", __func__, ipcs[TID_SENDER].sock[IPC_SEND], ret);
}

static void send_cmd_msg(struct cmd_msg *msg)
{
	struct video_buffer *buffer;

	buffer = (struct video_buffer *)malloc(sizeof(struct video_buffer));
	memset(buffer, 0, sizeof(struct video_buffer));
	memcpy(&buffer->msg, msg, sizeof(struct cmd_msg));

	printf("[%s] cmd_msg: 0x%08X 0x%08X\n", __func__, buffer->msg.type, buffer->msg.value);	
	do_send_cmd_msg(buffer);
}

static void recv_cmd_msg(struct video_buffer *buffer, struct cmd_msg *msg)
{
	memcpy(msg, &buffer->msg, sizeof(struct cmd_msg));

	printf("[%s] cmd_msg: 0x%08X 0x%08X\n", __func__, msg->type, msg->value);	
	
	free(buffer);
}


static void handle_cmd_msg(struct cmd_msg *msg)
{
	if (msg->type != 1) {
		assert(msg->type == 1);
		return;
	}

	switch (msg->value)
	{
	case CMD_PLAY_STOP:
		printf("[%s]: CMD_PLAY_STOP\n", __func__);
		break;

	case CMD_RUN_MODE:
		printf("[%s]: CMD_RUN_MODE\n", __func__);
		input_mode = INPUT_CAMERA;
		printf("INPUT MODE : %s\n", (input_mode == INPUT_CAMERA) ? "INPUT_CAMERA" : "INPUT_FILE");
		break;

	case CMD_TEST_RUN_MODE:
		printf("[%s]: CMD_TEST_RUN_MODE\n", __func__);
		RewindMotionJpegFile(&MotionJpegFd);
		input_mode = INPUT_FILE;
		printf("INPUT MODE : %s\n", (input_mode == INPUT_CAMERA) ? "INPUT_CAMERA" : "INPUT_FILE");
		break;

	case CMD_TEST_ACC:
		printf("[%s]: CMD_TEST_ACC\n", __func__);
		input_mode = INPUT_FILE;
		printf("INPUT MODE : %s\n", (input_mode == INPUT_CAMERA) ? "INPUT_CAMERA" : "INPUT_FILE");
		RewindMotionJpegFile(&MotionJpegFd);
		face_detect_acc_test_mode = 1;
		break;

	default:
		break;
	}
}

bool do_capture_camera(struct task_info *task, struct video_buffer *&vp)
{
	float* imgOrigin = NULL;	// camera image

	if (!g_camera->CaptureRGBA(&imgOrigin, 1000, true)) {
		printf("failed to capture RGBA image from camera\n");
		assert(0);
		return false;
	}

	//printf("[%s]: imgOrigin : %p\n", __func__, imgOrigin);

	//memcpy(vp->output, imgOrigin, buf_img_size);
	cudaRGBA32ToBGRA32((float4*)imgOrigin, (float4*)vp->output, imgWidth, imgHeight); //ADDED DP
	//cv::Mat origin_cpu(imgHeight, imgWidth, CV_32FC4, imgOrigin);

	cudaRGBA32ToRGB8((float4*)vp->output, (uchar3*)vp->rgb_gpu, imgWidth, imgHeight);
	return true;
}

void do_capture(struct task_info *task, struct video_buffer *buffer)
{
	if (buffer->output == NULL) {
		printf("[%s]: recv cmd\n", __func__);
		struct cmd_msg msg;
		recv_cmd_msg(buffer, &msg);
		handle_cmd_msg(&msg);
	}
	else {
		if (face_detect_measure)
			compute_duration(buffer);
		buffer_count++;
	}

	if (gTcpConnectedPort == -1)
		return;

	assert(imgWidth);
	assert(imgHeight);

	if (input_mode == INPUT_CAMERA)
    {
		while (buffer_count > MJPEG_OUT_BUF_LOW) {
	       	struct video_buffer *vp = video_buffer_get_current();
			video_buffer_init(vp);
			video_buffer_mark_time(vp);

			if (!do_capture_camera(task, vp)) {
				printf("Failed to get Camera image...\n");
				break;
			}
			video_buffer_set_timestamp(vp);
			video_buffer_mark_time(vp);
			video_buffer_next();
			int nwritten = write(ipcs[task->tid].sock[IPC_SEND], &vp, sizeof(vp));
			buffer_count--;
			DEBUG("[%s] write next ipc [0] = %d, buffer_count : %d\n", __func__, nwritten, buffer_count);
			usleep(10 * 1000);
		}
    }
    else
    {
		while (buffer_count > MJPEG_OUT_BUF_LOW) {
			struct video_buffer *vp = video_buffer_get_current();
			video_buffer_init(vp);
			video_buffer_mark_time(vp);
            if (!LoadMotionJpegFrame(&MotionJpegFd, vp)) {
				printf("Load Failed JPEG.. Maybe EOF\n");
				break;
				//assert(0);
			}
			video_buffer_set_timestamp(vp);
			video_buffer_mark_time(vp);
			video_buffer_next();
			int nwritten = write(ipcs[task->tid].sock[IPC_SEND], &vp, sizeof(vp));

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
		if (ret == 0) {
			DEBUG("[%s] TID:%d PREV:%d ret = %d\n", __func__, tid, tid_prev, ret);
			continue;
		}
		if (ret < 0) {
			fprintf(stderr, "[%s] TID:%d PREV:%d ret = %d, errno: %d %s\n",
				__func__, tid, tid_prev, ret, errno, strerror(errno));
			assert(0);
			usleep(1000 * 1000);
			continue;
		}

		int nread;
		struct video_buffer *buffer;
		nread = read(fds[0].fd, &buffer, sizeof(buffer));
		assert(nread == sizeof(buffer));

		video_buffer_mark_time(buffer);
		for (int i = 0; i < WORK_FUNC_NR; i++) {
			if (task->do_work[i])
				task->do_work[i](task, buffer);
			else
				break;
		}
		video_buffer_mark_time(buffer);
		
		int nwritten = write(ipcs[tid].sock[IPC_SEND], &buffer, sizeof(buffer));
		DEBUG("[%s] TID: %d, write next ipc fd = %d, buffer: %p\n", __func__, tid, ipcs[tid].sock[IPC_SEND], buffer);
	}
	return NULL;
}

void do_face_crop_and_align(struct task_info *task, struct video_buffer *buffer)
{
    buffer->num_dets = get_detections(*buffer->origin_cpu, buffer->detections, buffer->rects, buffer->keypoints);
	DEBUG("[%s]: Frame # : %d FACE # : %d\n", __func__, buffer->frame_number, buffer->num_dets);
	if (buffer->num_dets <= 0 || buffer->num_dets > face_detect_max) {
		DEBUG("[%s]: Frame # : %d  NO FACE : %d\n", __func__, buffer->frame_number, buffer->num_dets);
		return;
	}

    // crop and align the faces. Get faces to format for "dlib_face_recognition_model" to create embeddings
    crop_and_align_faces(*buffer->imgRGB_gpu, buffer->cropped_buffer_gpu, buffer->cropped_buffer_cpu, buffer->rects, buffer->faces, buffer->keypoints);
}

void do_face_embede(struct task_info *task, struct video_buffer *buffer)
{
	if (buffer->num_dets <= 0 || buffer->num_dets > face_detect_max) {
		DEBUG("[%s]: Frame # : %d  NO FACE : %d\n", __func__, buffer->frame_number, buffer->num_dets);
		return;
	}

	// generate face embeddings from the cropped faces and store them in a vector
    task->embedder->embeddings(buffer->faces, buffer->face_embeddings);
}

void do_face_predict(struct task_info *task, struct video_buffer *buffer)
{
	if (buffer->num_dets <= 0 || buffer->num_dets > face_detect_max) {
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

static void diconnect_client(void)
{
	printf("[%s] client connection error : %d : %s \n", __func__, errno, strerror(errno));
	CloseTcpConnectedPort(gTcpConnectedPort);
	gTcpConnectedPort = -1;
	return;
}

static void do_draw_fps(struct task_info *task, struct video_buffer *buffer)
{
	char str[256];
	int n = 0;

	n += sprintf(str + n, "TensorRT FPS: %d", video_fps);
#if 0
	n += sprintf(str + n, " %.2f", send_fps);
#endif
    cv::putText(*buffer->origin_cpu, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(255, 255, 255, 255), 3); // mat, text, coord, font, scale, bgr color, line thickness
    cv::putText(*buffer->origin_cpu, str, cv::Point(0, 20),
            cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 0, 255), 1);
	
}

static void do_send_video(struct task_info *task, struct video_buffer *buffer)
{
	int nread;
	clock_t clk;

	if (gTcpConnectedPort == -1)
		return;

	//Render captured image
	if (face_detect_send_timestamp) {
		if (TcpSendImageAsJpeg(gTcpConnectedPort, buffer->origin_cpu, buffer->timestamp) < 0) {
			diconnect_client();
			return;
			assert(0);
		}
	} else {
		if (TcpSendImageAsJpeg(gTcpConnectedPort, buffer->origin_cpu) < 0) {
			diconnect_client();
			return;
			assert(0);
		}
	}
	clk = clock();
	send_fps = (0.90 * send_fps) + (0.1 * (1 / ((double)(clk - pre_clk) / CLOCKS_PER_SEC)));
	pre_clk = clk;
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
	printf("[%s] video fps = %d  buffer_count = %d\n", __func__, video_fps, buffer_count);
}

static void handle_client_msg(int epollfd, TTcpConnectedPort tcpConnectedPort)
{
	struct cmd_msg msg;
	int ret;

	ret = ReadDataTcp(tcpConnectedPort, (unsigned char *)&msg, sizeof(msg));
	if (ret != sizeof(msg)) {
		if (epoll_ctl(epollfd, EPOLL_CTL_DEL, tcpConnectedPort, NULL) == -1) {
			perror("epoll_ctl: EPOLL_CTL_DEL conn_sock");
			exit(EXIT_FAILURE);
		}
		printf("[%s] epoll_ctl: EPOLL_CTL_DEL conn_sock\n", __func__);
		diconnect_client();
		return;
	}
	assert(ret == sizeof(msg));

	printf("[%s] client msg type : 0x%08X, value : 0x%08X\n", __func__, msg.type, msg.value);
	send_cmd_msg(&msg);
}

static struct task_info g_task_info[TID_NR] = {
	{ TID_CAPTURE,         video_task_reader,      { 0, /*do_capture*/ }, },
  	{ TID_FACE_DETECT,     video_task_face_detect, { do_face_detect,},  },
	{ TID_FACE_RECOGNIZE1, video_task_face_detect, { do_face_crop_and_align, }, },
	{ TID_FACE_RECOGNIZE2, video_task_face_detect, { do_face_embede }, },
	{ TID_FACE_RECOGNIZE3, video_task_face_detect, { do_face_predict, do_draw_fps, }, },
	{ TID_SENDER,          video_task_face_detect, { do_send_video, }, }
};



void start_video_stream(TTcpConnectedPort tcpConnectedPort)
{
	int ret;

	struct cmd_msg msg;
	msg.type = CMD_TYPE_NORMAL;
	msg.value = CMD_TEST_RUN_MODE;
	
	send_cmd_msg(&msg);
	printf("[%s] Triger reading\n", __func__);
}

// perform face recognition with Raspberry Pi camera
int camera_face_recognition(int argc, char *argv[])
{
    TTcpListenPort    *TcpListenPort;
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

   	if (argc == 3)
		video_path = argv[2];

	if (video_path) {
        if (!OpenMotionJpegFile(&MotionJpegFd, video_path, &imgWidth, &imgHeight))
        {
            printf("ERROR! Unable to open file %s\n",video_path);
            return -1;
        }
	}

	int width = VIDEO_WIDTH, height = VIDEO_HEIGHT;
    if (usecamera)
    {
        printf("camera\n");
		if (imgWidth) {
			width = imgWidth;
			height = imgHeight;
		}
        g_camera = getCamera(width, height);  // create jetson camera - PiCamera. USB-Cam needs different operations in Loop!! not implemented!

        if( !g_camera )    
        {
            printf("load camera failed\n");
            return -1;
        }
        width = g_camera->GetWidth();
        height = g_camera->GetHeight();
    }

	printf("[%s]: Video  Width = %d Height = %d\n", __func__, imgWidth, imgHeight);
	printf("[%s]: Camera Width = %d Height = %d\n", __func__, width, height);
	if (video_path && usecamera) {
		assert(imgWidth == width);
		assert(imgHeight == height);
	}
	if (usecamera) {
		imgWidth = width;
		imgHeight = height;
	}

	// allocate CUDA buffer for the image
	buf_img_size = (imgWidth * imgHeight * (sizeof(float4) * 8)) / 8;
	printf("[%s]: buf_img_size = %ld\n", __func__, buf_img_size);

	assert(buf_img_size);

	video_buffer_alloc(ring_buffer, buf_img_size);

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

#define MAX_EVENTS		10
	struct epoll_event ev, events[MAX_EVENTS];
	int epollfd;
	int timerfd;

	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = TcpListenPort->ListenFd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		perror("epoll_ctl: TcpListenPort");
		exit(EXIT_FAILURE);
	}
	
	timerfd = timerfd_open(1, 1);
	ev.events = EPOLLIN;
	ev.data.fd = timerfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		perror("epoll_ctl: timerfd");
		exit(EXIT_FAILURE);
	}

	printf("Listening for connections\n");

    while(!user_quit) {
		int n;
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == TcpListenPort->ListenFd) {
				TTcpConnectedPort connectedPort = AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen);
				if (connectedPort == -1) {  
					printf("AcceptTcpConnection Failed\n");
					continue;
				}
				if (gTcpConnectedPort != -1) {
					printf("Already connected, disconnecting..\n");
					CloseTcpConnectedPort(connectedPort);
					continue;
				}
				gTcpConnectedPort = connectedPort;
				printf("Accepted connection Request\n");
				ev.data.fd = gTcpConnectedPort;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
					perror("epoll_ctl: conn_sock");
					exit(EXIT_FAILURE);
				}
				if (face_detect_autostart)
					start_video_stream(gTcpConnectedPort);
			} else if (events[n].data.fd == timerfd) {
				handle_timer(timerfd);
			} else { // from client
				handle_client_msg(epollfd, events[n].data.fd);
			}
		}
    }   

    SAFE_DELETE(g_camera);
    //TODO : CHECK(cudaFreeHost(rgb_cpu));
    //TODO : CHECK(cudaFreeHost(cropped_buffer_cpu[0]));
    //TODO : CHECK(cudaFreeHost(cropped_buffer_cpu[1]));
    CloseTcpConnectedPort(gTcpConnectedPort); // Close network port;
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

static int u_ignore_sigpipe(void) {
	struct sigaction act;

	if (sigaction(SIGPIPE, (struct sigaction *)NULL, &act) == -1)
		return -1;
	if (act.sa_handler == SIG_DFL) {
		act.sa_handler = SIG_IGN;
		if (sigaction(SIGPIPE, &act, (struct sigaction *)NULL) == -1)
			return -1;
	}
	return 0;
}

static void get_env_value(void)
{
	char *value;

	value = getenv("FACE_DETECT_MAX");
	if (value) {
		face_detect_max = strtol(value, NULL, 0);
		printf("[%s] FACE_DETECT_MAX face_detect_max = %d\n", __func__, face_detect_max);
	}

	value = getenv("FACE_DETECT_AUTOSTART");
	if (value) {
		face_detect_autostart = strtol(value, NULL, 0);
		printf("[%s] FACE_DETECT_AUTOSTART face_detect_autostart = %d\n", __func__, face_detect_autostart);
	}

	value = getenv("FACE_DETECT_MEASURE");
	if (value) {
		face_detect_measure = strtol(value, NULL, 0);
		printf("[%s] FACE_DETECT_MEASURE face_detect_measure = %d\n", __func__, face_detect_measure);
	}

	value = getenv("FACE_DETECT_SEND_TIMESTAMP");
	if (value) {
		face_detect_send_timestamp = strtol(value, NULL, 0);
		printf("[%s] FACE_DETECT_SEND_TIMESTAMP face_detect_send_timestamp = %d\n", __func__, face_detect_send_timestamp);
	}
}

int main(int argc, char *argv[])
{
    int status;
    int terminated_pid;
    pid_t pid;

    int state = 0;
    //set_rt_policy(getpid());
    u_ignore_sigpipe();

    while(1) {
        pid = fork();

        if (pid < 0) {
            printf("[Error] fork failed!\n");
        }
        else if (pid == 0) { // child
            get_env_value();
            state = camera_face_recognition( argc, argv );
            if(state == 1) cout << "Restart is required! Please type ./main again." << endl;
            exit(1);
        }
        else { // parent
            printf("parent process is going to wait!\n");
            terminated_pid = wait(&status);
            printf("terminated pid = %d\n", terminated_pid);
        }
    }

    return 0;
}
