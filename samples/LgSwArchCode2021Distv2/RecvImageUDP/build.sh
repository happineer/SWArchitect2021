g++ RecvImageUDP.cpp ../Common/NetworkUDP.cpp ../Common/UdpSendRecvJpeg.cpp -o RecvImageUDP -I../Common -I/opt/intel/openvino/opencv/include -L/opt/intel/openvino/opencv/lib  -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_video -lopencv_videoio


