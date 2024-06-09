
//--------------------------------------------------------------
#include "ofApp.h"
using namespace std;

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableAlphaBlending();
    ofEnableDepthTest();
//    light.enable();
//    light.setPosition(ofVec3f(100,100,200));
//    light.lookAt(ofVec3f(0,0,0));
    ofSetBackgroundColor(0);
    loadCSVData();
    normSetup();
    imageToGrid();
    setCellSphereRadius();
    variableSetup();
    lastMinute = ofGetElapsedTimeMillis();

    std::cout << "set up finished"  << std::endl;
}




//--------------------------------------------------------------
void ofApp::update(){
    rotationAngle += 0.1;  // for globe spin

    if(currentYear == 199){ start = false;}

    if(start){
        now = ofGetElapsedTimeMillis();
        if (now - lastMinute >= speed) { // 60000 milliseconds = 1 minute
            automaCellulare();
            checkDataDirection();
            updatePlotter();
            currentYear += 1;
            lastMinute = now;
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //worldImage.draw(0,0);

//    earthSpinning();
    
    cellArrayToImage();
    
    drawGUI();
    dataPlotter();

}

void ofApp::dataPlotter(){
    plotWidth = ofGetWidth() * 0.3;
    plotHeight = ofGetHeight() * 0.2;
    origin.set(ofGetWidth() - plotWidth, plotHeight + 30);


    
    if(pollutionPoly.size() > 0){
        ofSetColor(50, 50, 50);
        populationPoly.draw();
        ofSetColor(255, 255, 255);
        pollutionPoly.draw();
        ofSetColor(0, 255, 0);
        foodPoly.draw();
        ofSetColor(50, 50, 50);
        industryPoly.draw();
        ofSetColor(50, 50, 50);
        lifeExpPoly.draw();
    }
    //ofSetColor(100);
    //ofDrawRectangle(ofGetWidth() - plotWidth, 0 + 30, plotWidth, plotHeight);
 
}

void ofApp::updatePlotter(){
    ofSetLineWidth(3);

    float xx = ((float)currentYear / 200.0) * plotWidth + origin.x;
    populationPoly.addVertex( xx , origin.y - (populationData[currentYear] * plotHeight));
    pollutionPoly.addVertex( xx , origin.y - (ecoFootData[currentYear] * plotHeight));
    foodPoly.addVertex( xx , origin.y - (foodData[currentYear] * plotHeight));
    industryPoly.addVertex( xx , origin.y - (industryData[currentYear] * plotHeight));
    lifeExpPoly.addVertex( xx , origin.y - (lifeExpData[currentYear] * plotHeight));
    
    populationPoly.getSmoothed(5, 0.33);
    pollutionPoly.getSmoothed(5, 0.33);
    foodPoly.getSmoothed(5, 0.33);
    industryPoly.getSmoothed(5, 0.33);
    lifeExpPoly.getSmoothed(5, 0.33);
    
}

void ofApp::earthSpinning(){
    cam.begin();
    ofPushMatrix();
    
    ofRotateXDeg(80);  // Rotate the sphere 90 degrees around the X-axis
    ofRotateZDeg(rotationAngle);  // Apply the rotation around the Y-axis for spinning effect
    ofSetColor(246,245, 257);
    sphere.draw();
    cellArrayToImage3D();

    ofPopMatrix();

//    ofPushMatrix();
//    // Translate to the top right quadrant
//    ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
//    // Adjust the position further if necessary
//    ofTranslate(-spherePlanet.getRadius(), -spherePlanet.getRadius());
//    // Draw the second sphere
//    spherePlanet.draw();
//    ofPopMatrix();

    cam.end();
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
            latitude = Gudermannian(convertRange(j,r2,r1));
            longitude = ofRadToDeg(i / R) - 180.0;

            // conversion to spherical coordinates
            float phi = (90 - (latitude)) * (_PI / 180);
            float theta = (180 + (longitude)) * (_PI / 180);
            cell.pos3D.x = -1* (S * sin(phi) * cos(theta));
            cell.pos3D.y = S * sin(phi) * sin(theta);
            cell.pos3D.z = S * cos(phi);
            
            ofColor rgbColor =  worldImage.getColor(i,j);
            cell.cellColor = rgbColor;
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
    ofLog() << colors.size() << " size" ;

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
            ofDrawSphere(_c.pos3D.x, _c.pos3D.y, _c.pos3D.z, _c.sphereRadius);
        }
    }
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
                    else {nCity += 1;}
                }
            }
            float chance = ofRandom(8.0);
            
            if(foodIncreasing){
                if(currentType == "sand" && nGrass >= 2){
                   if(chance < foodData[currentYear]){
                       cout << "chance " + ofToString(chance) + " foodLvl " +  ofToString(foodData[currentYear]) << endl;
                        cells[i][j].cellType = "grass";
                        cells[i][j].cellColor = ofColor(0, 255, 0);
                        cout << "make grass" << endl;
                    }
                }
            }
            
            if(!foodIncreasing){
                if(currentType == "grass" && currentInitType == "sand"){
                    if(chance < foodData[currentYear]/2   ){
                        cells[i][j].cellType = "sand";
                        cells[i][j].cellColor = ofColor(255,250,205);
                        cout << "make sand" << endl;
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




//--------------------------------------------------------------
// Draw GUI
//--------------------------------------------------------------
void ofApp::drawGUI(){
    //    myfont.drawString("Future Worlds", ofGetWidth()/2 - myfont.stringWidth("Future Wolrds")/2 - 25, 100);
    //    ofDrawBitmapStringHighlight("point " + ofToString(mouseX) + " " + ofToString(mouseY) , 600, 600);

    centreH = ofGetHeight()/2;
    centreW = ofGetWidth()/2;
    ofSetRectMode(OF_RECTMODE_CORNER);
    ofFill();
    ofSetColor(255, 255, 255);
//    ofDrawEllipse(ofGetWidth() - 300 , ofGetHeight() - 50 , 50, 50);
    
    // timeline
    ofDrawLine(centreW - 300 , ofGetHeight() - 50 , centreW + 300 , ofGetHeight() - 50);
    ofDrawLine(centreW - 300 , ofGetHeight() - 50 + 5 , centreW - 300 , ofGetHeight() - 50 - 5);
    ofDrawLine(centreW + 300 , ofGetHeight() - 50 + 5 , centreW + 300 , ofGetHeight() - 50 - 5);
    
    // timeline cursror
    ofDrawEllipse(centreW - 300 + (currentYear * 3), ofGetHeight() - 50, 10,10);
    
    // Year
    ofDrawBitmapStringHighlight(ofToString(currentYear + startYear) , centreW - 315 + (currentYear * 3), ofGetHeight() - 28);
    ofDrawBitmapStringHighlight("1900", centreW - 300 , ofGetHeight() - 70);
    ofDrawBitmapStringHighlight("2100", centreW + 270 , ofGetHeight() - 70);
    
    ofSetColor(255, 0, 0);
    ofDrawEllipse(centreW - 300 + (maxPopYear * 3), ofGetHeight() - 50, 5,5);
    ofSetColor(0, 255, 0);
    ofDrawEllipse(centreW - 300 + (maxPulYear * 3), ofGetHeight() - 50, 5,5);
    ofSetColor(255, 255, 0);
    ofDrawEllipse(centreW - 300 + (maxIndYear * 3), ofGetHeight() - 50, 5,5);
    
    ofSetColor(255, 0, 0, 255);

    
//    ofDrawBitmapStringHighlight("Year:       " + ofToString(currentYear + startYear) , 50, 30);

    // Data and Current Values
    ofDrawBitmapStringHighlight("Population: " + ofToString(populationData[currentYear], 3) , 50, 50);
    ofDrawBitmapStringHighlight("Resources:  " + ofToString(lifeExpData[currentYear], 3) , 50, 70);
    
    std::string flagStr = pollutionIncreasing ? "increasing" : "decresing";
    std::string extPol = extremePollution ? " extreme!" : " __";


    
    ofDrawBitmapStringHighlight("Pollution:  " + flagStr + extPol, 50, 90);
    ofDrawBitmapStringHighlight("Industry:   " + ofToString(industryData[currentYear], 3) , 50, 110);
    ofDrawBitmapStringHighlight("Food:       " + ofToString(foodData[currentYear], 3) , 50, 130);
    
    
    int currentHeight = ofGetHeight();
    ofDrawBitmapStringHighlight("ice count:       " + ofToString(iceCounter) , 50, currentHeight - 20);
    ofDrawBitmapStringHighlight("ocean count:       " + ofToString(oceanCounter) , 50, currentHeight - 40);
    ofDrawBitmapStringHighlight("grass count:       " + ofToString(grassCounter) , 50, currentHeight - 60);
    ofDrawBitmapStringHighlight("sand count:       " + ofToString(sandCounter) , 50, currentHeight - 80);
    ofDrawBitmapStringHighlight("city count:       " + ofToString(cityCounter) , 50, currentHeight - 100);
    
    ofDrawBitmapStringHighlight( worldType, 50, currentHeight - 120);


    
    // Data Type color indicator
    ofSetColor(255, 0, 0);
    ofDrawEllipse(40, 45, 5,5);
    ofSetColor(0, 255, 0);
    ofDrawEllipse(40, 85, 5,5);
    ofSetColor(255, 255, 0);
    ofDrawEllipse(40, 105, 5,5);
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
    pollutionIncreasing = (ecoFootData[currentYear] < ecoFootData[nextYear] && ecoFootData[currentYear] > 0.1);
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
    setupMaxYear();
    
    cout << "good Setup" << endl;
}

void ofApp::badSetup(){
    ecoFootData = badData[9];
    industryData = badData[1];
    foodData = badData[2];
    populationData = badData[3];
    lifeExpData = badData[5];
    
    setupMaxYear();
    
    cout << "bad Setup" << endl;
}

void ofApp::normSetup(){
    ecoFootData = normData[9];
    industryData = normData[1];
    foodData = normData[2];
    populationData = normData[3];
    lifeExpData = normData[5];
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
    imageToGrid();
    setCellSphereRadius();
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
    
    return "sand";
}
