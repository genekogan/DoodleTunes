#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxGrt.h"
#include "ofxCcv.h"
#include "ofxOsc.h"
using namespace ofxCv;
using namespace cv;

// where to send osc messages by default
#define DEFAULT_OSC_DESTINATION "localhost"
#define DEFAULT_OSC_ADDRESS "/classification"
#define DEFAULT_OSC_PORT 9000

// instrument names
struct FoundSquare {
    ofImage img;
    string label;
    bool isPrediction = false;
    cv::Rect rect;
    float area;
    void draw();
};


class ofApp : public ofBaseApp
{
public:
    
    vector<string> classNames = {
        "drums",
        "bass guitar",
        "saxophone",
        "keyboard"
    };

    void setup();
    void update();
    void draw();
    void exit();
    
    void setTrainingLabel(int & label_);
    void addSamplesToTrainingSet();
    void gatherFoundSquares();
    void trainClassifier();
    void classifyCurrentSamples();
    
    void addSamplesToTrainingSetNext();
    void classifyNext();
    
    void save();
    void load();
    
    void keyPressed(int key);
    
    int width, height;
    float msgColor;
    
    ofVideoGrabber cam;
    ContourFinder contourFinder, contourFinder2;
    ofFbo fbo;
    ofxCvColorImage colorImage;
    ofxCvGrayscaleImage grayImage, rImage, gImage, bImage;
    ofxCvGrayscaleImage rgDiff, rbDiff, gbDiff;
    ofxCvGrayscaleImage grayBg, graySnapshot;
    ofxCvGrayscaleImage grayDiff, graySnapshotDiff;
    cv::Scalar diffMean;
    
    ofxOscSender sender;
    string oscDestination, oscAddress;
    int oscPort;
    
    ofxPanel gui;
    ofxToggle bRunning;
    ofxButton bAdd, bTrain, bClassify, bSave, bLoad;
    ofParameter<float> minArea, maxArea, threshold;
    ofParameter<float> maxMotion, amtMotion, minDiff, amtDiff, amtColor, maxColor, lerpMotion;
    ofParameter<int> nDilate;
    ofParameter<int> trainingLabel;
   
    int nInstruments;
    vector<FoundSquare> foundSquares;
    vector<int> instrumentCountPrev;
    vector<float> instrumentAreaPrev;
    
    ClassificationData trainingData;
    GestureRecognitionPipeline pipeline;
    ofxCcv ccv;
    bool isTrained, toAddSamples, toClassify;
};



