//
// Created by zhou on 18-5-4.
//

#ifndef MAIN_RNET_RT_H
#define MAIN_RNET_RT_H

#include "baseEngine.h"
#include "network.h"

class Rnet_engine : public baseEngine {

public:
    Rnet_engine();
    ~Rnet_engine();
    void serialize();
    void init(int row, int col);
    std::string filename_base = "rnet_engine_";
    friend class Rnet;
};

class Rnet {
public:
    Rnet(const Rnet_engine &rnet_engine);
    ~Rnet();
    void run(const int input_num,const Rnet_engine &engine, cudaStream_t& stream);
    mydataFmt Rthreshold;
    cudaStream_t stream;
    struct pBox *location_;
    struct pBox *score_;
    struct pBox *rgb;
    int INPUT_H;
    int INPUT_W;
    int OUT_PROB_SIZE;
    int OUT_LOCATION_SIZE;
    int inputIndex,outputProb,outputLocation;
    void *buffers[3];
//    void generate_batches(void* boxes_data, cuda::GpuMat& image);
private:
    const int BatchSize;
    const int INPUT_C;
    const ICudaEngine &Engine;
    //must be computed at runtime

};


#endif //MAIN_RNET_RT_H
