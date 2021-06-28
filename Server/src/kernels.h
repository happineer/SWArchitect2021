#ifndef KERNEL_H_INCLUDED
#define KERNEL_H_INCLUDED

#include<cuda.h>
#include<cuda_runtime.h>
#include<opencv4/opencv2/opencv.hpp>
// #include<opencv2/gpu/gpu.hpp> 
//#include<opencv2/core/cuda_devptrs.hpp> 
#include<opencv4/opencv2/core/cuda_types.hpp> 
// #include<opencv2/gpu/stream_accessor.hpp> 
#include <opencv4/opencv2/core/cuda_stream_accessor.hpp>
#include "device_launch_parameters.h"
#include <opencv4/opencv2/cudaimgproc.hpp>
#include <opencv4/opencv2/cudafeatures2d.hpp>
#include <opencv4/opencv2/cudawarping.hpp>
//using namespace cv;

void gpu_image2Matrix(int width, int height , cv::cuda::GpuMat & image, float* matrix, cudaStream_t& stream);
//void gpu_generatebox(void * score, void * location, float scale);
//void boxes2bactch(int num, int crop_size, int width, int height, int* boxes_data, cuda::GpuMat, cudaStream_t& stream);
void gpu_image2Matrix_with_transpose(int width, int height,  cv::cuda::GpuMat & image, float* matrix, cudaStream_t &stream);
#endif //KERNEL_H_INCLUDED