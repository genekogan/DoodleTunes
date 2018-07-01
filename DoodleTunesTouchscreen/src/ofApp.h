#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxGrt.h"
#include "ofxCcv.h"
#include "ofxOsc.h"
#include "ofxSequencer.h"
#include "DrawGui.h"

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



// todo
// - should update when cv params shifted
// - retrain + augmentation
// - combined interface
// - integrate abletn

// git
// - original, shanghai (drawing), nixdorf (touchscreen)


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
    void setupAudio();
    void update();
    void draw();
    void drawDebug();
    void drawPresent();
    void exit();
    
    void setTrainingLabel(int & label_);
    void addSamplesToTrainingSet();
    void gatherFoundSquares();
    void trainClassifier();
    void classifyCurrentSamples();
    
    void addSamplesToTrainingSetNext();
    void classifyNext();
    void beatsIn(int & eventInt);
    void playbackChange();
    void sendOSC();
    
    void save();
    void load();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    
    int width, height;
    
    //ofVideoGrabber cam;
    ContourFinder contourFinder, contourFinder2;
    ofFbo fbo;
    ofxCvGrayscaleImage grayImage;
    ofxCvColorImage colorImage;
    
    ofxOscSender sender;
    string oscDestination, oscAddress;
    int oscPort;
    
    ofxPanel gui;
    ofxToggle bRunning;
    ofxButton bAdd, bTrain, bClassify, bSave, bLoad;
    ofParameter<float> minArea, maxArea, threshold;
    ofParameter<int> nDilate;
    ofParameter<int> trainingLabel;
    
    // music stuff
    int nInstruments;
    vector<FoundSquare> foundSquares;
    vector<int> instrumentCountPrev, instrumentCount;
    vector<float> instrumentAreaPrev, instrumentArea;
    
    // training
    ClassificationData trainingData;
    GestureRecognitionPipeline pipeline;
    ofxCcv ccv;
    bool isTrained, toAddSamples, toClassify;
    
    // interface
    DrawGui drawer;
    bool debug;
    
    
    
    
    bool toUpdateSound;
    
    
    
    ////////
    ofxSequencer sequencer;
    
    ofParameter<int> drumController;
    ofParameter<int> bassController;
    ofParameter<int> pianoController;
    ofParameter<int> saxController;
    
    int cols;
    int rows;
    
    ofEvent <int> beatEvent;
    string displayString;
    
    //SoundPlayer
    vector <ofSoundPlayer>  drum;
    vector <ofSoundPlayer>  bass;
    vector <ofSoundPlayer>  piano;
    vector <ofSoundPlayer>  sax;
    
    int numSamples;
    
};



