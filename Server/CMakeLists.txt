cmake_minimum_required(VERSION 3.5)
project(LgFaceRecDemoTCP)
SET(CUDA_VERSION 10.0)
FIND_PACKAGE(CUDA ${CUDA_VERSION} REQUIRED)

set(ARCH ${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)

add_definitions(-std=c++11)
include_directories(/usr/local/cuda/include)
include_directories(/home/lg/jetson-utils-master_latest/build/${CMAKE_SYSTEM_PROCESSOR}/include/jetson-utils)
include_directories(/home/lg/dlib-19.22/dlib/build)
include_directories(/usr/include/gstreamer-1.0)
include_directories(/usr/lib/${ARCH}/gstreamer-1.0/include)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/include/libxml2 )
include_directories(/usr/lib/${ARCH}/glib-2.0/include)
link_directories(/opt/cuda/lib64)
link_directories(/usr/include )
link_directories(/home/lg/dlib-19.22/dlib/build)

find_package( OpenCV REQUIRED)
find_package ( Boost REQUIRED COMPONENTS system filesystem)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(src)

set(CORE_SRCS
    src/alignment.cpp 
    src/baseEngine.cpp 
    src/common.cpp 
    src/face_classifier.cpp
    src/face_embedder.cpp 
    src/kernels.cu 
    src/mtcnn.cpp
    src/network.cpp
    src/NetworkTCP.cpp 
    src/onet_rt.cpp 
    src/pnet_rt.cpp 
    src/rnet_rt.cpp 
    src/TcpSendRecvJpeg.cpp
    )

# AUX_SOURCE_DIRECTORY(./src DIR_SRCS)
cuda_add_executable( LgFaceRecDemoTCP ${CORE_SRCS} src/LgFaceRecDemoTCP.cpp)
cuda_add_executable( Monitor src/Monitor.cpp)


set(CMAKE_CXX_FLAGS  "-lpthread")
set(CMAKE_CXX_FLAGS  "-O3")
set(CMAKE_CXX_FLAGS  "-Wno-deprecated-declarations")

message(${Boost_LIBRARIES})

target_link_libraries( LgFaceRecDemoTCP ${Boost_LIBRARIES})

target_link_libraries( LgFaceRecDemoTCP ${OpenCV_LIBS} )
target_link_libraries( LgFaceRecDemoTCP /home/lg/dlib-19.22/dlib/build/libdlib.a )
target_link_libraries( LgFaceRecDemoTCP cudart)
target_link_libraries( LgFaceRecDemoTCP nvinfer)
target_link_libraries( LgFaceRecDemoTCP nvparsers)
target_link_libraries( LgFaceRecDemoTCP cblas)
target_link_libraries( LgFaceRecDemoTCP cudnn)
target_link_libraries( LgFaceRecDemoTCP curand)
target_link_libraries( LgFaceRecDemoTCP cusolver)
target_link_libraries( LgFaceRecDemoTCP cublas)
target_link_libraries( LgFaceRecDemoTCP /home/lg/jetson-utils-master_latest/build/${CMAKE_SYSTEM_PROCESSOR}/lib/libjetson-utils.so)
target_link_libraries( LgFaceRecDemoTCP jpeg )
target_link_libraries( LgFaceRecDemoTCP png )

