#pragma once

#include "ofMain.h"
#include "ofxGrt.h"
#include "ofxCcv.h"
#include "FoundSquare.h"
#include <atomic>


class ThreadedCcv: public ofThread {
public:
    ~ThreadedCcv();
    void setup(string ccvPath);
    
    void trainClassifier();
    void predict();
    
    void sendFoundSquares(map<int, FoundSquare*> * foundSquares);
    
    bool getIsTrained(){return isTrained;}
    bool getIsPredicting(){return toPredict;}
    bool getHasResults(){return hasResults;}
    void resetResults(){hasResults = false;}
    
    void update();
    void threadedFunction();
    void start();
    void stop();
    
    void save();
    void load();
    
protected:
    
    std::condition_variable condition;
    map<int, FoundSquare*> *foundSquares;
    
    bool isTrained;
    bool toPredict;
    bool isPredicting;
    bool hasResults;
    
    ClassificationData trainingData;
    GestureRecognitionPipeline pipeline;
    ofxCcv ccv;
};

