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
void ofApp::setup() {
    //ofSetWindowShape(1600, 900);
    ofSetFullscreen(true);
    
    width = 960;
    height = 720;
    
    debug = false;
    nInstruments = 4;
    toUpdateSound = false;

    drawer.setup(width, height);
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
    gui.add(trainingLabel.set("Training Label", 0, 0, classNames.size()-1));
    gui.add(bAdd.setup("Add samples"));
    gui.add(bTrain.setup("Train"));
    gui.add(bRunning.setup("Run", false));
    gui.add(bClassify.setup("Classify"));
    gui.add(bSave.setup("Save"));
    gui.add(bLoad.setup("Load"));
    gui.add(gCv);
    gui.setPosition(0, 400);
    gui.loadFromFile("cv_settings.xml");
    
    fbo.allocate(width, height);
    colorImage.allocate(width, height);
    grayImage.allocate(width, height);
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

    setupAudio();
    load();
}

//--------------------------------------------------------------
void ofApp::setupAudio() {
    cols = 1;
    rows = 4;
    
    sequencer.setup(cols, 120/8, rows); //columns, bpm, rows
    sequencer.setBpm(120/8);
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
        drum[i].load("exported_clips_4sec/drum_" + ofToString(i) + ".wav");
        drum[i].setVolume(0.75);
        
        bass[i].load("exported_clips_4sec/bass_" + ofToString(i) + ".wav");
        bass[i].setVolume(0.75);
        
        piano[i].load("exported_clips_4sec/piano_" + ofToString(i) + ".wav");
        piano[i].setVolume(0.75);
        
        sax[i].load("exported_clips_4sec/sax_" + ofToString(i) + ".wav");
        sax[i].setVolume(0.75);
    }
}

//--------------------------------------------------------------
void ofApp::beatsIn(int & eventInt){
    toUpdateSound = true;
}

//--------------------------------------------------------------
void ofApp::update(){
    ofSoundUpdate();

    if (toUpdateSound) {
        toUpdateSound = false;
        if (drumController > 0 ) {
            drum[drumController].play();
        }
        if (bassController > 0 ) {
            bass[bassController].play();
        }
        if (pianoController > 0 ) {
            piano[pianoController].play();
        }
        if (saxController > 0 ) {
            sax[saxController].play();
        }
    }
    
    if(drawer.isFrameNew())
    {
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
        cout << " contours " << contourFinder.size() << endl;
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

    if (toAddSamples) {
        addSamplesToTrainingSet();
        toAddSamples = false;
    }
    
    if (isTrained && bRunning && (drawer.isShapeNew() || toClassify)) {
        classifyCurrentSamples();
        toClassify = false;
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(70);

    if (debug) {
        drawDebug();
    } else {
        drawPresent();
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
    ofTranslate(0, 20);
    fbo.draw(0, 0, pWidth, pHeight);
    ofSetColor(0, 255, 0);
    ofPushMatrix();
    ofScale(float(pWidth) / width, float(pHeight) / height);
    contourFinder2.draw();
    ofPopMatrix();
    ofDrawBitmapStringHighlight("merged", 0, 0);
    ofPopMatrix();
    ofPopStyle();
    
    // draw tiles
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, pHeight+60);
    for (int i=0; i<foundSquares.size(); i++) {
        ofPushMatrix();
        //ofTranslate(226*i, 0);
        ofTranslate(226*(i%2), 226*int(i/2));
        foundSquares[i].draw();
        ofPopMatrix();
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
    ofTranslate(210, pHeight+60);
    for (int i=0; i<foundSquares.size(); i++) {
        ofPushMatrix();
        ofTranslate(226*i, 0);
        foundSquares[i].draw();
        ofPopMatrix();
    }
    ofPopMatrix();
    ofPopStyle();
    
    ofPopMatrix();
    ofPopStyle();
    
    //sequencer.draw();
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
        ofPixels pix;
        drawer.getCanvas().readToPixels(pix);        
        fs.img.setFromPixels(pix);        
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
    ofLog(OF_LOG_NOTICE, "Classifiying on frame "+ofToString(ofGetFrameNum()));
    
    instrumentCount.clear();
    instrumentArea.clear();
    
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
    
    
    ofLog() << "instrumnt count ";
    cout << ofToString(instrumentCount) << endl;
    
    //Send OSC messages to Ableton via liveOSC commands
    if (isInstrumentCountDifferent) {
        playbackChange();
        //sendOSC();
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
        //if (instrumentCount[i] > 0) {
            sequencer.setValue(i, 0, instrumentCount[i]);
        //}
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


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key=='1') {
        debug = !debug;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    if (debug) return;
    drawer.mouseDragged(x, y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (debug) return;
    drawer.mousePressed(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    if (debug) return;
    drawer.mouseReleased(x, y);
}