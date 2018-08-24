#include "DrawGui.h"


void DrawGui::setup(int width, int height) {
    x0 = 10;
    y0 = 100;
    
    ofFbo::Settings settings;
    settings.useStencil = true;
    settings.height = height;
    settings.width = width;
    settings.internalformat = GL_RGB; //GL_RGBA32F_ARB;
    settings.numSamples = 1;
    
    canvas.allocate(settings);
    //canvas.allocate(width, height);
    
    clear();
    
    changed = false;
    toClassify = false;
    hasReleased = false;
}

void DrawGui::update() {
    if (hasReleased) {
        float waitingTime = ofGetElapsedTimef()-releaseT;
        if (waitingTime > timeOut) {
            toClassify = true;
            points.clear();
            hasReleased = false;
        }
    }
}
        
void DrawGui::updateCanvas() {
    canvas.begin();
    
    ofSetColor(0);
    ofSetLineWidth(4);
    ofBeginShape();
    ofNoFill();
    for (int p=0; p<points.size(); p+=5) {
        ofCurveVertex(points[p].x, points[p].y);
    }
    ofEndShape();
    
    canvas.end();
}

//--------------------------------------------------------------
void DrawGui::clear(){
    
    ofPushMatrix();
    ofPushStyle();
    
    canvas.begin();
    
    ofFill();
    ofSetColor(255);
    ofDrawRectangle(0, 0, canvas.getWidth(), canvas.getHeight());
    
    canvas.end();

    ofPopMatrix();
    ofPopStyle();
    
    changed = true;
}

//--------------------------------------------------------------
void DrawGui::draw(){
    ofPushMatrix();
    ofPushStyle();
    
    ofSetColor(255);
    canvas.draw(x0, y0);

//
//    ofPushMatrix();
//    ofTranslate(100, 100);
//    ofSetColor(0);
//    ofSetLineWidth(8);
//    ofBeginShape();
//    ofNoFill();
////    for (int p=0; p<points.size(); p+=10) {
////        ofCurveVertex(points[p].x, points[p].y);
////    }
//    ofEndShape();
//    ofPopMatrix();
    
    ofPopMatrix();
    ofPopStyle();
    
}


//--------------------------------------------------------------
void DrawGui::mouseDragged(int x, int y){
    float x1 = ofGetPreviousMouseX()-x0;
    float y1 = ofGetPreviousMouseY()-y0;
    float x2 = ofGetMouseX()-x0;
    float y2 = ofGetMouseY()-y0;
    
    releaseT = ofGetElapsedTimef();
    
    // when should this change
    changed = true;
    
    
    canvas.begin();
    
    ofSetColor(0);
    //ofSetLineWidth(8);
    ofSetLineWidth(7);
    ofBeginShape();
    ofNoFill();
    ofDrawLine(x1, y1, x2, y2);
    ofEndShape();
    
    canvas.end();
    
    //points.push_back(ofVec2f(x2, y2));
}

//--------------------------------------------------------------
bool DrawGui::isFrameNew() {
    if (changed) {
        changed = false;
        return true;
    } else {
        return false;
    }
}

//--------------------------------------------------------------
bool DrawGui::isShapeNew() {
    if (toClassify) {
        toClassify = false;
        return true;
    } else {
        return false;
    }
}

//--------------------------------------------------------------
void DrawGui::mousePressed(int x, int y){
    
}

//--------------------------------------------------------------
void DrawGui::mouseReleased(int x, int y){
    updateCanvas();
    releaseT = ofGetElapsedTimef();
    hasReleased = true;
}
