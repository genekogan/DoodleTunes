#include "ofMain.h"

// todo
// - when mouse released, "save last" for undo
// - smoother lines
// - better interface

class DrawGui {
public:
    void setup(int width, int height);
    void update();
    void updateCanvas();
    void draw();
    void clear();
    ofFbo & getCanvas() {return canvas;}
    bool isFrameNew();
    bool isShapeNew();
    
    void mouseDragged(int x, int y);
    void mousePressed(int x, int y);
    void mouseReleased(int x, int y);
    
    vector<ofVec2f> points;
    ofFbo canvas;
    bool changed;
    bool toClassify;
    
    int x0, y0;
    
    ofParameter<float> timeOut;
    float releaseT;
    bool hasReleased;
};
