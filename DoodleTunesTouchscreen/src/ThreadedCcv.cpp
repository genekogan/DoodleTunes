#include "ThreadedCcv.h"


//--------------------------------------------------------------
ThreadedCcv::~ThreadedCcv(){
    stop();
    waitForThread(false);
}

//--------------------------------------------------------------
void ThreadedCcv::setup(string ccvPath){
    toPredict = false;
    hasResults = false;
    isPredicting = false;
    isTrained = false;
    ccv.setup(ccvPath);
    
    trainingData.setNumDimensions(4096);
    AdaBoost adaboost;
    adaboost.enableNullRejection(false);
    adaboost.setNullRejectionCoeff(3);
    adaboost.setMinNumEpochs(1000);
    adaboost.setMaxNumEpochs(2000);
    adaboost.setNumBoostingIterations(300);
    pipeline.setClassifier(adaboost);
    
    start();
}

//--------------------------------------------------------------
void ThreadedCcv::start(){
    startThread();
}

//--------------------------------------------------------------
void ThreadedCcv::stop(){
    std::unique_lock<std::mutex> lck(mutex);
    stopThread();
    condition.notify_all();
}

//--------------------------------------------------------------
void ThreadedCcv::update(){
    std::unique_lock<std::mutex> lock(mutex);
    condition.notify_all();
}

//--------------------------------------------------------------
void ThreadedCcv::sendFoundSquares(map<int, FoundSquare*> * foundSquares) {
    if (!isTrained) {
        ofLog() << "Not trained yet... go away";
        return;
    }
    this->foundSquares = foundSquares;
    toPredict = true;
}

//--------------------------------------------------------------
void ThreadedCcv::trainClassifier() {
    ofLog(OF_LOG_NOTICE, "Training...");
    if (pipeline.train(trainingData)){
        ofLog(OF_LOG_NOTICE, "getNumClasses: "+ofToString(pipeline.getNumClasses()));
    }
    isTrained = true;
    ofLog(OF_LOG_NOTICE, "Done training...");
}

//--------------------------------------------------------------
void ThreadedCcv::predict() {
    isPredicting = true;
    ofLog(OF_LOG_NOTICE, "Classifiying "+ofToString(ofGetFrameNum()));
    
    int i = 0;
    for (map<int, FoundSquare*>::iterator it = foundSquares->begin(); it != foundSquares->end(); it++) {
        if (it->second->change < 1.0 && it->second->idxLabel > -1) {
            continue;
        }
        vector<float> encoding = ccv.encode(it->second->img, ccv.numLayers()-1);
        VectorFloat inputVector(encoding.size());
        for (int i=0; i<encoding.size(); i++) inputVector[i] = encoding[i];
        if (pipeline.predict(inputVector)) {
            int label = pipeline.getPredictedClassLabel();
            it->second->label = classNames[label];
            it->second->idxLabel = label;
            it->second->borderColor = 0;
        }
    }
    hasResults = true;
    isPredicting = false;
}

//--------------------------------------------------------------
void ThreadedCcv::threadedFunction(){
    while(isThreadRunning()){
        std::unique_lock<std::mutex> lock(mutex);
        if (toPredict) {
            predict();
            toPredict = false;
        }
        condition.wait(lock);
    }
}

//--------------------------------------------------------------
void ThreadedCcv::save() {
    pipeline.save(ofToDataPath("doodleclassifier_model.grt"));
    trainingData.save(ofToDataPath("TrainingData.grt"));
}

//--------------------------------------------------------------
void ThreadedCcv::load() {
    pipeline.load(ofToDataPath("doodleclassifier_model.grt"));
    isTrained = true;
}

