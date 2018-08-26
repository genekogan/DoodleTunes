#include "ThreadedCcv.h"


//--------------------------------------------------------------
ThreadedCcv::~ThreadedCcv(){
    stop();
    waitForThread(false);
}

//--------------------------------------------------------------
void ThreadedCcv::setup(string ccvPath, map<int, FoundSquare*> * foundSquares){
    this->foundSquares = foundSquares;

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
void ThreadedCcv::predict() {
    if (!isTrained) {
        ofLog() << "Not trained yet... go away";
        return;
    }
    toPredict = true;
}

//--------------------------------------------------------------
void ThreadedCcv::addSample(vector<float> & encoding, int label) {
    VectorFloat inputVector(encoding.size());
    for (int i=0; i<encoding.size(); i++) inputVector[i] = encoding[i];
    trainingData.addSample(label, inputVector);
}

//--------------------------------------------------------------
void ThreadedCcv::addSamples(int nAugment, float maxAng, bool flipH) {
    ofLog(OF_LOG_NOTICE, "Adding samples...");
    
    ofImage img;
    int numSamples = foundSquares->size() * (1 + nAugment) * (flipH ? 2 : 1);
    int idx = 0;
    for (map<int, FoundSquare*>::iterator it = foundSquares->begin(); it != foundSquares->end(); it++) {
        vector<float> encoding = ccv.encode(it->second->img, ccv.numLayers()-1);
        int label = it->second->idxLabel;
        addSample(encoding, label);
        ofLog(OF_LOG_NOTICE, " Added sample #"+ofToString(++idx)+"/"+ofToString(numSamples)+", label="+ofToString(label));
        
        if (flipH) {
            ofImage imgFlipped;
            imgFlipped.setFromPixels(it->second->img);
            imgFlipped.mirror(false, true);
            encoding = ccv.encode(imgFlipped, ccv.numLayers()-1);
            addSample(encoding, label);
            ofLog(OF_LOG_NOTICE, " Added sample #"+ofToString(++idx)+"/"+ofToString(numSamples)+", label="+ofToString(label));
        }

        for (int a=0; a<nAugment; a++) {
            float ang = ofRandom(-maxAng, maxAng);
            ofPushMatrix();
            ofFbo fbo;
            fbo.allocate(it->second->img.getWidth(), it->second->img.getHeight());
            fbo.begin();
            ofSetColor(255);
            ofFill();
            ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
            ofSetRectMode(OF_RECTMODE_CENTER);
            ofTranslate(fbo.getWidth()/2, fbo.getHeight()/2);
            ofRotateDeg(ang);
            ofScale(ofMap(abs(ang), 0, maxAng, 1, 0.95));
            it->second->draw(0, 0, fbo.getWidth(), fbo.getHeight());
            fbo.end();
            ofPopMatrix();

            fbo.readToPixels(img.getPixels());
            encoding = ccv.encode(img, ccv.numLayers()-1);
            addSample(encoding, label);
            ofLog(OF_LOG_NOTICE, " Added sample #"+ofToString(++idx)+"/"+ofToString(numSamples)+", label="+ofToString(label));
            
            if (flipH) {
                img.mirror(false, true);
                encoding = ccv.encode(img, ccv.numLayers()-1);
                addSample(encoding, label);
                ofLog(OF_LOG_NOTICE, " Added sample #"+ofToString(++idx)+"/"+ofToString(numSamples)+", label="+ofToString(label));
            }
        }
    }
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
void ThreadedCcv::runPredictions() {
    isPredicting = true;
    
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
            runPredictions();
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

