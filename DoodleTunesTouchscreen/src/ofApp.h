#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxGrt.h"
#include "ofxCcv.h"
#include "ofxOsc.h"
#include "ofxSequencer.h"
#include "ofxCanvasButton.h"
#include "FoundSquare.h"
#include "ThreadedCcv.h"
#include "DrawGui.h"
#include "AppButton.h"

using namespace ofxCv;
using namespace cv;

// where to send osc messages by default
#define DEFAULT_OSC_DESTINATION "localhost"
#define DEFAULT_OSC_ADDRESS "/classification"
#define DEFAULT_OSC_PORT 9000
#define OSC_ENABLED false



// wishlist
// - thread classification
// - only classify new drawings




// todo
// - clear button
// - should update when cv params shifted
// - clear after clear
// - turn off osc
// x retrain + augmentation
// - combined interface
// x integrate abletn

// x git
// x original, shanghai (drawing), nixdorf (touchscreen)


class ofApp : public ofBaseApp
{
public:

    void setup();
    void setupAudio();
    void update();
    void updateCv();
    void updateAudio();
    void draw();
    void drawDebug();
    void drawPresent();
    void exit();
    
    void setTrainingLabel(int & label_);
    void addSamplesToTrainingSet();
    void gatherFoundSquares(bool augment);
    void trainClassifier();
    void classifyCurrentSamples();
    void updateInstruments();
    void clearDeadSquares();

    void addSamplesToTrainingSetNext();
    void classifyNext();
    void clearButtonClicked();
    void beatsIn(int & eventInt);
    void playbackChange();
    void sendOSC();
    void takeScreenshot();
    void clearDrawer();
    
    void save();
    void load();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseScrolled(int x, int y, float scrollX, float scrollY);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    
    
    // cv
    ContourFinder contourFinder, contourFinder2;
    ofFbo fbo;
    ofxCvGrayscaleImage grayImage;
    ofxCvColorImage colorImage;
    
    // osc
    ofxOscSender sender;
    string oscDestination, oscAddress;
    int oscPort;
    
    // gui
    ofxPanel gui;
    ofxToggle bRunning;
    ofxButton bAdd, bTrain, bClassify, bSave, bLoad;
    ofParameter<float> minArea, maxArea, threshold;
    ofParameter<int> nDilate;
    ofParameter<int> trainingLabel;
    
    // training
    ThreadedCcv ccv;
    map<int, FoundSquare*> foundSquares;
    bool isTrained, toAddSamples, toClassify;
    bool flipH = true;
    int nAugment = 4;
    float maxAng = 20;
    
    // interface
    DrawGui drawer;
    bool debug;
    float debugScrollY;
    AppButton bClear;
    int width, height;
    
    // music stuff
    ofxSequencer sequencer;
    bool toUpdateSound;
    ofParameter<int> drumController;
    ofParameter<int> bassController;
    ofParameter<int> pianoController;
    ofParameter<int> saxController;
    int nInstruments;
    
    // SoundPlayer
    ofEvent<int> beatEvent;
    string displayString;
    vector <ofSoundPlayer>  drum;
    vector <ofSoundPlayer>  bass;
    vector <ofSoundPlayer>  piano;
    vector <ofSoundPlayer>  sax;
    vector<int> instrumentCountPrev, instrumentCount;
    vector<float> instrumentAreaPrev, instrumentArea;
    int numSamples;
    int nScreenshots;
    int lastDrumController, lastBassController, lastPianoController, lastSaxController = 0;
    long sequencerCount = 0;
    int cols;
    int rows;
};



