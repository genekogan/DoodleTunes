#pragma once

#include "ofMain.h"

class AppButton {
public:
    AppButton();
    void setup(string msg, int x, int y, int w, int h, int fontSize);
    
    void draw();
    
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y);
    void mousePressed(int x, int y);
    void mouseReleased(int x, int y);
    
    static ofEvent<void> buttonClickedEvent;
    void buttonClicked();
    
    ofRectangle button;
    string msg;
    bool isHover, isPressed;
    ofTrueTypeFont font;
    int fontSize;
};


