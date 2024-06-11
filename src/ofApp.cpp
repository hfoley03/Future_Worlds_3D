
//--------------------------------------------------------------
#include "ofApp.h"
using namespace std;

//--------------------------------------------------------------
void ofApp::setup(){
    ofEnableAlphaBlending();
    ofEnableDepthTest();
//    light.enable();
//    light.setPosition(ofVec3f(100,100,200));
//    light.lookAt(ofVec3f(0,0,0));
 //   myfont.load("fonts/LEDLIGHT.otf", 60);
    ofSetVerticalSync(true);
    ofSetFrameRate(30);

    fontSetup();
    otherPlanetsSetup();
    myGraphCols();
    ofSetBackgroundColor(0);
    loadCSVData();
    normSetup();
    imageToGrid();
    setCellSphereRadius();
    variableSetup();
    lastMinute = ofGetElapsedTimeMillis();
    
    placemarkerImage.load("photos/Cairo/Cairo_neutral_2095.png");

    initCityBools();
    currentCity = "NEW YORK";
    updateImageForYear(worldType);
    loadImageForYear();
    
    startTime = 0;
    float transitionStartTime = 0.0f;
    float transitionDuration = 3.0f;
    fadeValue = 0.0f;

    std::cout << "set up finished"  << std::endl;
    ofSetBackgroundColor(0);
}

//--------------------------------------------------------------
void ofApp::update(){
    rotationAngle += 0.1f;  // for globe spin
    centreH = ofGetHeight()/2;
    centreW = ofGetWidth()/2;
    updatePlanets();
    sendOsc();
    fadeInOut =255.0f * fadeValue;
    //cout << fadeInOut << endl;
    if(currentYear == 199){ start = false;}

    if(start){
        now = ofGetElapsedTimeMillis();
        if (now - lastMinute >= speed) { // 60000 milliseconds = 1 minute
            automaCellulare();
            checkDataDirection();
            updatePlotter();
            
            currentYear += 1;
            lastMinute = now;

            updateImageForYear(worldType);
            loadImageForYear();
        }
    }
    
    float currentTime = ofGetElapsedTimef();
    float timeSinceStart = currentTime - transitionStartTime;
    if (moveToGlobe) {
        fadeValue = ofClamp(timeSinceStart / transitionDuration, 0.0f, 1.0f);
    } else {
        fadeValue = ofClamp(1.0f - (timeSinceStart / 6.0f), 0.0f, 1.0f);
    }
    
    //cout << fadeValue << endl;
    
    ofxOscMessage m5;
    m5.setAddress("/foley");
    m5.addFloatArg(fadeValue);
    oscOut.sendMessage(m5);

    
    if(isDrifting){
        for (int i = 0; i < 80; i++) {
            for (int j = 0; j < 80; j++) {
                if (moveToGlobe) {
                    cells[i][j].driftPos.interpolate(cells[i][j].pos3D, 0.05f);
                } else {
                    cells[i][j].driftPos += cells[i][j].velocity;
                }
            }
        }
    }

    
    float fps = ofGetFrameRate();
    std::string fpsString = "FPS: " + std::to_string(fps);
    ofSetWindowTitle(fpsString);
}

//--------------------------------------------------------------
void ofApp::draw(){
    //worldImage.draw(0,0);
    ofSetColor(255, 255, 255, 255);
//    ofDrawLine(0, centreH, centreW*2, centreH);
//    ofDrawLine(centreW, 0, centreW, centreH*2);


    
    //placemarkerImage.draw(0,0);
    cam.begin();

    drawPlanets();
    earthSpinning();

    cam.end();

  //  cellArrayToImage();
    drawGUI();
    dataPlotter();
    drawDataLegend();
    drawCityImages();
    citySelect();
    

}

void ofApp::myGraphCols(){
    blue.set( 57,96,169);
    purple.set( 152,66,109);
    organge.set( 252,134,88);
    brown.set( 121,104,37);
    greenA.set( 93,173,90);
    greenB.set(129,203,94);
    yello.set(252,251,189);
    
    popColor.setHsb(0, 255, 255);       // Red
    lifeExpColor.setHsb(64, 255, 255);  // Yellow
    ecoFootColor.setHsb(128, 255, 255); // Green
    foodColor.setHsb(192, 255, 255);    // Cyan
    industryColor.setHsb(240, 255, 255);// Blue
    
    popSelected = false;
    lifeExpSelected = false;
    ecoFootSelected = false;
    foodSelected = false;
    industrySelected = false;
}

void ofApp::dataPlotter(){
    plotWidth = imageSize; //ofGetWidth() * 0.3f;
    plotHeight = imageSize; //ofGetHeight() * 0.2f;
    origin.set(ofGetWidth() - plotWidth - 20, plotHeight + 30 + ofGetHeight()/2);

    origin.set(ofGetWidth() - plotWidth - edgeOffset, centreH - plotHeight/2);

    
    if(pollutionPoly.size() > 0){
        
        ofSetColor(popColor, fadeInOut);
        ofDrawCircle(populationPoly.getVertices()[0], 5);
        ofDrawCircle(populationPoly.getVertices().back(), 5);
        populationPoly.draw();

        ofSetColor(lifeExpColor, fadeInOut);
        ofDrawCircle(lifeExpPoly.getVertices()[0], 5);
        ofDrawCircle(lifeExpPoly.getVertices().back(), 5);
        lifeExpPoly.draw();

        ofSetColor(ecoFootColor, fadeInOut);
        pollutionPoly.draw();
        ofDrawCircle(pollutionPoly.getVertices()[0], 5);
        ofDrawCircle(pollutionPoly.getVertices().back(), 5);

        ofSetColor(foodColor, fadeInOut);
        ofDrawCircle(foodPoly.getVertices()[0], 5);
        ofDrawCircle(foodPoly.getVertices().back(), 5);
        foodPoly.draw();

        ofSetColor(industryColor, fadeInOut);
        ofDrawCircle(industryPoly.getVertices()[0], 5);
        ofDrawCircle(industryPoly.getVertices().back(), 5);
        industryPoly.draw();
    }
    ofSetColor(50,50,50, fadeInOut);
    //ofDrawRectRounded(origin.x, centreH + 10 - (plotHeight/2), plotWidth, plotHeight + 30 , 25);
    ofDrawRectangle(origin.x, origin.y, plotWidth, plotHeight);
    ofSetColor(200,200,200, fadeInOut);
    int borderOffset = 10;
    ofDrawRectangle(origin.x - borderOffset, origin.y - borderOffset, plotWidth + borderOffset*2, plotHeight + borderOffset*2);

    //ofDrawRectRounded(origin.x -10, 0, plotWidth + 20, plotHeight + 30  + 20 , 25);
    
}

void ofApp::drawDataLegend(){
    float legendYDim =  10;
    myfont.setLineHeight(legendYDim * 5.0f);
    float stringH =  myfont.getLineHeight() * 0.7f;
    
    float rightMargin = ofGetWidth() - edgeOffset;
    ofVec2f originL;
    originL.set(origin.x + 20 , origin.y + stringH/2 + 300);
    
    //ofSetColor(255, 0, 0);
    //ofDrawCircle(originL, 5);
    
    float textX;
    
    // Population
    textX = rightMargin - popText.getStringBoundingBox("Population", 0, 0).width;
    ofSetColor(popColor, fadeInOut);
    popText.drawString("Population", textX, originL.y + stringH);
    
    // Life Expectancy
    textX = rightMargin - lifeExpText.getStringBoundingBox("Life Expectancy", 0, 0).width;
    ofSetColor(lifeExpColor, fadeInOut);
    lifeExpText.drawString("Life Expectancy", textX, originL.y + stringH*2);

    // Eco Foot Print
    textX = rightMargin - ecoFootText.getStringBoundingBox("Eco Foot Print", 0, 0).width;
    ofSetColor(ecoFootColor, fadeInOut);

    ecoFootText.drawString("Eco Foot Print", textX, originL.y + stringH*3);

    // Food
    textX = rightMargin - foodText.getStringBoundingBox("Food", 0, 0).width;
    ofSetColor(foodColor, fadeInOut);
    foodText.drawString("Food", textX, originL.y + stringH*4);

    // Industry
    textX = rightMargin - industryText.getStringBoundingBox("Industry", 0, 0).width;
    ofSetColor(industryColor, fadeInOut);
    industryText.drawString("Industry", textX, originL.y + stringH*5);

    popRect = popText.getStringBoundingBox("Population", textX, originL.y + stringH);
    lifeExpRect  = lifeExpText.getStringBoundingBox("Life Expectancy", textX, originL.y + stringH*2);
    ecoFootRect  = ecoFootText.getStringBoundingBox("Eco Foot Print", textX, originL.y + stringH*3);
    foodRect  = foodText.getStringBoundingBox("Food", textX, originL.y + stringH*4);
    industryRect  = industryText.getStringBoundingBox("Industry", textX, originL.y + stringH*5);
    
   //ofSetColor(40);
    //ofDrawRectangle(origin.x, origin.y + 20, plotWidth, plotHeight);
}

void ofApp::updatePlotter(){
    ofSetLineWidth(3);
    float xx = ((float)currentYear / 200.0f) * (plotWidth - 40) + origin.x + 20;
    float graphPolyHeight = plotHeight - 20;
    populationPoly.addVertex( xx , origin.y + imageSize  - (populationData[currentYear] * graphPolyHeight + 10));
    pollutionPoly.addVertex( xx , origin.y + imageSize - (ecoFootData[currentYear] * graphPolyHeight + 10));
    foodPoly.addVertex( xx , origin.y + imageSize - (foodData[currentYear] * graphPolyHeight + 10));
    industryPoly.addVertex( xx , origin.y  + imageSize- (industryData[currentYear] * graphPolyHeight + 10));
    lifeExpPoly.addVertex( xx , origin.y + imageSize - (lifeExpData[currentYear] * graphPolyHeight + 10));
    
//    populationPoly.getSmoothed(5, 0.33f);
//    pollutionPoly.getSmoothed(5, 0.33f);
//    foodPoly.getSmoothed(5, 0.33);
//    industryPoly.getSmoothed(5, 0.33f);
//    lifeExpPoly.getSmoothed(5, 0.33f);
    
    
}

void ofApp::earthSpinning(){
    ofPushMatrix();
    
    ofRotateXDeg(80);  // Rotate the sphere 90 degrees around the X-axis
    ofRotateZDeg(rotationAngle);  // Apply the rotation around the Y-axis for spinning effect
    ofSetColor(246,245, 257);
    
    if(checkSpheresInPosition(1.0f)){
        sphere.draw();
    }
        
    cellArrayToImage3D();
    //ofDrawLine(256+20,  ofGetHeight()/2 - 128, 0, cells[40][40].pos3D.x, cells[40][40].pos3D.y, cells[40][40].pos3D.z);

    ofPopMatrix();



}

void ofApp::imageToGrid(){
        
    //worldImage.load("merc1_2.JPG");
    worldImage.load("merc1_2.JPG");
    worldImage.resize(1280/2, 1280/2); //640
    float imageHeight = worldImage.getHeight();
    
   // worldImage.load("merc256~2.jpg");
   // worldImage.resize(1280/2, 1280/2); //640
    
    std::cout << "width and height" << std::endl;
    std::cout << to_string(worldImage.getWidth())  << std::endl;
    std::cout << to_string(worldImage.getHeight())  << std::endl;
    
    r1 = {-1*_PI, _PI};
   // r2 = {0.0, 256.0};
    r2 = {0.0, 640};

    int across = 0;
    int down = 0;
    
    int a = worldImage.getWidth();
    int b = a / 80;
    
    std::cout << a  << std::endl;
    std::cout << b  << std::endl;
    int colorCounter = 0;
    for(int i =b/2; i < a; i = i + b)
    {
        for(int j = b/2; j < a; j = j + b)
        {
            Cell cell;
            cell.pos2D.x = i;
            cell.pos2D.y = j;
            
            // Calculation of longitude and latitude from Map
            double longitude, latitude;
            float S = 128;
            
            latitude = Gudermannian(convertRange(j,r2,r1));  //from 0-640 to -pi - +pi
            longitude = ofRadToDeg(i / R) - 180.0f;

            // conversion to spherical coordinates
            float phi = (90 - (latitude)) * (_PI / 180);
            float theta = (180 + (longitude)) * (_PI / 180);
            cell.pos3D.x = -1* (S * sin(phi) * cos(theta));
            cell.pos3D.y = S * sin(phi) * sin(theta);
            cell.pos3D.z = S * cos(phi);
            
            // random positions and vels for drifitng animiation
            cell.driftPos = ofVec3f(ofRandom(-500, 500), ofRandom(-500, 500), ofRandom(-500, 500));
            cell.velocity = ofVec3f(ofRandom(-1, 1), ofRandom(-1, 1), ofRandom(-1, 1));
            
            ofColor rgbColor =  worldImage.getColor(i,j);
            cell.cellColor = rgbColor;
            cell.initCellcolor = cell.cellColor;

            cell.cellType = classifyCelltype(rgbColor, imageHeight, j);
            cell.initCellType = cell.cellType;

            
            colorCounter++;
            cells[across][down] = cell;
            
//            ofLog() <<  "y = " + to_string(j) + " lat: " << latitude << " degrees";
//            ofLog() <<  "x = " + to_string(i) + " long: " << longitude << " degrees";
            //ofLog() << cell.cellColor;
//            ofLog() << "____";
            
            down++;
        }
        down = 0;
        across++;
    }
    std::cout << across << std::endl;
    std::cout << down << std::endl;

}

void ofApp::cellArrayToImage(){
    for(int i = 0; i < 80; i = i + 1)
    {
        for(int j = 0; j < 80; j = j + 1)
        {
            Cell _c = cells[i][j];
            ofSetColor(_c.cellColor);
//            if(_c.cellType == "ice" || _c.cellType == "ocean" || _c.cellType == "sand"){
            if(_c.cellType == "sand"){

//                ofSetColor(0);
                ofSetColor(_c.cellColor);

            }
            else{
//                ofSetColor(0);

                ofSetColor(_c.cellColor);
            }
            ofDrawCircle(_c.pos2D.x, _c.pos2D.y, 4);
        }
    }
}

void ofApp::cellArrayToImage3D(){
    for(int i = 0; i < 80; i = i + 1)
    {
        for(int j = 0; j < 80; j = j + 1)
        {
            Cell _c = cells[i][j];
            ofSetColor(_c.cellColor);
            //ofDrawSphere(_c.pos3D.x, _c.pos3D.y, _c.pos3D.z, _c.sphereRadius);
            ofDrawSphere(_c.driftPos.x, _c.driftPos.y, _c.driftPos.z, _c.sphereRadius);

        }
    }
}

bool ofApp::checkSpheresInPosition(float threshold) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (cells[i][j].driftPos.distance(cells[i][j].pos3D) > threshold) {
                return false;
            }
        }
    }
    return true;
}

void ofApp::automaCellulare(){
    int nIce = 0;
    int nCity = 0;
    int nOcean = 0;
    int nGrass = 0;
    int nSand = 0;
    int ii;
    int jj;
    int iceCounterTemp = 0;
    sandCounter = 0;
    oceanCounter = 0;
    cityCounter = 0;
    grassCounter = 0;


    Cell _c; // current cell
    
    for(int i = 0; i < 80; i = i + 1)
    {
        for(int j = 0; j < 80; j = j + 1)
        {
            nIce = 0;
            nCity = 0;
            nOcean = 0;
            nGrass = 0;
            nSand = 0;

            _c = cells[i][j]; // current cell of interest
            string currentType = _c.cellType;
            string currentInitType = _c.initCellType;
            
            if(currentType== "ice"){ iceCounterTemp++;}
            else if(currentType == "sand"){ sandCounter++;}
            else if(currentType == "grass"){ grassCounter++;}
            else if(currentType == "ocean"){ oceanCounter++;}
            else if(currentType == "city"){ cityCounter++;}



            //check the cells around me
            for(int xx = -1; xx <= 1; xx = xx + 1)
            {
                for(int yy = -1; yy <= 1; yy = yy + 1)
                {
                    if( j + yy < 0 || j + yy > 79){
                        continue;
                    }
                    
                    
                    if((i + xx)< 0){ ii = 79; }
                    else if (i + xx > 79){ ii = 0; }
                    else { ii = i + xx; }
                    
                    string cellType = cells[ii][j + yy].cellType;
                    if ( cellType == "grass"){nGrass += 1;}
                    else if ( cellType == "ice"){nIce += 1;}
                    else if ( cellType == "sand"){nSand += 1;}
                    else if ( cellType == "ocean"){nOcean += 1;}
                    else if ( cellType == "city"){nCity += 1;}

                }
            }
            float chance = ofRandom(8.0);
            
            // FOOD RULE SAND VS GRASS
            if(true){
                
                if(foodIncreasing){
                    if(currentType == "sand" && nGrass >= 2){
                        if(chance < foodData[currentYear]){
                            //cout << "chance " + ofToString(chance) + " foodLvl " +  ofToString(foodData[currentYear]) << endl;
                            cells[i][j].cellType = "grass";
                            cells[i][j].cellColor = grassColors[(int)ofRandom(7)];
                            //cout << "make grass" << endl;
                            //cout << cells[i][j].cellColor << endl;
                        }
                    }
                }
                
                if(!foodIncreasing){
                    if(currentType == "grass" && currentInitType == "sand"){
                        if(chance < foodData[currentYear]/2   ){
                            cells[i][j].cellType = "sand";
                            cells[i][j].cellColor = sandColors[(int)ofRandom(4)];
                            //cout << "make sand" << endl;
                        }
                    }
                }
            }
            
            // Eco Foot Rule Grass Vs Sea
            if(true){
                if(pollutionIncreasing){
                    if(currentType == "grass" && nOcean >= 4){
                        if(ecoFootData[currentYear] > 0.2){
                            if(chance < ecoFootData[currentYear]){ //chance
                                cells[i][j].cellType = "ocean";
                                cells[i][j].cellColor = seaColors[(int)ofRandom(6)];
                            }
                        }
                    }
                }
                
                if(!pollutionIncreasing){
                    if(currentType == "ocean" && currentInitType == "grass"){
                        if(ecoFootData[currentYear] < 0.4){
                            if(chance < 1.0){ //chance
                                cells[i][j].cellType = "grass";
                                cells[i][j].cellColor = grassColors[(int)ofRandom(7)];
                            }
                        }
                    }
                }
            }
            
            
            if(true){
                if(populationIncreasing){
                    if(currentType == "grass" && nCity >= 1){
                        if(chance < 0.1f){
                            cells[i][j].cellType = "city";
                            cells[i][j].cellColor = ofColor(255,0,0);
                        }
                    }
                }
                if(!populationIncreasing){
                    if(currentType == "city" && nGrass >= 2 && currentInitType == "grass" ){
                        if(chance < 0.1f){
                            cells[i][j].cellType = "grass";
                            cells[i][j].cellColor = grassColors[(int)ofRandom(7)];
                        }
                    }
                }
            }
            
            
            // Eco Foot Rule ice Vs sea
            if(true){
                if(pollutionIncreasing){
                    if(ecoFootData[currentYear] > 0.4){
                        if(currentType == "ice" && nOcean >= 4){
                            if(chance < ecoFootData[currentYear] *2.0f){ //chance chance < ecoFootData[currentYear]
                                cells[i][j].cellType = "ocean";
                                cells[i][j].cellColor = seaColors[(int)ofRandom(6)];
                            }
                        }
                    }
                }
                
                if(!pollutionIncreasing){
                    if(currentType == "ocean" && currentInitType == "ice"){
                        if(ecoFootData[currentYear] < 0.4){
                            if(chance < 2.0){ //chance
                                cells[i][j].cellType = "ice";
                                cells[i][j].cellColor = ofColor(230,240,244);
                            }
                        }
                    }
                    
                    if(ecoFootData[currentYear] < 0.4){
                        if(currentType == "ocean" && nIce >=4){
                            if(chance < 1.0){
                                cells[i][j].cellType = "ice";
                                cells[i][j].cellColor = ofColor(240,240,244);
                            }
                        }
                    }
                }
            }
            
            
            //  ICE
            
//            if(nIce == 3 && ofRandom(1.0) > 0.5 ){
//                if(pollutionIncreasing){
//                    ofLog() << "ocean";
//                    cells[i][j].cellType = "ocean";
////                    cells[i][j].cellColor = ofColor(12, 140, 175);
//                    cells[i][j].cellColor = ofColor(0, 0, 255);
//
//                }
//                else {
//                    if(iceCounter < maxNumberIce){
//                        ofLog() << "ice";
//                        cells[i][j].cellType = "ice";
//                        cells[i][j].cellColor = ofColor(255, 255, 255);
//                    }
//                }
//            }
//
//            if(nIce == 3 && ofRandom(1.0) > 0.5 && (_c.cellType == "grass" || _c.cellType == "sand")){
//                if(extremePollution){
//                    ofLog() << "extreme pollution go ocean";
//                    cells[i][j].cellType = "ocean";
////                    cells[i][j].cellColor = ofColor(12, 140, 175);
//                    cells[i][j].cellColor = ofColor(255, 0, 0);
//
//                }
//                else {
//                    ofLog() << "extreme pollution go graass";
//                    cells[i][j].cellType = "grass";
//                    cells[i][j].cellColor = ofColor(0, 255, 0);
//                    }
//                }
            
            
            
        }
    }
    iceCounter = iceCounterTemp;
}

void ofApp::drawGUI(){
    ofSetColor(255, 0, 255);
    titleDisplay();
//    ofDrawBitmapStringHighlight("point " + ofToString(mouseX) + " / " + ofToString(ofGetWidth())
//                                + " "  + ofToString(mouseY) + " / " + ofToString(ofGetHours()), 600, 600);


    ofSetRectMode(OF_RECTMODE_CORNER);
    ofFill();
    ofSetColor(255, 255, 255, fadeInOut);
//    ofDrawEllipse(ofGetWidth() - 300 , ofGetHeight() - 50 , 50, 50);
    

    
    // Year
    timelineText.drawString("1900", centreW - 350 , ofGetHeight() - 45);
    timelineText.drawString("2100", centreW + 305 , ofGetHeight() - 45);
    timelineText.drawString(ofToString(currentYear + startYear) , centreW - 315 + (currentYear * 3), ofGetHeight() - 28);

    // timeline
    ofSetColor(255, 255, 255, 150*fadeValue);
    ofDrawLine(centreW - 300 , ofGetHeight() - 50 ,     centreW + 300 , ofGetHeight() - 50);
    ofDrawLine(centreW - 300 , ofGetHeight() - 50 + 5 , centreW - 300 , ofGetHeight() - 50 - 5);
    ofDrawLine(centreW + 300 , ofGetHeight() - 50 + 5 , centreW + 300 , ofGetHeight() - 50 - 5);
    
    // timeline cursror
    ofDrawEllipse(centreW - 300 + (currentYear * 3), ofGetHeight() - 50, 10,10);
    
    ofSetColor(255, 0, 0, fadeInOut);
    ofDrawEllipse(centreW - 300 + (maxPopYear * 3), ofGetHeight() - 50, 5,5);
    ofSetColor(0, 255, 0, fadeInOut);
    ofDrawEllipse(centreW - 300 + (maxPulYear * 3), ofGetHeight() - 50, 5,5);
    ofSetColor(255, 255, 0, fadeInOut);
    ofDrawEllipse(centreW - 300 + (maxIndYear * 3), ofGetHeight() - 50, 5,5);
    
//    ofSetColor(255, 0, 0, 255);
//
//
////    ofDrawBitmapStringHighlight("Year:       " + ofToString(currentYear + startYear) , 50, 30);
//
//    // Data and Current Values
//    ofDrawBitmapStringHighlight("Population: " + ofToString(populationData[currentYear], 3) , 50, 50);
//    ofDrawBitmapStringHighlight("Resources:  " + ofToString(lifeExpData[currentYear], 3) , 50, 70);
//
//    std::string flagStr = pollutionIncreasing ? "increasing" : "decresing";
//    std::string extPol = extremePollution ? " extreme!" : " __";
//
//
//
//    ofDrawBitmapStringHighlight("Pollution:  " + flagStr + extPol, 50, 90);
//    ofDrawBitmapStringHighlight("Industry:   " + ofToString(industryData[currentYear], 3) , 50, 110);
//    ofDrawBitmapStringHighlight("Food:       " + ofToString(foodData[currentYear], 3) , 50, 130);
//
//
//    int currentHeight = ofGetHeight();
//    ofDrawBitmapStringHighlight("ice count:       " + ofToString(iceCounter) , 50, currentHeight - 20);
//    ofDrawBitmapStringHighlight("ocean count:       " + ofToString(oceanCounter) , 50, currentHeight - 40);
//    ofDrawBitmapStringHighlight("grass count:       " + ofToString(grassCounter) , 50, currentHeight - 60);
//    ofDrawBitmapStringHighlight("sand count:       " + ofToString(sandCounter) , 50, currentHeight - 80);
//    ofDrawBitmapStringHighlight("city count:       " + ofToString(cityCounter) , 50, currentHeight - 100);
//
//    ofDrawBitmapStringHighlight( worldType, 50, currentHeight - 120);
//
//    // Data Type color indicator
//    ofSetColor(255, 0, 0);
//    ofDrawEllipse(40, 45, 5,5);
//    ofSetColor(0, 255, 0);
//    ofDrawEllipse(40, 85, 5,5);
//    ofSetColor(255, 255, 0);
//    ofDrawEllipse(40, 105, 5,5);
}

void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    switch (key){
        case ' ':
            automaCellulare();
            std::cout << "run cellure automata to update state of cells" << std::endl;
            break;
        case 'p':
            pollutionIncreasing = !pollutionIncreasing;
            extremePollution = !extremePollution;
            std::cout << "pollution state flipped" << std::endl;
            break;
        case 'o':
            populationIncreasing = !populationIncreasing;
            std::cout << "population state flipped" << std::endl;
            break;
        case 's':
            start = !start;
            std::cout << "play pause" << std::endl;
            break;
            
        case 'a':
            animate = !animate;
            std::cout << "animate" << std::endl;
            break;
            
        case '1':
            normSetup();
            worldType ="Normal World";
            break;
            
        case '2':
            goodSetup();
            worldType ="Good World";

            break;
            
        case '3':
            badSetup();
            worldType ="Bad World";
            break;
            
        case  '4':
            speed = 200;
            break;
            
        case  '5':
            speed = 50;
            break;
            
        case  'R':
            resetSimulation();
            break;
        case 'M':
            transitionStartTime = ofGetElapsedTimef();
            moveToGlobe = !moveToGlobe;
            break;
        case 'N':
            isDrifting = !isDrifting;
            break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
//    popSelected = false;
//    lifeExpSelected = false;
//    ecoFootSelected = false;
//    foodSelected = false;
//    industrySelected = false;
//
//    if (popRect.inside(x, y)) {
//        popSelected = true;
//        std::cout << "Inside popRect" << std::endl;
//    } else if (lifeExpRect.inside(x, y)) {
//        lifeExpSelected = true;
//        std::cout << "Inside lifeExpRect" << std::endl;
//    } else if (ecoFootRect.inside(x, y)) {
//        ecoFootSelected = true;
//        std::cout << "Inside ecoFootRect" << std::endl;
//    } else if (foodRect.inside(x, y)) {
//        foodSelected = true;
//        std::cout << "Inside foodRect" << std::endl;
//    } else if (industryRect.inside(x, y)) {
//        industrySelected = true;
//        std::cout << "Inside industryRect" << std::endl;
//    }
    
    if (leftArrowBox.inside(x, y)) {
        cout << "Left city clicked!" << endl;
        currentCityIndex = (currentCityIndex - 1 + cities.size()) % cities.size();

    }

    if (rightArrowBox.inside(x, y)) {
        cout << "Right city clicked!" << endl;
        currentCityIndex = (currentCityIndex + 1) % cities.size();
    }
    
    
    if(worldSwapButton.inside(x,y)){
        currentWorldIndex++;
        if(currentWorldIndex > 2){
            currentWorldIndex = 0;
        }
        
        cout << "WorldSwap clicked! "  + ofToString(currentWorldIndex)<< endl;
        
        if(currentWorldIndex == 0){
            normSetup();
            worldType ="Normal World";
        }
        else if(currentWorldIndex == 1){
            goodSetup();
            worldType ="Good World";
        }
        else if(currentWorldIndex == 2){
            badSetup();
            worldType ="Bad World";
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

void ofApp::checkDataDirection() {
    
    int nextYear = currentYear + 1;
    if( ecoFootData[currentYear] > 0.0){
        pollutionIncreasing = (ecoFootData[currentYear] < ecoFootData[nextYear]);
    }
    populationIncreasing = (populationData[currentYear] < populationData[nextYear] && populationData[currentYear] > 0.1);
    foodIncreasing = (foodData[currentYear] < foodData[nextYear]);
    industryIncreasing = (industryData[currentYear] < industryData[nextYear]);
    resourcesIncreasing = (lifeExpData[currentYear] < lifeExpData[nextYear]);
    extremePollution = (ecoFootData[currentYear] > 0.5);
}

void ofApp::loadCSVData(){
    
    if(normCsv.load("normal.csv")) {
        for (int i = 0; i < 11; ++i) {
            for (int j = 0; j < arraySize; j++) {
                normData[i][j] = std::stof(normCsv[j+1][i]);
                //std::cout << normData[i][j] << " ";
            }
            //std::cout << "next" << std::endl;
        }
        std::cout << "Normal Data Read" << std::endl;

    }
    else {
        std::cout << "no file found?";
    }
    
    if(goodCsv.load("good.csv")) {
        for (int i = 0; i < 11; ++i) {
            for (int j = 0; j < arraySize; j++) {
                goodData[i][j] = std::stof(goodCsv[j+1][i]);
            }
        }
        std::cout << "Good Data Read" << std::endl;

    }
    else {
        std::cout << "no file found?";
    }
    
    if(badCsv.load("bad.csv")) {
        for (int i = 0; i < 11; ++i) {
            for (int j = 0; j < arraySize; j++) {
                badData[i][j] = std::stof(badCsv[j+1][i]);
            }
        }
        std::cout << "Bad Data Read" << std::endl;

    }
    else {
        std::cout << "no file found?";
    }
    
}

void ofApp::goodSetup(){
    ecoFootData = goodData[9];
    industryData = goodData[1];
    foodData = goodData[2];
    populationData = goodData[3];
    lifeExpData = goodData[5];
    persistantPollutionData = goodData[4];
    setupMaxYear();
    
    cout << "good Setup" << endl;
}

void ofApp::badSetup(){
    ecoFootData = badData[9];
    industryData = badData[1];
    foodData = badData[2];
    populationData = badData[3];
    lifeExpData = badData[5];
    persistantPollutionData = badData[4];

    
    setupMaxYear();
    
    cout << "bad Setup" << endl;
}

void ofApp::normSetup(){
    ecoFootData = normData[9];
    industryData = normData[1];
    foodData = normData[2];
    populationData = normData[3];
    lifeExpData = normData[5];
    persistantPollutionData = normData[4];

    //            for (int j = 0; j < arraySize; j++) {
    //                std::cout << j << std::endl;
    //                std::cout << ecoFootData[j] << std::endl;
    //            }
    setupMaxYear();
    cout << "normal Setup" << endl;

}

void ofApp::variableSetup(){
    pollutionIncreasing = false;
    populationIncreasing = true;
    extremePollution = false;
    startYear = 1900;
    endYear = 2100;
    r1 = {-1*_PI, _PI};
    r2 = {0.0, 640.0};
    sphere.setRadius(128);
    spherePlanet.setRadius(32);
    rotationAngle = 0.0;
    currentCityIndex = 0;
    oscOut.setup("localhost", 8080);    //OSC
    oscIn.setup(8000);                  //OSC
    cities = {"NEW YORK", "CAIRO", "LAGOS", "RIO"};
    worldMessages = {"what if we do nothing?","whats the best we can hope for?", "whats the worst that could happen?"};


}

void ofApp::setupMaxYear(){
    maxPopYear = distance(populationData, max_element(populationData, populationData + 200));
    maxPulYear = distance(ecoFootData, max_element(ecoFootData, ecoFootData + 200));
    maxIndYear = distance(industryData, max_element(industryData, industryData + 200));
}

void ofApp::resetSimulation(){
    currentYear = 0;
    populationPoly.clear();
    pollutionPoly.clear();
    foodPoly.clear();
    industryPoly.clear();
    lifeExpPoly.clear();
    //imageToGrid();
    //setCellSphereRadius();
    
    for (int i = 0; i < 80; i++) {
        for (int j = 0; j < 80; j++) {
            cells[i][j].cellColor = cells[i][j].initCellcolor;
            cells[i][j].cellType = cells[i][j].initCellType;

        }
    }
    updateImageForYear(worldType);
    
    
    cout << "Reset Simulation" << endl;
}

void ofApp::setCellSphereRadius(){
    float value = 8.0;
    int start = 0;
    int end = 80;
    float minScale = 0.05;
    float maxScale = 0.95;
    float scale = 0;
    
    for(int i = 0; i < 80; i = i + 1)
    {
        for(int j = 0; j < 80; j = j + 1)
        {
            float scale = scaleValue(j, start, end, minScale, maxScale);
            cells[i][j].sphereRadius = value * scale;
        }
    }
}

float ofApp::scaleValue(int i, int start, int end, float minScale, float maxScale) {
    int mid = (start + end) / 2;
        float scale = minScale + (maxScale - minScale) * (1.0 - std::abs(i - mid) / float(mid));
    
    return scale;
}

double ofApp::Gudermannian(double y) {
    return atan(sinh(y)) * (180.0 / PI);
}

double ofApp::GudermannianInv(double latitude) {
    int sign = (latitude > 0) - (latitude < 0);
    double sinValue = sin(latitude * (PI / 180.0) * sign);
    return sign * (log((1 + sinValue) / (1 - sinValue)) / 2.0);
}

double ofApp::convertRange(double value, const std::array<double, 2>& r1, const std::array<double, 2>& r2) {
    return (value - r1[0]) * (r2[1] - r2[0]) / (r1[1] - r1[0]) + r2[0];
}

ofColor ofApp::rgbToHsb(ofColor rgb){
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    // find min and mac value of rgb components for brightness calc
    float maxVal = max(r, max(g, b));
    float minVal = min(r, min(g, b));
    float delta = maxVal - minVal;

    float hue = 0.0f;
    float saturation = (maxVal == 0.0f) ? 0.0f : delta / maxVal;
    float brightness = maxVal;
    
    if (delta != 0.0f) {
        if (maxVal == r) {
            hue = 60.0f * fmod(((g - b) / delta), 6.0f);
        } else if (maxVal == g) {
            hue = 60.0f * (((b - r) / delta) + 2.0f);
        } else if (maxVal == b) {
            hue = 60.0f * (((r - g) / delta) + 4.0f);
        }
    }

    if (hue < 0.0f) {
        hue += 360.0f;
    }

    hue = fmod(hue, 360.0f); // Make sure hue is between 0 and 360

    ofColor hsbColor;
    hsbColor.setHsb(hue / 360.0f * 255.0f, saturation * 255.0f, brightness * 255.0f);

    return hsbColor;
}

bool ofApp::findColor(ofColor col, float min, float max){
    float hue = col.getHueAngle();
    if(hue> min && hue < max){
        return true;
    }else{
        return false;
    }
}

bool ofApp::findSaturation(ofColor col, float min, float max){
    float sat = col.getSaturation();
    if(sat> min && sat < max){
        return true;
    }else{
        return false;
    }
}

string ofApp::classifyCelltype(ofColor rgbColor, float imHeight, float j)
{
    if(j < imHeight * 0.35f || j > imHeight * 0.65f  ){
        if (findSaturation(rgbColor, 0.f, 40.f)) {return "ice";}
        else if (rgbColor.getBrightness() > 230) {return"ice";}
        else if (findColor(rgbColor, 170.f, 270.f)) {return "ocean";}
        else if (findColor(rgbColor, 70.f, 169.f)) {return "grass";}
        else if (rgbColor.getBrightness() < 150.0f) {return"grass";}


    }
    else if (findColor(rgbColor, 170.f, 270.f)) {return "ocean";}
    else if (findColor(rgbColor, 70.f, 169.f)) {return "grass";}
    else if (rgbColor.getBrightness() < 150.0f) {return"grass";}
    
    if(findColor(rgbColor, 0.0f, 10.0f)){
        cout << "city" << endl;
        return "city";
    }
    
    return "sand";
}

void ofApp::otherPlanetsSetup(){
    mars.color = ofColor(193,68,14);
    mars.radius = 5.0f;
    mars.orbitRadiusX = 800.0f;
    mars.orbitRadiusY = 200.0f;
    mars.orbitSpeed = 5.0f;
    mars.angle = 0.0f;
    mars.inclination = 45.0f;

    jupiter.color = ofColor(228, 229, 198);
    jupiter.radius = 5.0f;
    jupiter.orbitRadiusX = 1000.0f;
    jupiter.orbitRadiusY = 250.0f;
    jupiter.orbitSpeed = 3.0f;
    jupiter.angle = 0.0f;
    jupiter.inclination = 45.0f;
    
    pluto.color = ofColor(37, 114, 206);
    pluto.radius = 5.0f;
    pluto.orbitRadiusX = 9000.0f;
    pluto.orbitRadiusY = 190.0f;
    pluto.orbitSpeed = 2.0f;
    pluto.angle = 0.0f;
    pluto.inclination = 45.0f;
}

void ofApp::updatePlanets() {
    mars.angle += mars.orbitSpeed * ofGetLastFrameTime();
    jupiter.angle += jupiter.orbitSpeed * ofGetLastFrameTime();
    pluto.angle += pluto.orbitSpeed * ofGetLastFrameTime();


    float marsRad = mars.angle * DEG_TO_RAD;
    float jupiterRad = jupiter.angle * DEG_TO_RAD;
    float plutoRad = pluto.angle * DEG_TO_RAD;


    mars.pos3D.x = mars.orbitRadiusX * cos(marsRad);
    mars.pos3D.y = mars.orbitRadiusY * sin(marsRad);
    mars.pos3D.z = -50;
    mars.pos3D.rotate(mars.inclination, ofVec3f(1, 0, 0));

    jupiter.pos3D.x = jupiter.orbitRadiusX * cos(jupiterRad);
    jupiter.pos3D.y = jupiter.orbitRadiusY * sin(jupiterRad);
    jupiter.pos3D.z = -25;
    jupiter.pos3D.rotate(jupiter.inclination, ofVec3f(1, 0, 0));
    
    pluto.pos3D.x = pluto.orbitRadiusX * cos(plutoRad);
    pluto.pos3D.y = pluto.orbitRadiusY * sin(plutoRad);
    pluto.pos3D.z = -25;
    pluto.pos3D.rotate(pluto.inclination, ofVec3f(1, 0, 0));
}

void ofApp::drawPlanets() {
    if (mars.pos3D.z < 0) {
        ofSetColor(mars.color);
        ofDrawSphere(mars.pos3D.x, mars.pos3D.y, mars.pos3D.z, mars.radius);
    }

    if (jupiter.pos3D.z < 0) {
        ofSetColor(jupiter.color);
        ofDrawSphere(jupiter.pos3D.x, jupiter.pos3D.y, jupiter.pos3D.z, jupiter.radius);
    }
    
    if (pluto.pos3D.z < 0) {
        ofSetColor(pluto.color);
        ofDrawSphere(pluto.pos3D.x, pluto.pos3D.y, pluto.pos3D.z, pluto.radius);
    }
}

void ofApp::fontSetup(){
    string fontname = "fonts/nasalization-rg.otf";
    myfont.load(fontname, 18);
    popText.load(fontname, 18);
    lifeExpText.load(fontname, 18);
    foodText.load(fontname, 18);
    ecoFootText.load(fontname, 18);
    industryText.load(fontname, 18);
    timelineText.load(fontname, 12);
    cityFont.load(fontname, 18);
    
    titleFont.load(fontname, 48);
    worldMessageFont.load(fontname, 18);
}


void ofApp::updateImageForYear(std::string condition) {
    string condition_conv = convertWorld(condition);
    int realCurrentYear = currentYear + startYear;

    for (int i = 0; i < 4; i++){
        string cityName = cities[i];
        string currentCityLC = toTitleCase(cityName);
        std::string imageName = "photos/" + currentCityLC + "/" + currentCityLC + "_"
            + condition_conv + "_" + std::to_string(realCurrentYear) + ".png";
        if (ofFile::doesFileExist(imageName)) {
            if (currentCityLC == "New York") {
                cityImgNY.load(imageName);
                cityImgNY.resize(imageSize, imageSize);
            }
            else if (currentCityLC == "Rio") {
                cityImgRio.load(imageName);
                cityImgRio.resize(imageSize, imageSize);

            }
            else if (currentCityLC == "Cairo") {
                cityImgCairo.load(imageName);
                cityImgCairo.resize(imageSize, imageSize);
            }
            else if (currentCityLC == "Lagos") {
                cityImgLagos.load(imageName);
                cityImgLagos.resize(imageSize, imageSize);
            }
            else {
                std::cout << "Unknown city: " << currentCityLC << std::endl;
            }
        }
    }
}


void ofApp::loadImageForYear() {
    string currentCityLC = toTitleCase(currentCity);

    
    if (currentCityLC == "New York") {
        bShowImageNY = true;
        bShowImageRio = false;
        bShowImageCairo = false;
        bShowImageLagos = false;
    }
    else if (currentCityLC == "Rio") {
        bShowImageNY = false;
        bShowImageRio = true;
        bShowImageCairo = false;
        bShowImageLagos = false;

    }
    else if (currentCityLC == "Cairo") {
        bShowImageNY = false;
        bShowImageRio = false;
        bShowImageCairo = true;
        bShowImageLagos = false;
    }
    else if (currentCityLC == "Lagos") {
        bShowImageNY = false;
        bShowImageRio = false;
        bShowImageCairo = false;
        bShowImageLagos = true;
    }
        

    
}

std::string ofApp::toTitleCase(const std::string& input) {
    std::string result = input;
    bool capitalize = true;

    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] == ' ') {
            capitalize = true;
        }
        else if (capitalize && std::isalpha(result[i])) {
            result[i] = std::toupper(result[i]);
            capitalize = false;
        }
        else {
            result[i] = std::tolower(result[i]);
        }
    }

    return result;
}

std::string ofApp::convertWorld(const std::string& input) {
    std::map<std::string, std::string> worldMap = {
        {"Good World", "good"},
        {"Normal World", "neutral"},
        {"Bad World", "bad"}
    };

    auto it = worldMap.find(input);
    if (it != worldMap.end()) {
        return it->second;
    }
    else {
        return "unknown"; 
    }
}

void ofApp::drawCityImages() {
    ofSetColor(255);
    ofSetColor(200);
   
    int borderOffset = 10;
    ofDrawRectangle(origin.x - borderOffset, origin.y - borderOffset, plotWidth + borderOffset*2, plotHeight + borderOffset*2);
    
    ofSetColor(255, 255, 255, fadeInOut);

        
    if (bShowImageNY) {
        cityImgNY.draw(edgeOffset, ofGetHeight()/2 - imageSize/2);
    }
    if (bShowImageRio) {
        cityImgRio.draw(edgeOffset, ofGetHeight()/2 - imageSize/2);
    }
    if (bShowImageCairo) {
        cityImgCairo.draw(edgeOffset, ofGetHeight()/2 - imageSize/2);
    }
    if (bShowImageLagos) {
        cityImgLagos.draw(edgeOffset, ofGetHeight() / 2 - imageSize / 2);
    }


}

void ofApp::initCityBools(){
    bShowImageNY = false;
    bShowImageRio = false;
    bShowImageCairo = false;
    bShowImageLagos = false;
}

void ofApp::citySelect(){
    // Draw the red rectangle
    
    //ofDrawRectangle(edgeOffset, centreH + imageSize/2 + edgeOffset/2, imageSize, 60);
    
    float rectWidth = imageSize;
    float rectHeight = 40;
    float triangleSize = rectHeight;
    float triangleHeight = sqrt(3.0) / 2.0 * triangleSize;
    
    leftTriangleA.set(edgeOffset - triangleHeight / 2 +20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2);
    leftTriangleB.set(edgeOffset + triangleHeight / 2 +20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2 - triangleSize / 2);
    leftTriangleC.set(edgeOffset + triangleHeight / 2 + 20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2 + triangleSize / 2);
    
    rightTriangleA.set(edgeOffset + rectWidth + triangleHeight / 2 -20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2);
    rightTriangleB.set(edgeOffset + rectWidth - triangleHeight / 2 - 20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2 - triangleSize / 2);
    rightTriangleC.set(edgeOffset + rectWidth - triangleHeight / 2 - 20, centreH + imageSize / 2 + edgeOffset/2 + rectHeight / 2 + triangleSize / 2);
    
    ofSetColor(200,200,200, (int)fadeInOut);
    ofDrawTriangle(leftTriangleA, leftTriangleB, leftTriangleC);
    ofDrawTriangle(rightTriangleA, rightTriangleB, rightTriangleC);
    
    leftArrowBox.set(leftTriangleA.x, leftTriangleB.y, triangleHeight, triangleSize);
    rightArrowBox.set(rightTriangleB.x, rightTriangleB.y, triangleHeight, triangleSize);

    ofSetColor(255, fadeInOut); // White color for the box
//    ofNoFill();
//    ofDrawRectangle(leftArrowBox);
//    ofDrawRectangle(rightArrowBox);
//    ofFill();
    
    currentCity = cities[currentCityIndex];
    ofRectangle cityBox = cityFont.getStringBoundingBox(currentCity, 0, 0);
    float cityX = edgeOffset + rectWidth / 2 - cityBox.width / 2;
    float cityY = centreH + imageSize / 2 + edgeOffset + + cityBox.height / 2;
    cityFont.drawString(currentCity, cityX, cityY);
}

void ofApp::worldMessageDisplay(){

}

void ofApp::titleDisplay(){
    ofSetColor(255);
    string titleString = "Future Worlds";
    ofRectangle titleBox = titleFont.getStringBoundingBox(titleString, 0, 0);
    float titleX = centreW - titleBox.width/2;
    float titley = edgeOffset*2 - titleBox.height/2;
    titleFont.drawString(titleString, titleX, titley);
    
    ofSetColor(255, fadeInOut);
    string worldMessageString = worldMessages[currentWorldIndex];
    ofRectangle worldMessageBox = worldMessageFont.getStringBoundingBox(worldMessageString, 0, 0);
    float wMesgX = centreW - worldMessageBox.width/2;
    float wMesgY = edgeOffset*2 + titleBox.height;
    
    worldMessageFont.drawString(worldMessageString, wMesgX, wMesgY);
    worldSwapButton.set(titleX, titley - titleBox.height ,titleBox.width,titleBox.height);
    
    //ofDrawRectangle(worldSwapButton);
}


void ofApp::sendOsc(){
    //timeSent = ofGetElapsedTimef();
    float initPopOSCVal = populationData[0];
    float initPulOSCVal = ecoFootData[0];
    float initIndOSCVal = industryData[0];
    float initPulIncOSCVal = 1.0f - ecoFootData[0];
    float initFoodOSCVal = foodData[0];

    float popOSCVal = populationData[currentYear];
    float pulOSCVal = ecoFootData[currentYear];
    float indOSCVal = industryData[currentYear];
    float pulIncOSCVal = 1.0f - ecoFootData[currentYear];
    float foodOSCVal = foodData[currentYear];


    ofxOscMessage m1;
    m1.setAddress("/pop");

    
    ofxOscMessage m2;
    m2.setAddress("/pul");
    ofxOscMessage m4;
    m4.setAddress("/pulInv");
    
    ofxOscMessage m3;
    m3.setAddress("/ind");
    
    ofxOscMessage m6;
    m6.setAddress("/food");


    
    if(currentYear < 2098){
        m1.addFloatArg(popOSCVal);
        m2.addFloatArg(pulOSCVal);
        m3.addFloatArg(indOSCVal);
        m4.addFloatArg(pulIncOSCVal);
        m6.addFloatArg(foodOSCVal);


    }
    else {
        m1.addFloatArg(initPopOSCVal);
        m2.addFloatArg(initPulOSCVal);
        m3.addFloatArg(initIndOSCVal);
        m4.addFloatArg(initPulIncOSCVal);
        m6.addFloatArg(initFoodOSCVal);

    }
    
    oscOut.sendMessage(m3);
    oscOut.sendMessage(m1);
    oscOut.sendMessage(m2);
    oscOut.sendMessage(m4);
    oscOut.sendMessage(m6);
    
}
