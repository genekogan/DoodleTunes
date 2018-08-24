#pragma once

#include "ofMain.h"
#include "ofxCv.h"


static vector<string> classNames = {
    "drums",
    "bass guitar",
    "saxophone",
    "keyboard"
};


// instrument names
struct FoundSquare {ofImage diff; ofImage prev;float change;
    FoundSquare();
    
    void updateImage(ofPixels & pix, float cx, float cy, float cw, float ch);
    void draw(int w, int h, bool classLabel, bool metaLabel);
    void draw() {draw(img.getWidth(), img.getHeight(), true, true);}

    ofImage img;
    string label;
    int idxLabel;
    int trackerLabel;
    bool active;
    bool isPrediction;
    cv::Rect rect;
    float area;
    float borderColor;
};



