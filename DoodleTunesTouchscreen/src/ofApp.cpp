#include "ofApp.h"

//--------------------------------------------------------------
FoundSquare::FoundSquare() {
    active = false;
    isPrediction = false;
    img.allocate(224, 244, OF_IMAGE_COLOR);
    prev.allocate(224, 224, OF_IMAGE_COLOR);
    diff.allocate(224, 224, OF_IMAGE_COLOR);
}

//--------------------------------------------------------------
void FoundSquare::draw(int w, int h, bool classLabel, bool metaLabel) {
    borderColor = ofLerp(borderColor, 255, 0.033);
    string labelStr = "no class";
    labelStr = ofToString(label) + " (id: "+ofToString(trackerLabel)+", diff "+ofToString(change)+") act"+ofToString(active?"1":"0");
    img.draw(0, 0, w, h);
    if (borderColor < 254) {
        ofPushStyle();
        ofSetLineWidth(8);
        ofNoFill();
        ofSetColor(borderColor, 255, borderColor);
        ofDrawRectangle(0, 0, w, h);
        ofPopStyle();
    }
    if (classLabel) {
        ofDrawBitmapStringHighlight(labelStr, 4, h);
    }
    if (metaLabel) {
        ofDrawBitmapStringHighlight("{"+ofToString(rect.x)+","+ofToString(rect.y)+","+ofToString(rect.width)+","+ofToString(rect.height)+"}, area="+ofToString(area), 4, h+20);
    }
}

//--------------------------------------------------------------
void FoundSquare::updateImage(ofPixels & pix, float cx, float cy, float cw, float ch) {
    if (img.isAllocated()) {
        prev.setFromPixels(img.getPixels());
        img.setFromPixels(pix);
        img.crop(cx, cy, cw, ch);
        img.resize(224, 224);
        absdiff(img, prev, diff);
        diff.update();
        cv::Scalar diffMean = mean(toCv(diff));
        change = diffMean[0] + diffMean[1] + diffMean[2] + diffMean[3];
    } else {
        img.setFromPixels(pix);
        img.crop(cx, cy, cw, ch);
        img.resize(224, 224);
    }
}

//--------------------------------------------------------------
void ofApp::setup() {
//    ofSetWindowShape(1920, 1080);
    ofSetFullscreen(true);
    
    width = 1280;
    height = 900;
    
    debug = false;
    nInstruments = 4;
    toUpdateSound = false;

    // setup drawer and ccv
    drawer.setup(width, height);
    ccv.setup(ofToDataPath("image-net-2012.sqlite3"), &foundSquares);
    
    // events
    bAdd.addListener(this, &ofApp::addSamplesToTrainingSetNext);
    bTrain.addListener(this, &ofApp::trainClassifier);
    bClassify.addListener(this, &ofApp::classifyNext);
    bSave.addListener(this, &ofApp::save);
    bLoad.addListener(this, &ofApp::load);
    trainingLabel.addListener(this, &ofApp::setTrainingLabel);
    
    // osc settings
    oscDestination = DEFAULT_OSC_DESTINATION;
    oscAddress = DEFAULT_OSC_ADDRESS;
    oscPort = DEFAULT_OSC_PORT;
    if (OSC_ENABLED) {
        sender.setup(oscDestination, oscPort);
    }
    
    // setup gui
    gui.setup();
    gui.setName("DoodleClassifier");
    ofParameterGroup gCv;
    gCv.setName("CV initial");
    gCv.add(minArea.set("Min area", 10, 1, 100));
    gCv.add(maxArea.set("Max area", 200, 1, 500));
    gCv.add(threshold.set("Threshold", 128, 0, 255));
    gCv.add(nDilate.set("Dilations", 1, 0, 8));
    gCv.add(drawer.timeOut.set("timeOut", 0.5, 0.001, 2.0));
    gui.add(trainingLabel.set("Training Label", 0, 0, classNames.size()-1));
    gui.add(nAugment.set("augment N", 4, 0, 10));
    gui.add(maxAng.set("augmentation maxAng", 20, 0, 45));
    gui.add(bAdd.setup("Add samples"));
    gui.add(bTrain.setup("Train"));
    gui.add(bRunning.setup("Run", false));
    gui.add(bClassify.setup("Classify"));
    gui.add(bSave.setup("Save"));
    gui.add(bLoad.setup("Load"));
    gui.add(gCv);
    gui.setPosition(0, 560);
    gui.loadFromFile("cv_settings.xml");
    bRunning = false;
    
    // cv stuff
    fbo.allocate(width, height);
    colorImage.allocate(width, height);
    grayImage.allocate(width, height);
    isTrained = false;
    toAddSamples = false;
    toClassify = false;

    // instruments
    for (int i = 0; i< nInstruments; i++) {
        instrumentCountPrev.push_back(0);
        instrumentAreaPrev.push_back(0);
        instrumentCount.push_back(0);
        instrumentArea.push_back(0);
    }
    
    // setup clear button
    bClear.setup("Clear", 10, 10, 200, 64, 36);
    ofAddListener(AppButton::buttonClickedEvent, this, &ofApp::clearButtonClicked);

    // initializ
    setupAudio();
//    load();
}

//--------------------------------------------------------------
void ofApp::setupAudio() {
    cols = 1;
    rows = 4;
    
    sequencer.setup(cols, 120/4, rows); //columns, bpm, rows
    sequencer.setMouseActive(false);
    
    drumController.set("drum", 0, 0, 4);
    bassController.set("bass", 0, 0, 4);
    pianoController.set("piano", 0, 0, 4);
    saxController.set("sax", 0, 0, 4);
    
    sequencer.addRow(&drumController);
    sequencer.addRow(&bassController);
    sequencer.addRow(&saxController);
    sequencer.addRow(&pianoController);
    
    sequencer.setSmooth(true);
    sequencer.start();
    sequencer.setPosition(0, 0, 200, 200);
    displayString = "No beats yet";
    ofAddListener(sequencer.beatEvent, this, &ofApp::beatsIn);
    
    numSamples = 5;
    
    drum.resize(numSamples);
    bass.resize(numSamples);
    piano.resize(numSamples);
    sax.resize(numSamples);
    
    for (int i = 0; i< numSamples; i++) {
        drum[i].load("sounds/drum_" + ofToString(i) + ".wav");
        drum[i].setVolume(0.75);
        
        bass[i].load("sounds/bass_" + ofToString(i) + ".wav");
        bass[i].setVolume(0.75);
        
        piano[i].load("sounds/piano_" + ofToString(i) + ".wav");
        piano[i].setVolume(0.75);
        
        sax[i].load("sounds/sax_" + ofToString(i) + ".wav");
        sax[i].setVolume(0.75);
    }
}

//--------------------------------------------------------------
void ofApp::clearButtonClicked(){
    clearDrawer();
}

//--------------------------------------------------------------
void ofApp::beatsIn(int & eventInt){
    toUpdateSound = true;
}

//--------------------------------------------------------------
void ofApp::update(){
    clearDeadSquares();
    
    ofSoundUpdate();
    ccv.update();
    drawer.update();
    
    if(drawer.isFrameNew()){
        updateCv();
//        if (!ccv.getIsPredicting()) {
//            clearDeadSquares();
//        }
    }
    
    // add to training set
    if (toAddSamples) {
        gatherFoundSquares();
        addSamplesToTrainingSet();
        toAddSamples = false;
    }
    
    // update instruments
    if (ccv.getHasResults()){
        updateInstruments();
        ccv.resetResults();
    }

    // classify next
    if (bRunning && ccv.getIsTrained() && !ccv.getIsPredicting() && drawer.isShapeNew()) {
        gatherFoundSquares();
        classifyCurrentSamples();
        toClassify = false;
    }

    // update audio
    if (toUpdateSound) {
        toUpdateSound = false;
        updateAudio();
    }
}

//--------------------------------------------------------------
void ofApp::clearDeadSquares(){
    for (map<int, FoundSquare*>::iterator it = foundSquares.begin(); it != foundSquares.end(); ) {
        if (!it->second->active) {
            delete it->second;
            foundSquares.erase(it++);
        } else {
            ++it;
        }
    }
}

//--------------------------------------------------------------
void ofApp::updateCv(){
    // get grayscale image and threshold
    ofPixels pix;
    drawer.getCanvas().readToPixels(pix);
    colorImage.setFromPixels(pix);
    grayImage.setFromColorImage(colorImage);
    for (int i=0; i<nDilate; i++) {
        grayImage.erode_3x3();
    }
    grayImage.threshold(threshold);
    grayImage.invert();
    
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
        ofPushStyle();
        ofPopStyle();
        ofEndShape(OF_CLOSE);
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
}


//--------------------------------------------------------------
void ofApp::updateAudio(){
    
    drumController = min(int(drumController),numSamples-1);
    bassController = min(int(bassController),numSamples-1);
    pianoController = min(int(pianoController),numSamples-1);
    saxController = min(int(saxController),numSamples-1);
    
    /////
    //Drum
    if (drumController != lastDrumController) { //If there is a change in the occurances of this instrument
        for (int i = 0; i< numSamples; i++) {
            drum[i].setVolume(0); //Turn down all existing samples (since they might need to be stopped midway)
        }
        if (drumController > 0 ) { //Turn up and play the appropriate sample
            drum[drumController].play();
            drum[drumController].setVolume(0.75);
        }
        
    } else if (sequencerCount%2 == 0) { //If there is no change in the number of occurances of this instrument and we hit an even beat
        if (drumController > 0 ) {
            drum[drumController].play(); //re-play the appropriate sample
        }
    }
    
    //Bass
    if (bassController != lastBassController) {
        for (int i = 0; i< numSamples; i++) {
            bass[i].setVolume(0);
        }
        if (bassController > 0 ) {
            bass[bassController].play();
            bass[bassController].setVolume(0.75);
        }
        
    } else if (sequencerCount%2 == 0) {
        if (bassController > 0 ) {
            bass[bassController].play();
        }
    }
    
    //Piano
    if (pianoController != lastPianoController) {
        for (int i = 0; i< numSamples; i++) {
            piano[i].setVolume(0);
        }
        if (pianoController > 0 ) {
            piano[pianoController].play();
            piano[pianoController].setVolume(0.75);
        }
        
    } else if (sequencerCount%2 == 0) {
        if (pianoController > 0 ) {
            piano[min(int(pianoController), numSamples-1)].play();
        }
    }
    
    //Sax
    if (saxController != lastSaxController) {
        for (int i = 0; i< numSamples; i++) {
            sax[i].setVolume(0);
        }
        if (saxController > 0 ) {
            sax[saxController].play();
            sax[saxController].setVolume(0.75);
        }
        
    } else if (sequencerCount%2 == 0) {
        if (saxController > 0 ) {
            sax[saxController].play();
        }
    }
    
    lastDrumController = drumController;
    lastBassController = bassController;
    lastPianoController = pianoController;
    lastSaxController = saxController;
    
    sequencerCount++;
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(70);
    if (debug) {
        drawDebug();
    } else {
        drawPresent();
        bClear.draw();
    }
}

//--------------------------------------------------------------
void ofApp::drawPresent(){
    int pWidth = ofGetWidth()-(width + 30);
    int pHeight = height * pWidth / width;
    
    drawer.draw();
    
    ofPushMatrix();
    ofPushStyle();

    ofTranslate(width + 20, 0);
    
    // merged
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, 10);
    fbo.draw(0, 0, pWidth, pHeight);
    ofSetColor(0, 255, 0);
    ofPushMatrix();
    ofScale(float(pWidth) / width, float(pHeight) / height);
    contourFinder2.draw();
    ofPopMatrix();
    
    ofPopMatrix();
    ofPopStyle();
    
    // draw tiles
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, pHeight+20);
    
    //int n = foundSquares.size();
    int n = foundSquares.size();
    //float w = 290;
    float w = (ofGetWidth() - width - 20) / 2.0 - 2*5;
    int nc = 2;
    if (n > 4) {
        //w = 195;
        w = (ofGetWidth() - width - 20) / 3.0 - 3*5;
        nc = 3;
    }
    if (n > 9) {
        //w = 145;
        w = (ofGetWidth() - width - 20) / 4.0 - 4*5;
        nc = 4;
    }
    if (n > 16) {
        //w = 115;
        w = (ofGetWidth() - width - 20) / 5.0 - 5*5;
        nc = 5;
    }

    int i = 0;
    for (map<int, FoundSquare*>::iterator it = foundSquares.begin(); it != foundSquares.end(); it++) {
        ofPushMatrix();
        ofTranslate((w+6) * (i%nc), (w+16)*int(i / nc));
        it->second->draw(w, w, true, false);
        ofPopMatrix();
        i++;
    }
    
    ofPopMatrix();
    ofPopStyle();

    ofPopMatrix();
    ofPopStyle();
    
}

//--------------------------------------------------------------
void ofApp::drawDebug(){
    
    int pWidth = 0.32 * ofGetWidth();
    int pHeight = height * pWidth / width;
    
    ofPushMatrix();
    ofPushStyle();
    
    gui.draw();
    
    ofPushMatrix();
    //ofScale(0.75, 0.75);
    
    // original
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, 20);
    //cam.draw(0, 0);
    drawer.getCanvas().draw(0, 0, pWidth, pHeight);
    
    ofDrawBitmapStringHighlight("original", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    // thresholded
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0.333*ofGetWidth(), 20);
    grayImage.draw(0, 0, pWidth, pHeight);
    ofSetColor(0, 255, 0);
    ofPushMatrix();
    ofScale(float(pWidth) / width, float(pHeight) / height);
    contourFinder.draw();
    ofPopMatrix();
    ofDrawBitmapStringHighlight("thresholded", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    // merged
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0.667*ofGetWidth(), 20);
    fbo.draw(0, 0, pWidth, pHeight);
    ofSetColor(0, 255, 0);
    ofPushMatrix();
    ofScale(float(pWidth) / width, float(pHeight) / height);
    contourFinder2.draw();
    ofPopMatrix();
    ofDrawBitmapStringHighlight("merged", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    ofPopMatrix();
    
    // draw tiles
    ofPushMatrix();
    ofPushStyle();
    
    float ty = pHeight+60 - debugScrollY;
    ofTranslate(210, ty);
    
    float dispW = ofGetWidth() - 210;
    float dispH = ofGetHeight() - (pHeight+60);
    
    float w = 226;
    int nx = int(dispW / w);
    
    int i=0;
    for (map<int, FoundSquare*>::iterator it = foundSquares.begin(); it != foundSquares.end(); it++) {
        float x = w * (i % nx);
        float y = w * int(i/nx);
        
        ofPushMatrix();
        ofTranslate(x, y);
        it->second->draw(int(w-2), int(w-2), bRunning, false);
        ofPopMatrix();
        i++;
    }
    
    ofPopMatrix();
    ofPopStyle();
    
    ofPopMatrix();
    ofPopStyle();
    
    //sequencer.draw();
}

//--------------------------------------------------------------
void ofApp::takeScreenshot() {
    ofPixels pix;
    drawer.getCanvas().readToPixels(pix);
    ofImage img;
    img.setFromPixels(pix);
    img.save("training_screenshots/screenshot"+ofToString(nScreenshots)+".png");
    nScreenshots++;
}

//--------------------------------------------------------------
void ofApp::exit() {
    gui.saveToFile("cv_settings.xml");
}

//--------------------------------------------------------------
void ofApp::gatherFoundSquares() {
    if (ccv.getIsPredicting()) {
        return;
    }
    debugScrollY = 0;
    
    // reset activeness until found again
    for (map<int, FoundSquare*>::iterator it = foundSquares.begin(); it != foundSquares.end(); it++) {
        it->second->active = false;
    }
    
    // copy global drawer canvas
    ofPixels pix;
    drawer.getCanvas().readToPixels(pix);

    // process contours
    RectTracker& tracker = contourFinder2.getTracker();
    for (int i=0; i<contourFinder2.size(); i++) {
        FoundSquare *fs;
        int trackerLabel = contourFinder2.getLabel(i);

        if (foundSquares.find(trackerLabel) == foundSquares.end()) {
            fs = new FoundSquare();
            foundSquares[trackerLabel] = fs;
        } else {
            fs = foundSquares[trackerLabel];
        }
        
        fs->active = true;
        fs->trackerLabel = trackerLabel;
        fs->rect = contourFinder2.getBoundingRect(i);
        fs->area = contourFinder2.getContourArea(i);
        
        if (!bRunning || !ccv.getIsTrained()) {
            fs->idxLabel = trainingLabel;
            fs->label = classNames[trainingLabel];
        }
        
        float cx = max(0.0f, (float) fs->rect.x-5.0f);
        float cy = max(0.0f, (float) fs->rect.y-5.0f);
        float cw = min(pix.getWidth()-1.0f-cx, (float) fs->rect.width+10.0f);
        float ch = min(pix.getHeight()-1.0f-cy, (float) fs->rect.height+10.0f);

        fs->updateImage(pix, cx, cy, cw, ch);
    }
}

//--------------------------------------------------------------
void ofApp::addSamplesToTrainingSet() {
    ofLog(OF_LOG_NOTICE, "Adding samples...");
    //gatherFoundSquares();
    ccv.addSamples(nAugment, maxAng, flipH);
    takeScreenshot();
}

//--------------------------------------------------------------
void ofApp::trainClassifier() {
    ccv.trainClassifier();
}

//--------------------------------------------------------------
void ofApp::classifyCurrentSamples() {
    ofLog(OF_LOG_NOTICE, "Classifiying on frame "+ofToString(ofGetFrameNum()));
    ccv.predict();
}

//--------------------------------------------------------------
void ofApp::updateInstruments() {
    float maxArea = 0.0;
    
    for (int i = 0; i< nInstruments; i++) {
        instrumentCount[i] = 0;
        instrumentArea[i] = 0;
    }
    
    // count instruments found
    for (map<int, FoundSquare*>::iterator it = foundSquares.begin(); it != foundSquares.end(); it++) {
        int label = it->second->idxLabel;
        instrumentCount[label]++;
        instrumentArea[label] = max(instrumentArea[label], it->second->area);
        maxArea = max((float) maxArea, (float) it->second->area);
    }
    
    // check for instrument count changes
    bool isInstrumentCountDifferent = false;
    for (int i = 0; i< nInstruments; i++) {
        if (instrumentCount[i] != instrumentCountPrev[i]) {
            isInstrumentCountDifferent = true;
        }
    }
    instrumentCountPrev = instrumentCount;
    instrumentAreaPrev = instrumentArea;
    
    ofLog() << "instrument count " << ofToString(instrumentCount);
    
    //Send OSC messages to Ableton via liveOSC commands
    if (isInstrumentCountDifferent) {
        playbackChange();
        // only send osc messages if instrument count varies
        if (OSC_ENABLED){
            sendOSC();
        }
    }
}

//--------------------------------------------------------------
void ofApp::playbackChange() {
//    sequencer.setValue<int>(0, 0, 1); //drum
//    sequencer.setValue<int>(1, 0, 0); //bass
//    sequencer.setValue<int>(2, 0, 2); //piano
//    sequencer.setValue<int>(3, 0, 1); //sax

    ofLog() << "playback change ";
    for (int i = 0; i<nInstruments; i++) {
        sequencer.setValue(i, 0, instrumentCount[i]);
    }
}

//--------------------------------------------------------------
void ofApp::sendOSC() {
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

//--------------------------------------------------------------
void ofApp::setTrainingLabel(int & label_) {
    trainingLabel.setName(classNames[label_]);
}

//--------------------------------------------------------------
void ofApp::save() {
    ccv.save();
}

//--------------------------------------------------------------
void ofApp::load() {
    ccv.load();
    bRunning = true;
}

//--------------------------------------------------------------
void ofApp::classifyNext() {
    toClassify = true;
}

//--------------------------------------------------------------
void ofApp::addSamplesToTrainingSetNext() {
    toAddSamples = true;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key=='1') {
        debug = !debug;
    } else if (key=='2') {
        clearDrawer();
    }
}

//--------------------------------------------------------------
void ofApp::clearDrawer(){
    drawer.clear();
    updateCv();
    gatherFoundSquares();
    clearDeadSquares();
    updateInstruments();
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
    bClear.mouseMoved(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    if (debug) return;
    drawer.mouseDragged(x, y);
    bClear.mouseDragged(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){
    if (debug) {
        debugScrollY = ofClamp(debugScrollY - 10*scrollY, -2000, 2000);
        return;
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (debug) return;
    drawer.mousePressed(x, y);
    bClear.mousePressed(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    if (debug) return;
    drawer.mouseReleased(x, y);
    bClear.mouseReleased(x, y);
}
