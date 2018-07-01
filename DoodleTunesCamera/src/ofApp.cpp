#include "ofApp.h"


//--------------------------------------------------------------
void FoundSquare::draw() {
    img.draw(0, 0);
    string labelStr = "no class";
    labelStr = (isPrediction?"predicted: ":"assigned: ")+label;
    ofDrawBitmapStringHighlight(labelStr, 4, img.getHeight()-22);
    ofDrawBitmapStringHighlight("{"+ofToString(rect.x)+","+ofToString(rect.y)+","+ofToString(rect.width)+","+ofToString(rect.height)+"}, area="+ofToString(area), 4, img.getHeight()-5);
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowShape(1600, 900);
    
    width = 640;
    height = 480;
    nInstruments = 4;
    
    cam.setDeviceID(0);
    cam.setup(width, height);
    ccv.setup(ofToDataPath("image-net-2012.sqlite3"));
    
    bAdd.addListener(this, &ofApp::addSamplesToTrainingSetNext);
    bTrain.addListener(this, &ofApp::trainClassifier);
    bClassify.addListener(this, &ofApp::classifyNext);
    bSave.addListener(this, &ofApp::save);
    bLoad.addListener(this, &ofApp::load);
    trainingLabel.addListener(this, &ofApp::setTrainingLabel);
    
    // default settings
    oscDestination = DEFAULT_OSC_DESTINATION;
    oscAddress = DEFAULT_OSC_ADDRESS;
    oscPort = DEFAULT_OSC_PORT;
    sender.setup(oscDestination, oscPort);
    
    gui.setup();
    gui.setName("DoodleClassifier");
    
    ofParameterGroup gCv;
    gCv.setName("CV initial");
    gCv.add(minArea.set("Min area", 10, 1, 100));
    gCv.add(maxArea.set("Max area", 200, 1, 500));
    gCv.add(threshold.set("Threshold", 128, 0, 255));
    gCv.add(nDilate.set("Dilations", 1, 0, 8));
    
    ofParameterGroup gMotion;
    gMotion.setName("Motion settings");
    gMotion.add(lerpMotion.set("MotionLerp", 0.3, 0, 1));
    gMotion.add(maxMotion.set("MaxMotion", 2, 0, 7));
    gMotion.add(amtMotion.set("Motion", 0, 0, 7));
    gMotion.add(maxColor.set("MaxColor", 10, 0, 20));
    gMotion.add(amtColor.set("Color", 0, 0, 20));
    gMotion.add(minDiff.set("MinDiff", 1, 0, 2));
    gMotion.add(amtDiff.set("Diff", 0, 0, 2));
    
    gui.add(trainingLabel.set("Training Label", 0, 0, classNames.size()-1));
    gui.add(bAdd.setup("Add samples"));
    gui.add(bTrain.setup("Train"));
    gui.add(bRunning.setup("Run", false));
    gui.add(bClassify.setup("Classify"));
    gui.add(bSave.setup("Save"));
    gui.add(bLoad.setup("Load"));
    gui.add(gCv);
    gui.add(gMotion);
    gui.setPosition(0, 400);
    gui.loadFromFile("cv_settings.xml");
    
    fbo.allocate(width, height);
    colorImage.allocate(width, height);
    grayImage.allocate(width, height);
    rImage.allocate(width, height);
    gImage.allocate(width, height);
    bImage.allocate(width, height);
    rgDiff.allocate(width, height);
    rbDiff.allocate(width, height);
    gbDiff.allocate(width, height);
    
    grayBg.allocate(width, height);
    grayDiff.allocate(width, height);
    graySnapshot.allocate(width, height);
    graySnapshotDiff.allocate(width, height);
    graySnapshot = grayImage;
    
    isTrained = false;
    toAddSamples = false;
    toClassify = false;
    
    trainingData.setNumDimensions(4096);
    AdaBoost adaboost;
    adaboost.enableNullRejection(false);
    adaboost.setNullRejectionCoeff(3);
    pipeline.setClassifier(adaboost);
    
    for (int i = 0; i< nInstruments; i++) {
        instrumentCountPrev.push_back(0);
        instrumentAreaPrev.push_back(0);
    }

    load();
    bRunning = true;
}


//--------------------------------------------------------------
void ofApp::update(){
    cam.update();
    if(!cam.isFrameNew()) {
        return;
    }

    // get grayscale image and threshold
    colorImage.setFromPixels(cam.getPixels());
    grayImage.setFromColorImage(colorImage);
    for (int i=0; i<nDilate; i++) {
        grayImage.erode_3x3();
    }
    grayImage.threshold(threshold);
    //grayImage.invert();
    
    // How much color?
    colorImage.convertToGrayscalePlanarImage(rImage, 0);
    colorImage.convertToGrayscalePlanarImage(gImage, 1);
    colorImage.convertToGrayscalePlanarImage(bImage, 2);
    rgDiff.absDiff(rImage, gImage);
    rbDiff.absDiff(rImage, bImage);
    gbDiff.absDiff(gImage, bImage);
    cv::Scalar rgDiffMean = mean(toCv(rgDiff));
    cv::Scalar rbDiffMean = mean(toCv(rbDiff));
    cv::Scalar gbDiffMean = mean(toCv(gbDiff));
    
    float avgC = 0.33333 * (rgDiffMean[0] + rbDiffMean[0] + gbDiffMean[0]);
    //amtColor.set(ofLerp(amtMotion, avgC, lerpMotion));
    amtColor.set(avgC);
    
    // Difference from snapshot
    grayDiff.absDiff(grayBg, grayImage);
    diffMean = mean(toCv(grayDiff));
    float avgM1 = (diffMean[0] + diffMean[1] + diffMean[2]) / 3.0;
    //amtMotion.set(ofLerp(amtMotion, avgM1, lerpMotion));
    amtMotion.set(avgM1);

    // How much motion
    graySnapshotDiff.absDiff(graySnapshot, grayImage);
    diffMean = mean(toCv(graySnapshotDiff));
    float avgM2 = (diffMean[0] + diffMean[1] + diffMean[2]) / 3.0;
    amtDiff.set(ofLerp(amtDiff, avgM2, lerpMotion));
    grayBg = grayImage;
    
    // find initial contours
    contourFinder.setMinAreaRadius(minArea);
    contourFinder.setMaxAreaRadius(maxArea);
    contourFinder.setThreshold(127);
    contourFinder.findContours(grayImage);
    contourFinder.setFindHoles(true);
    
    // draw all contour bounding boxes to FBO
    fbo.begin();
    ofClear(0, 255);
    ofFill();
    ofSetColor(255);
    for (int i=0; i<contourFinder.size(); i++) {
        //cv::Rect rect = contourFinder.getBoundingRect(i);
        //ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);
        ofBeginShape();
        for (auto p : contourFinder.getContour(i)) {
            ofVertex(p.x, p.y);
        }
        ofEndShape();
    }
    fbo.end();
    ofPixels pixels;
    fbo.readToPixels(pixels);
    
    // find merged contours
    contourFinder2.setMinAreaRadius(minArea);
    contourFinder2.setMaxAreaRadius(maxArea);
    contourFinder2.setThreshold(127);
    contourFinder2.findContours(pixels);
    contourFinder2.setFindHoles(false);
    
    // check if conditions are met
    //cout << amtDiff << " > " << minDiff << ", " << amtColor << " > " << maxColor << ", " << amtMotion << " < " << maxMotion << endl;
    if ((amtDiff > minDiff) &&
        (amtColor < maxColor) &&
        (amtMotion < maxMotion)) {
        toClassify = true;
    }
    
    if (toAddSamples) {
        addSamplesToTrainingSet();
        toAddSamples = false;
    }
    else if (isTrained && (bRunning && toClassify)) {
        classifyCurrentSamples();
        toClassify = false;
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(50);
    gui.draw();

    ofPushMatrix();
    ofScale(0.75, 0.75);
    
    // original
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, 20);
    colorImage.draw(0, 0);
    ofDrawBitmapStringHighlight("original", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    // thresholded
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(width, 20);
    grayImage.draw(0, 0);
    ofSetColor(0, 255, 0);
    contourFinder.draw();
    ofDrawBitmapStringHighlight("thresholded", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    // merged
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(2*width, 20);
    fbo.draw(0, 0);
    graySnapshotDiff.draw(0,480 );
    ofSetColor(0, 255, 0);
    contourFinder2.draw();
    ofDrawBitmapStringHighlight("merged", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    ofPopMatrix();
    
    // draw tiles
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(210, 0.75*height+60);
    for (int i=0; i<foundSquares.size(); i++) {
        ofPushMatrix();
        ofTranslate(226*i, 0);
        foundSquares[i].draw();
        ofPopMatrix();
    }
    ofPopMatrix();
    ofPopStyle();
    
    // msg bubble if classified
    ofPushStyle();
    ofFill();
    msgColor = ofLerp(msgColor, 0, 0.01);
    ofSetColor(0, msgColor, 0);
    ofDrawEllipse(ofGetWidth()-20, ofGetHeight() - 20, 50, 50);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::exit() {
    gui.saveToFile("cv_settings.xml");
}

//--------------------------------------------------------------
void ofApp::gatherFoundSquares() {
    foundSquares.clear();
    for (int i=0; i<contourFinder2.size(); i++) {
        FoundSquare fs;
        fs.rect = contourFinder2.getBoundingRect(i);
        fs.area = contourFinder2.getContourArea(i);
        fs.img.setFromPixels(cam.getPixels());
        fs.img.crop(fs.rect.x, fs.rect.y, fs.rect.width, fs.rect.height);
        fs.img.resize(224, 224);
        foundSquares.push_back(fs);
    }
}

//--------------------------------------------------------------
void ofApp::addSamplesToTrainingSet() {
    ofLog(OF_LOG_NOTICE, "Adding samples...");
    gatherFoundSquares();
    for (int i=0; i<foundSquares.size(); i++) {
        foundSquares[i].label = classNames[trainingLabel];
        vector<float> encoding = ccv.encode(foundSquares[i].img, ccv.numLayers()-1);
        VectorFloat inputVector(encoding.size());
        for (int i=0; i<encoding.size(); i++) inputVector[i] = encoding[i];
        trainingData.addSample(trainingLabel, inputVector);
        ofLog(OF_LOG_NOTICE, " Added sample #"+ofToString(i)+" label="+ofToString(trainingLabel));
    }
}

//--------------------------------------------------------------
void ofApp::trainClassifier() {
    ofLog(OF_LOG_NOTICE, "Training...");
    if (pipeline.train(trainingData)){
        ofLog(OF_LOG_NOTICE, "getNumClasses: "+ofToString(pipeline.getNumClasses()));
    }
    isTrained = true;
    ofLog(OF_LOG_NOTICE, "Done training...");
}

//--------------------------------------------------------------
void ofApp::classifyCurrentSamples() {
    msgColor = 255;
    graySnapshot = grayImage;
    amtDiff.set(0);
    
    ofLog(OF_LOG_NOTICE, "Classifiying on frame "+ofToString(ofGetFrameNum()));
    
    vector<int> instrumentCount;
    vector<float> instrumentArea;
    
    for (int i = 0; i< nInstruments; i++) {
        instrumentCount.push_back(0);
        instrumentArea.push_back(0);
    }
    
    ofLog(OF_LOG_NOTICE, "Classifiying "+ofToString(ofGetFrameNum()));
    gatherFoundSquares();
    float maxArea = 0.0;
    for (int i=0; i<foundSquares.size(); i++) {
        vector<float> encoding = ccv.encode(foundSquares[i].img, ccv.numLayers()-1);
        VectorFloat inputVector(encoding.size());
        for (int i=0; i<encoding.size(); i++) inputVector[i] = encoding[i];
        if (pipeline.predict(inputVector)) {
            int label = pipeline.getPredictedClassLabel();
            foundSquares[i].label = classNames[label];
            
            instrumentCount[label]++;
            instrumentArea[label] = max(instrumentArea[label], foundSquares[i].area);
            maxArea = max(maxArea, foundSquares[i].area);
        }
    }
    
    // only send osc messages if instrument count varies
    bool isInstrumentCountDifferent = false;
    for (int i = 0; i< nInstruments; i++) {
        if (instrumentCount[i] != instrumentCountPrev[i]) {
            isInstrumentCountDifferent = true;
        }
    }
    instrumentCountPrev = instrumentCount;
    instrumentAreaPrev = instrumentArea;
    
    //Send OSC messages to Ableton via liveOSC commands
    if (isInstrumentCountDifferent) {
        ofLog() << "send OSC messages ";
        for (int i = 0; i<nInstruments; i++) {
            if (instrumentCount[i] > 0) {
                
                //Launch the clips
                ofxOscMessage m;
                m.setAddress("/live/play/clip");
                m.addIntArg(i); //Set the track
                m.addIntArg(instrumentCount[i]-1); //Set the clip
                sender.sendMessage(m, false);
                
                //Set the track volume based on size of the instruments
                float trackVol = 0.75*instrumentArea[i]/maxArea;
                ofxOscMessage m2;
                m2.setAddress("/live/volume");
                m2.addIntArg(i);
                m2.addFloatArg(ofMap(trackVol,0,0.75,0.55,0.75));
                sender.sendMessage(m2, false);
            }
            else {
                ofxOscMessage m;
                m.setAddress("/live/stop/track");
                m.addIntArg(i); //Set the track
                sender.sendMessage(m, false);
            }
        }
    }
    
}

void ofApp::keyPressed(int key) {
    if (key==' ') {
        classifyCurrentSamples();
    }
}
//--------------------------------------------------------------
void ofApp::setTrainingLabel(int & label_) {
    trainingLabel.setName(classNames[label_]);
}

//--------------------------------------------------------------
void ofApp::save() {
    pipeline.save(ofToDataPath("doodleclassifier_model.grt"));
//    trainingData.save(ofToDataPath("TrainingData.grt"));
}

//--------------------------------------------------------------
void ofApp::load() {
    pipeline.load(ofToDataPath("doodleclassifier_model.grt"));
//    trainingData.load(ofToDataPath("TrainingData.grt"));
//    trainClassifier();
    isTrained = true;
}

//--------------------------------------------------------------
void ofApp::classifyNext() {
    toClassify = true;
}

//--------------------------------------------------------------
void ofApp::addSamplesToTrainingSetNext() {
    toAddSamples = true;
}
