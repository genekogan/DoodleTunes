#pragma once

#include "ofMain.h"
#include "ofxGrt.h"
#include "ofxCcv.h"
#include "FoundSquare.h"
#include <atomic>


class ThreadedCcv: public ofThread {
public:
    ~ThreadedCcv();
    
    void setup(string ccvPath, map<int, FoundSquare*> * foundSquares);
    void update();
    
    void addSamples(int nAugment, float maxAng, bool flipH);
    void trainClassifier();
    void predict();
    
    bool getIsTrained(){return isTrained;}
    bool getIsPredicting(){return toPredict;}
    bool getHasResults(){return hasResults;}
    void resetResults(){hasResults = false;}
    
    void start();
    void stop();
    
    void save();
    void load();
    
protected:
    
    void addSample(vector<float> & encoding, int label);
    void runPredictions();
    void threadedFunction();
    
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

