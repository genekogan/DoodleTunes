#include "AppButton.h"

#include "ofxCanvasButton.h"

//--------------------------------------------------------------
AppButton::AppButton() {
    isHover = false;
    isPressed = false;
    fontSize = 36;
}

//--------------------------------------------------------------
void AppButton::setup(string msg, int x, int y, int w, int h, int fontSize) {
    this->msg = msg;
    this->fontSize = fontSize;
    button = ofRectangle(x, y, w, h);
    font.load("verdana.ttf", fontSize);
}

//--------------------------------------------------------------
void AppButton::mouseMoved(int x, int y){
    isHover = button.inside(x, y);
}

//--------------------------------------------------------------
void AppButton::mouseDragged(int x, int y){
    
}

//--------------------------------------------------------------
void AppButton::mousePressed(int x, int y){
    if (isHover) {
        isPressed = true;
    }
}

//--------------------------------------------------------------
void AppButton::mouseReleased(int x, int y){
    if (isPressed) {
        isPressed = false;
    }
}

//--------------------------------------------------------------
void ofxCanvasButton::mouseReleased(int x, int y){
    if (isPressed) {
        isPressed = false;
        buttonClicked();
    }
}

//--------------------------------------------------------------
void AppButton::draw(){
    ofPushStyle();
    if (isPressed) {
        ofSetColor(ofColor::red);
    } else if (isHover) {
        ofSetColor(ofColor::orange);
    } else {
        ofSetColor(ofColor::white);
    }
    ofFill();
    ofDrawRectangle(button);
    ofSetColor(0);
    
    //ofDrawBitmapString(msg, button.x, button.y+20);
    font.drawString(msg, button.x, button.y+button.height/2+fontSize/2);
    ofPopStyle();
}

//--------------------------------------------------------------
void AppButton::buttonClicked() {
    static ofxCanvasButtonEvent newEvent;
    ofNotifyEvent(ofxCanvasButtonEvent::events, newEvent);
}

