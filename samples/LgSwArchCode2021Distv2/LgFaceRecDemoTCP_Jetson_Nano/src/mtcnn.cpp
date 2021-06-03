#include "mtcnn.h"
#include "kernels.h"
#include "cudaOverlay.h"


mtcnn::mtcnn(int row, int col){

    //set NMS thresholds
    nms_threshold[0] = 0.5;
    nms_threshold[1] = 0.6;
    nms_threshold[2] = 0.6;

    //set minimal face size (width in pixels)
    int minsize = 40;

    /*config  the pyramids */
    float minl = row<col?row:col;
    int MIN_DET_SIZE = 12;
    float m = (float)MIN_DET_SIZE/minsize;
    minl *= m;
    float factor = 0.709;
    int factor_count = 0;
    while(minl>MIN_DET_SIZE){
        if(factor_count>0)m = m*factor;
        scales_.push_back(m);
        minl *= factor;
        factor_count++;
    }
    float minside = row<col ? row : col;
    int count = 0;
    for (vector<float>::iterator it = scales_.begin(); it != scales_.end(); it++){
        if (*it > 1){
            cout << "the minsize is too small" << endl;
            while (1);
        }
        if (*it < (MIN_DET_SIZE / minside)){
            scales_.resize(count);
            break;
        }
        count++;
    }

    cout<<"Start generating TenosrRT runtime models"<<endl;
    
    //generate pnet models (multiple instances with different inputs)
    printf("generate P-net\n");
    pnet_engine = new Pnet_engine[scales_.size()];
    simpleFace_ = (Pnet**)malloc(sizeof(Pnet*)*scales_.size());
    for (size_t i = 0; i < scales_.size(); i++) {
        int changedH = (int)ceil(row*scales_.at(i));
        int changedW = (int)ceil(col*scales_.at(i));
        pnet_engine[i].init(changedH,changedW, i);
        simpleFace_[i] =  new Pnet(changedH,changedW,pnet_engine[i]);
    }

    //generate rnet model
    printf("generate R-net\n");
    rnet_engine = new Rnet_engine();
    rnet_engine->init(24,24);
    refineNet = new Rnet(*rnet_engine);

    //generate onet model
    printf("generate O-net\n");
    onet_engine = new Onet_engine();
    onet_engine->init(48,48);
    outNet = new Onet(*onet_engine);

    cout<<"End generating TensorRT runtime models"<<endl;

    //init cuda stream pool
    for(int i = 0;i<rnet_streams_num;i++)
        cudastreams[i] = cv::cuda::StreamAccessor::getStream(cv_streams[i]);

    boxes_data = (float*)malloc(sizeof(int)*4*rnet_max_input_num);
    CHECK(cudaMalloc(&gpu_boxes_data, sizeof(int)*4*rnet_max_input_num));

    cout<<"Input shape "<<row<<"*"<<col<<endl;
    cout<<"Min size "<<minsize<<endl;

}

mtcnn::~mtcnn(){

    delete[] pnet_engine;
    delete rnet_engine;
    delete onet_engine;
    for (int i = 0;i<scales_.size();i++)
    {
        delete(simpleFace_[i]);
    }
    delete simpleFace_;
    simpleFace_ = NULL;
    delete refineNet;
    delete outNet;
    free(boxes_data);
    CHECK(cudaFree(gpu_boxes_data));

}

void mtcnn::findFace(cv::cuda::GpuMat &image, vector<struct Bbox> * detections){
    struct orderScore order;
    int count = 0;

    clock_t first_time = clock();

    //Run pnet in parallel
    for (size_t i = 0; i < scales_.size(); i++) {
        int changedH = (int)ceil(image.rows*scales_.at(i));
        int changedW = (int)ceil(image.cols*scales_.at(i));
        (*simpleFace_[i]).scale = scales_.at(i);
        cv::cuda::resize(image, firstImage_buffer[i], cv::Size(changedW, changedH), 0, 0, cv::INTER_LINEAR,simpleFace_[i]->cv_stream);
        (*simpleFace_[i]).run(firstImage_buffer[i], scales_.at(i),pnet_engine[i]);
    }

    //generate bbox
    for(int i = scales_.size()-1;i>=0;i--)
    {
        cudaStreamSynchronize(simpleFace_[i]->cuda_stream); //Synchronize
        simpleFace_[i]->cpu_generateBbox(simpleFace_[i]->score_, simpleFace_[i]->location_, simpleFace_[i]->scale);
        nms((*simpleFace_[i]).boundingBox_, (*simpleFace_[i]).bboxScore_, nms_threshold[0]);
        for(vector<struct Bbox>::iterator it=(*simpleFace_[i]).boundingBox_.begin(); it!= (*simpleFace_[i]).boundingBox_.end();it++){
            if((*it).exist){
                firstBbox_.push_back(*it);
                order.score = (*it).score;
                order.oriOrder = count;
                firstOrderScore_.push_back(order);
                count++;
            }
        }
        (*simpleFace_[i]).bboxScore_.clear();
        (*simpleFace_[i]).boundingBox_.clear();
    }

    //the first stage's nms
    if(count<1)return;
    nms(firstBbox_, firstOrderScore_, nms_threshold[0]);
    refineAndSquareBbox(firstBbox_, image.rows, image.cols,true);

#ifdef LOG
    cout<<"Pnet time is "<<1000*double(clock()-first_time)/CLOCKS_PER_SEC<<endl;
#endif

    //second stage
    count = 0;
    clock_t second_time = clock();
    int inputted_num = 0;
    int second_step = 24*24*3;

    for(vector<struct Bbox>::iterator it=firstBbox_.begin(); it!=firstBbox_.end();it++){
        if((*it).exist){
            cv::Rect temp((*it).y1, (*it).x1, (*it).y2-(*it).y1, (*it).x2-(*it).x1);
            cv::cuda::resize(image(temp), secImages_buffer[inputted_num],
                    cv::Size(24, 24), 0, 0, cv::INTER_LINEAR, cv_streams[inputted_num]);
            gpu_image2Matrix_with_transpose(24,24,secImages_buffer[inputted_num],
                    (float*)(refineNet->buffers[refineNet->inputIndex])+inputted_num*second_step,cudastreams[inputted_num]);
            inputted_num++;
        }
    }

#ifdef LOG
    cout<<"Rnet input images number is "<<inputted_num<<endl;
#endif    

    cudaDeviceSynchronize();
    refineNet->run(inputted_num, *rnet_engine, refineNet->stream);

    int indx = 0;
    for(vector<struct Bbox>::iterator it=firstBbox_.begin(); it!=firstBbox_.end();it++)
    {
        if(it->exist)
        {
            if(*(refineNet->score_->pdata+ indx*refineNet->OUT_PROB_SIZE+1)>refineNet->Rthreshold)
            {
                memcpy(it->regreCoord, refineNet->location_->pdata+indx*refineNet->OUT_LOCATION_SIZE, refineNet->OUT_LOCATION_SIZE*sizeof(float));
                it->area = (it->x2 - it->x1)*(it->y2 - it->y1);
                it->score = *(refineNet->score_->pdata +indx*refineNet->OUT_PROB_SIZE+1);
                secondBbox_.push_back(*it);
                order.score = it->score;
                order.oriOrder = count++;
                secondBboxScore_.push_back(order);
            }
            else{
                it->exist=false;
            }
            indx++;
        }
    }

    if(count<1)return;
    nms(secondBbox_, secondBboxScore_, nms_threshold[1]);
    refineAndSquareBbox(secondBbox_, image.rows, image.cols,true);

#ifdef LOG    
    second_time = clock() - second_time;
    cout<<"Rnet time is  "<<1000*(double)second_time/CLOCKS_PER_SEC<<endl;
#endif

    //third stage
    count = 0;
    clock_t third_time = clock();
    inputted_num = 0;
    int third_step = 48*48*3;
    for(vector<struct Bbox>::iterator it=secondBbox_.begin(); it!=secondBbox_.end();it++){
        if((*it).exist){
            cv::Rect temp((*it).y1, (*it).x1, (*it).y2-(*it).y1, (*it).x2-(*it).x1);
            cv::cuda::resize(image(temp), thirdImages_buffer[inputted_num],
                         cv::Size(48, 48), 0, 0, cv::INTER_LINEAR, cv_streams[inputted_num]);
            gpu_image2Matrix_with_transpose(48,48,thirdImages_buffer[inputted_num],
                                            (float*)(outNet->buffers[refineNet->inputIndex])+inputted_num*third_step,cudastreams[inputted_num]);
            inputted_num++;
        }
    }

    cudaDeviceSynchronize();
    outNet->run(inputted_num, *onet_engine, outNet->stream);

#ifdef LOG    
    cout<<"Onet input images number is "<<inputted_num<<endl;
#endif

    indx = 0;
    for(vector<struct Bbox>::iterator it=secondBbox_.begin(); it!=secondBbox_.end();it++){
        if((*it).exist){
            mydataFmt *pp=NULL;
            if(*(outNet->score_->pdata + 2*indx +1)>outNet->Othreshold){
                memcpy(it->regreCoord, outNet->location_->pdata + 4*indx, 4*sizeof(mydataFmt));
                it->area = (it->x2 - it->x1)*(it->y2 - it->y1);
                it->score = *(outNet->score_->pdata +2*indx +1);
                pp = outNet->points_->pdata + 10* indx;
                for(int num=0;num<5;num++){
                    (it->ppoint)[num] = it->y1 + (it->y2 - it->y1)*(*(pp+num));
                }
                for(int num=0;num<5;num++){
                    (it->ppoint)[num+5] = it->x1 + (it->x2 - it->x1)*(*(pp+num+5));
                }
                thirdBbox_.push_back(*it);
                order.score = it->score;
                order.oriOrder = count++;
                thirdBboxScore_.push_back(order);
            }
            else{
                it->exist=false;
            }
            indx++;
        }
    }

    if(count<1)return;
    refineAndSquareBbox(thirdBbox_, image.rows, image.cols, true);
    nms(thirdBbox_, thirdBboxScore_, nms_threshold[2], "Min");
    *detections = thirdBbox_;


#ifdef LOG
    cout<<"Onet time is  "<<1000*(double)(clock()-third_time)/CLOCKS_PER_SEC<<endl;
    cout<<"total run time "<<1000*(double)(clock()-first_time)/CLOCKS_PER_SEC<<endl;
#endif



    firstBbox_.clear();
    firstOrderScore_.clear();
    secondBbox_.clear();
    secondBboxScore_.clear();
    thirdBbox_.clear();
    thirdBboxScore_.clear();
}
