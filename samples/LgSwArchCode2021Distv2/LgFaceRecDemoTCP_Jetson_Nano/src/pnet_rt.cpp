//
// Created by zhou on 18-4-30.
//
#include "pnet_rt.h"
#include "kernels.h"
#include <fstream>
#include <thrust/device_vector.h>
// stuff we know about the network and the caffe input/output blobs
Pnet_engine::Pnet_engine() : baseEngine("src/models/det1_relu.prototxt",
                                        "src/models/det1_relu.caffemodel",
                                        "data",
                                        "conv4-2",
                                        "prob1") {
};

Pnet_engine::~Pnet_engine() {
    shutdownProtobufLibrary();
}
void Pnet_engine::init(int row, int col, size_t index) {

    //modifiy the input shape of prototxt, write to temp.prototxt
    int first_spce = 16, second_space = 4;
    fstream protofile;
    protofile.open(prototxt, ios::in);
    std::stringstream buffer;
    buffer << protofile.rdbuf();
    std::string contents(buffer.str());
    string::size_type position_h, position_w;
    position_h = contents.find("dim");
    while (isdigit(contents[position_h + first_spce])) {
        contents.erase(position_h + first_spce, 1);
    }
    contents.insert(position_h + first_spce, to_string(row));
    position_w = contents.find("dim", position_h + first_spce);
    while (isdigit(contents[position_w + second_space])) {
        contents.erase(position_w + second_space, 1);
    }
    contents.insert(position_w + second_space, to_string(col));
    protofile.close();
    protofile.open("temp.prototxt", ios::out);
    protofile.write(contents.c_str(), contents.size());
    protofile.close();
    IHostMemory *gieModelStream{nullptr};
    
    // check if this model already exists and try do deserialize
    string filename = filename_base + to_string((int)index) + "_" + to_string(row) + "_" + to_string(col) + ".engine";
    if (!deserialize_engine(filename)){ 
        // if deserialization is not possible generate Tensorrt model
        caffeToGIEModel("temp.prototxt", model, std::vector<std::string>{OUTPUT_PROB_NAME, OUTPUT_LOCATION_NAME}, 1,
                        gieModelStream);
        // and save it for later use
        serialize_engine(filename);
    }

}


Pnet::Pnet(int row, int col, const Pnet_engine &pnet_engine) : BatchSize(1),
                                                               INPUT_C(3), Engine(pnet_engine.context->getEngine()) {
    Pthreshold = 0.6;
    this->score_ = new pBox;
    this->location_ = new pBox;
//    this->rgb = new pBox;
    INPUT_W = col;
    INPUT_H = row;
    //calculate output shape
    this->score_->width = int(ceil((INPUT_W - 2) / 2.) - 4);
    this->score_->height = int(ceil((INPUT_H - 2) / 2.) - 4);
    this->score_->channel = 2;

    this->location_->width = int(ceil((INPUT_W - 2) / 2.) - 4);
    this->location_->height = int(ceil((INPUT_H - 2) / 2.) - 4);
    this->location_->channel = 4;

    OUT_PROB_SIZE = this->score_->width * this->score_->height * this->score_->channel;
    OUT_LOCATION_SIZE = this->location_->width * this->location_->height * this->location_->channel;
    //allocate memory for outputs

    this->score_->pdata = (float *) malloc(OUT_PROB_SIZE * sizeof(float));
    this->location_->pdata = (float *) malloc(OUT_LOCATION_SIZE * sizeof(float));

    assert(Engine.getNbBindings() == 3);
    inputIndex = Engine.getBindingIndex(pnet_engine.INPUT_BLOB_NAME),
    outputProb = Engine.getBindingIndex(pnet_engine.OUTPUT_PROB_NAME),
    outputLocation = Engine.getBindingIndex(pnet_engine.OUTPUT_LOCATION_NAME);

    //creat GPU buffers and stream
    CHECK(cudaMalloc(&buffers[inputIndex], BatchSize * INPUT_C * INPUT_H * INPUT_W * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputProb], BatchSize * OUT_PROB_SIZE * sizeof(float)));
    CHECK(cudaMalloc(&buffers[outputLocation], BatchSize * OUT_LOCATION_SIZE * sizeof(float)));
    CHECK(cudaMalloc(&input_matrix,BatchSize * INPUT_C * INPUT_H * INPUT_W * sizeof(float)));
    cuda_stream = cv::cuda::StreamAccessor::getStream(cv_stream);
}

Pnet::~Pnet() {

    delete (score_);
    delete (location_);

    CHECK(cudaFree(buffers[inputIndex]));
    CHECK(cudaFree(buffers[outputProb]));
    CHECK(cudaFree(buffers[outputLocation]));
    CHECK(cudaFree(input_matrix ));
}

void Pnet::run(cv::cuda::GpuMat &image, float scale, const Pnet_engine &pnet_engine) {


    //DMA the input to the GPU ,execute the batch asynchronously and DMA it back;
//    continous(image);
    gpu_image2Matrix(INPUT_W,INPUT_H,image,(float*)buffers[inputIndex], cuda_stream);
    pnet_engine.context->enqueue(BatchSize, buffers, cuda_stream, nullptr);


#ifdef CPU
    CHECK(cudaMemcpyAsync(this->score_->pdata, buffers[outputProb], BatchSize * OUT_PROB_SIZE * sizeof(float),
                          cudaMemcpyDeviceToHost, cuda_stream));
    CHECK(cudaMemcpyAsync(this->location_->pdata, buffers[outputLocation],
                          BatchSize * OUT_LOCATION_SIZE * sizeof(float), cudaMemcpyDeviceToHost, cuda_stream));
#endif
#ifndef CPU
//    generateBbox(buffers[outputProb],  buffers[outputLocation], scale);
#endif
}

void Pnet::cpu_generateBbox(const pBox *score, const pBox  *location, mydataFmt scale) {
    //for pooling
    int stride = 2;
    int cellsize = 12;
    int count = 0;
    //score p

    mydataFmt *p = score->pdata + score->width * score->height;
    mydataFmt *plocal = location->pdata;
    struct Bbox bbox;
    struct orderScore order;
    for (int row = 0; row < score->height; row++) {
        for (int col = 0; col < score->width; col++) {
            if (*p > Pthreshold) {
                bbox.score = *p;
                order.score = *p;
                order.oriOrder = count;
                bbox.x1 = round((stride * row + 1) / scale);
                bbox.y1 = round((stride * col + 1) / scale);
                bbox.x2 = round((stride * row + 1 + cellsize) / scale);
                bbox.y2 = round((stride * col + 1 + cellsize) / scale);
                bbox.exist = true;
                bbox.area = (bbox.x2 - bbox.x1) * (bbox.y2 - bbox.y1);
                for (int channel = 0; channel < 4; channel++)
                    bbox.regreCoord[channel] = *(plocal + channel * location->width * location->height);
                boundingBox_.push_back(bbox);
                bboxScore_.push_back(order);
                count++;
            }
            p++;
            plocal++;
        }
    }

}