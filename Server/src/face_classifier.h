//Author : Daniel595

#ifndef FACE_CLASSIFIER_H
#define FACE_CLASSIFIER_H

#include "face_embedder.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <dlib/svm_threaded.h>
#include <dlib/svm.h>
#include <vector>
#include "json/json.h"
#include <fstream>



class face_classifier{

    typedef matrix<float, 0, 1> sample_type_embedding;          //type: collumn-vector with dynamic size to store the training-face embeddings
    typedef matrix<double, 0, 1> sample_type_svm;               //type: collumn-vector with dtype double which is required to train a SVM
    
    // SVM stuff 
    typedef histogram_intersection_kernel<sample_type_svm> kernel_type;
    typedef svm_c_trainer<kernel_type> trainer_type;
    typedef decision_function<kernel_type> classifier_type;    


public:
    face_classifier(face_embedder *embedder);
    ~face_classifier(void);
    void prediction(std::vector<sample_type_embedding> *face_embeddings, 
                    std::vector<double> *face_labels,
                    std::vector<matrix<rgb_pixel>> *faces,
                    bool retroactive_mode = false);
    void prediction(sample_type_embedding *face_embedding, 
                    double *face_label);
    void get_label_encoding(std::vector<std::string> *labels);
    int need_restart();

    // retroactive
    void retroactive_init();
    void save_unknown_data(string &unknown_img_filename, string &detection_time);
    void handle_unknown_data(matrix<rgb_pixel> &face);
    void load_poi_data();
    Json::Value poi_data;

private:
    void init(face_embedder *embedder);
    int deserialize_svm(int classes);
    void train_svm(std::vector<string> *info, face_embedder *embedder);
    void get_training_data(std::vector<sample_type_embedding> *face_embeddings, 
                            std::vector<double> *labels, 
                            std::vector<double> *total_labels,
                            face_embedder *embedder);
    void training(std::vector<sample_type_embedding> *face_embeddings, 
                    std::vector<double> *labels, 
                    std::vector<double> *total_labels );
    void serialize_svm();
    int get_info(std::vector<string> *info);
    int get_labels();

    sample_type_embedding test_embedding;
    std::string test_dir = "faces/test.jpg";                // a test image
    std::string labels_dir = "faces/labels.txt";            // file containing the class names
    std::string info_dir = "faces/info.txt";                // information about svm training
    std::string svm_dir = "svm/";                           // dir to store the svm for later reuse
    std::string train_data_dir = "faces/train/cropped/";    // dir containing the training face-images
    std::string unknown_dir = "faces/unknown/";             // dir containing unknown images when retroactive mode
    std::string unknown_filename = "faces/unknown_history.txt";    // file containing the time unknown person is detected
    std::string poi_filename = "faces/poi_info_file.txt";   // file containing the label, detection data of person of interest


	FILE *unknown_filp = NULL;
    
    //face_embedder embedder;                                 // embeddings network for generate training data
    std::vector<std::string> label_encoding;                // persons/class names
    std::vector<classifier_type> classifiers;               // actual classifiers/svm's
    std::vector<pair<double, double>> classifiersLabels;    // labels/positive and negative of each svm

    int num_classes;
    int num_classifiers;
    int restart;

    // retroactive
    int retroactive;
    int unknown_index;
    int frame_cnt;
};


#endif


