#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxOpenCv.h"
#include "ofxCsv.h"
#include <iostream>
#include <array>
#include <set>
#include <tuple>

#define PORT 8000


class Cell {
public:
    ofColor cellColor;
    ofColor initCellcolor;
    string cellType;
    string initCellType;
    ofVec3f driftPos;
    ofVec3f velocity;
    ofVec2f pos2D;
    ofVec3f pos3D;
    float sphereRadius;
};

struct Planet {
    ofVec3f pos3D;
    ofColor color;
    float radius;
    float orbitRadiusX;
    float orbitRadiusY;
    float orbitSpeed;
    float angle;
    float inclination;
};

class ofApp : public ofBaseApp{

    public:
        void setup() override;
        void update() override;
        void draw() override;
        void exit() override;

        void keyPressed(int key) override;
        void keyReleased(int key) override;
        void mouseMoved(int x, int y ) override;
        void mouseDragged(int x, int y, int button) override;
        void mousePressed(int x, int y, int button) override;
        void mouseReleased(int x, int y, int button) override;
        void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
        void mouseEntered(int x, int y) override;
        void mouseExited(int x, int y) override;
        void windowResized(int w, int h) override;
        void dragEvent(ofDragInfo dragInfo) override;
        void gotMessage(ofMessage msg) override;
    
    void drawDataLegend();
    

    void loadCSVData();
    void goodSetup();
    void badSetup();
    void normSetup();
    
    void resetSimulation();

    
    ofxCsv goodCsv;
    ofxCsv normCsv;
    ofxCsv badCsv;
    
    const int numArrays = 11;
    const int arraySize = 200;
    float goodData[11][200];
    float badData[11][200];
    float normData[11][200];
    
    float* ecoFootData;
    float* persistantPollutionData;
    float* populationData;
    float* foodData;
    float* industryData;
    float* lifeExpData;
    
    string worldType = "Normal World";
    
    bool dataReady = false;
    
    
    ofTrueTypeFont myfont;
    ofTrueTypeFont popText;
    ofTrueTypeFont lifeExpText;
    ofTrueTypeFont foodText;
    ofTrueTypeFont ecoFootText;
    ofTrueTypeFont industryText;
    ofTrueTypeFont timelineText;
    
    void fontSetup();
    
    ofRectangle popRect;
    ofRectangle lifeExpRect;
    ofRectangle foodRect;
    ofRectangle ecoFootRect;
    ofRectangle industryRect;
    
    ofVec2f popCirc1;
    ofVec2f lifeExpCirc1;
    ofVec2f foodCirc1;
    ofVec2f ecoFootCirc1;
    ofVec2f industryCirc1;
    
    void dataCirc1Setup();
    
    ofColor popColor;
    ofColor lifeExpColor;
    ofColor ecoFootColor;
    ofColor foodColor;
    ofColor industryColor;

    bool popSelected;
    bool lifeExpSelected;
    bool ecoFootSelected;
    bool foodSelected;
    bool industrySelected;



    void setupMaxYear();
    
    ofImage placemarkerImage;
    
    void imageToGrid();
    void cellArrayToImage();
    void cellArrayToImage3D();
    void earthSpinning();
    float scaleValue(int i, int start, int end, float minScale, float maxScale);
    double Gudermannian(double y);
    double GudermannianInv(double latitude);
    double convertRange(double value, const std::array<double, 2>& r1, const std::array<double, 2>& r2);
    string classifyCelltype(ofColor rgbColor, float imHeight, float j);
    void setCellSphereRadius();
    void drawGUI();
    void checkDataDirection();
    
    // OSC
    void sendOsc();
    //void receiveOsc();
    
    ofxOscReceiver oscIn;
    ofxOscSender oscOut;
    ofxOscMessage oscMessage;
    
    void variableSetup();
    void dataPlotter();

    void updatePlotter();

    void automaCellulare();

    
    ofColor rgbToHsb(ofColor rgb);
    
    const double _PI = 3.14159265358979323846;
    std::array<double, 2> r1;
    std::array<double, 2> r2;

    float rotationAngle;
    
    bool findColor(ofColor col, float min, float max);
    bool findSaturation(ofColor col, float min, float max);

    
    //const double R = (256 - 16)/(2 * _PI); // 203.718
    const double R = (640 - 640*(16/256))/(2 * _PI); // 203.718

    ofSpherePrimitive sphere;
    ofSpherePrimitive spherePlanet;

    ofLight light;
    ofTexture mTex;
    ofEasyCam cam;
    
    ofImage worldImage;
    
    ofxCvColorImage         colorImg;
    ofxCvGrayscaleImage     grayImage;
    ofxCvGrayscaleImage     grayBg;
    ofxCvGrayscaleImage     grayDiff;
    ofxCvContourFinder      contourFinder;

    int thresholdValue;
    float thresh;
    bool bLearnBackground;
    
    int imgWidth;
    int imgHeight;
    vector<ofPolyline> landmasses;
    
    Cell cells[80][80];
    
    vector<ofColor> colors;

    int a;
    int b;
    
    float testerA;
    float testerB;
    
    float lastMinute;
    unsigned long now;
    int speed = 200;
    
    // CELL AUTO
    
    int cityCounter = 4;
    int maxNumberCity = 200;
    int iceCounter = 0;
    int maxNumberIce = 2685;
    
    int oceanCounter = 0;
    int grassCounter = 0;
    int sandCounter = 0;
    
    bool animate = true;
    
    bool pollutionIncreasing;   //controls icecaps
    bool populationIncreasing;  //controls cities
    bool foodIncreasing;        //controls vegitation
    bool extremePollution;      //controls sea lvl
    bool industryIncreasing;    //
    bool resourcesIncreasing;
    
    int maxPopYear;
    int maxPulYear;
    int maxFoodYear;
    int maxIndYear;
    
    int startYear;
    int endYear;
    int currentYear;
    bool start = false;
    
    float centreH;
    float centreW;
    float edgeOffset = 60;
    int imageSize = 300;

    
    // animation
    bool moveToGlobe = false;
    bool isDrifting = true;
    bool areSpheresInPosition = false;
    bool checkSpheresInPosition(float threshold);
    float fadeInOut = 255.0f;
    float fadeValue = 0.0f;
    float startTime;
    float duration;
    bool isTimerRunning;
    bool incrementing;
    
    float transitionStartTime = 0.0f;
    float transitionDuration = 3.0f;
    
    // other planets
    Planet mars;
    Planet jupiter;
    Planet pluto;
    void otherPlanetsSetup();
    void updatePlanets();
    void drawPlanets();
    
    // Graph Plot Variables
    int plotWidth;
    int plotHeight;
    ofPolyline populationPoly;
    ofPolyline pollutionPoly;
    ofPolyline foodPoly;
    ofPolyline industryPoly;
    ofPolyline lifeExpPoly;
    ofVec2f origin;
    
    ofColor grassColors[8] = {
        ofColor(53, 78, 31),
        ofColor(52, 80, 31),
        ofColor(77, 100, 47),
        ofColor(98, 93, 54),
        
        ofColor(70, 87, 41),
        ofColor(119, 119, 75),
        ofColor(39, 75, 32),
        ofColor(44, 68, 26)
    };
    
    ofColor sandColors[5] = {
        ofColor(204, 173, 129),
        ofColor(177, 151, 121),
        ofColor(221, 199, 155),
        ofColor(145, 131, 91),
        ofColor(151, 109, 74),
    };
    
    ofColor seaColors[7] = {
        ofColor(11, 123, 160),
        ofColor(64,137 ,155 ),
        ofColor(102, 179,205 ),
        ofColor(12, 140, 176),
        ofColor(12, 134, 179),
        ofColor(12, 151,212 ),
        ofColor(6, 81, 98)
    };
    
    void myGraphCols();
    ofColor blue;
    ofColor purple;
    ofColor organge;
    ofColor brown;
    ofColor greenA;
    ofColor greenB;
    ofColor yello;
    
    int photoYears[31] = {
        1900, 1906, 1913, 1919, 1926, 1932, 1939, 1945, 1952, 1958, 1965, 1971, 1978,
        1984, 1991, 1997, 2004, 2010, 2017, 2023, 2030, 2036, 2043, 2049, 2056, 2062,
        2069, 2075, 2082, 2088, 2095
    };
    
    void picManager();
    ofImage currentPic;


    void citySelect();
    
    ofRectangle leftArrowBox;
    ofRectangle rightArrowBox;
    
    ofVec2f leftTriangleA;
    ofVec2f leftTriangleB;
    ofVec2f leftTriangleC;

    ofVec2f rightTriangleA;
    ofVec2f rightTriangleB;
    ofVec2f rightTriangleC;
    
    ofTrueTypeFont cityFont;
    vector<string> cities;
    int currentCityIndex;
    
    ofTrueTypeFont titleFont;
    ofTrueTypeFont worldMessageFont;
    
    vector<string> worldMessages;
    ofRectangle worldSwapButton;
    int currentWorldIndex;
    
    void worldMessageDisplay();
    void titleDisplay();


    
    
    //Eray
    
    void loadImageForYear(std::string condition);
    std::string convertWorld(const std::string& input);
    void drawCityImages();
    void initCityBools();
    
    ofImage cityImgMexico;
    ofImage cityImgNY;
    ofImage cityImgCairo;
    bool bShowImageMexico;
    bool bShowImageNY;
    bool bShowImageCairo;

    
//    float pollutionData[200] = {
//        0.01373, 0.00695, 0.00340, 0.00154, 0.00059, 0.00014, 0.00000, 0.00006, 0.00027, 0.00059, 0.00101, 0.00151, 0.00207, 0.00270, 0.00338, 0.00410, 0.00486, 0.00564, 0.00644, 0.00727, 0.00810, 0.00895, 0.00981, 0.01067, 0.01154, 0.01241, 0.01329, 0.01418, 0.01508, 0.01598, 0.01690, 0.01783, 0.01877, 0.01972, 0.02069, 0.02168, 0.02269, 0.02372, 0.02478, 0.02586, 0.02697, 0.02812, 0.02929, 0.03050, 0.03175, 0.03303, 0.03436, 0.03573, 0.03714, 0.03860, 0.04012, 0.04169, 0.04331, 0.04499, 0.04673, 0.04854, 0.05041, 0.05234, 0.05435, 0.05643, 0.05858, 0.06081, 0.06311, 0.06550, 0.06797, 0.07054, 0.07319, 0.07594, 0.07878, 0.08173, 0.08478, 0.08795, 0.09127, 0.09478, 0.09848, 0.10238, 0.10649, 0.11082, 0.11540, 0.12023, 0.12534, 0.13074, 0.13643, 0.14244, 0.14877, 0.15543, 0.16243, 0.16979, 0.17750, 0.18558, 0.19403, 0.20286, 0.21208, 0.22170, 0.23172, 0.24214, 0.25298, 0.26425, 0.27594, 0.28808, 0.30067, 0.31373, 0.32727, 0.34132, 0.35591, 0.37107, 0.38683, 0.40321, 0.42025, 0.43798, 0.45643, 0.47563, 0.49562, 0.51641, 0.53801, 0.56041, 0.58362, 0.60759, 0.63230, 0.65769, 0.68369, 0.71022, 0.73714, 0.76434, 0.79166, 0.81889, 0.84573, 0.87180, 0.89668, 0.91990, 0.94098, 0.95944, 0.97486, 0.98687, 0.99517, 0.99958, 1.00000, 0.99644, 0.98901, 0.97791, 0.96342, 0.94585, 0.92558, 0.90297, 0.87843, 0.85233, 0.82503, 0.79686, 0.76814, 0.73912, 0.71004, 0.68111, 0.65248, 0.62429, 0.59666, 0.56967, 0.54340, 0.51790, 0.49322, 0.46940, 0.44645, 0.42440, 0.40323, 0.38296, 0.36357, 0.34505, 0.32738, 0.31053, 0.29448, 0.27921, 0.26468, 0.25086, 0.23773, 0.22525, 0.21340, 0.20215, 0.19146, 0.18133, 0.17170, 0.16257, 0.15391, 0.14569, 0.13789, 0.13050, 0.12349, 0.11684, 0.11053, 0.10455, 0.09888, 0.09351, 0.08842, 0.08359, 0.07906, 0.07483, 0.07083, 0.06705, 0.06345, 0.06003, 0.05678, 0.05368
//    };

//
//    //pop
//    float populationData[200] = {
//        0.00132, 0.00313, 0.00552, 0.00817, 0.01091, 0.01365, 0.01636, 0.01904, 0.02168, 0.02430, 0.02691, 0.02950, 0.03208, 0.03466, 0.03726, 0.03986, 0.04249, 0.04514, 0.04781, 0.05052, 0.05326, 0.05603, 0.05884, 0.06170, 0.06460, 0.06755, 0.07055, 0.07360, 0.07671, 0.07989, 0.08313, 0.08642, 0.08976, 0.09315, 0.09659, 0.10010, 0.10367, 0.10731, 0.11101, 0.11478, 0.11863, 0.12527, 0.13183, 0.13841, 0.14506, 0.15181, 0.15866, 0.16565, 0.17279, 0.18008, 0.18754, 0.19517, 0.20297, 0.21096, 0.21913, 0.22749, 0.23604, 0.24474, 0.25359, 0.26260, 0.27178, 0.28113, 0.29066, 0.30037, 0.31028, 0.32038, 0.33064, 0.34105, 0.35157, 0.36220, 0.37294, 0.38379, 0.39482, 0.40604, 0.41743, 0.42891, 0.44047, 0.45213, 0.46387, 0.47572, 0.48765, 0.49967, 0.51178, 0.52397, 0.53624, 0.54864, 0.56116, 0.57380, 0.58650, 0.59924, 0.61201, 0.62482, 0.63765, 0.65050, 0.66336, 0.67623, 0.68908, 0.70192, 0.71473, 0.72752, 0.74033, 0.75320, 0.76611, 0.77906, 0.79204, 0.80503, 0.81802, 0.83101, 0.84374, 0.85623, 0.86857, 0.88080, 0.89294, 0.90497, 0.91672, 0.92776, 0.93837, 0.94857, 0.95835, 0.96782, 0.97663, 0.98357, 0.98907, 0.99337, 0.99651, 0.99860, 0.99974, 1.00000, 0.99964, 0.99907, 0.99816, 0.99685, 0.99497, 0.99235, 0.98898, 0.98466, 0.97926, 0.97287, 0.96561, 0.95758, 0.94887, 0.93960, 0.92990, 0.91985, 0.90953, 0.89901, 0.88834, 0.87756, 0.86673, 0.85575, 0.84446, 0.83297, 0.82133, 0.80958, 0.79788, 0.78624, 0.77469, 0.76327, 0.75197, 0.74079, 0.72976, 0.71887, 0.70797, 0.69712, 0.68636, 0.67571, 0.66519, 0.65493, 0.64492, 0.63515, 0.62562, 0.61633, 0.60729, 0.59848, 0.59003, 0.58189, 0.57405, 0.56650, 0.55927, 0.55233, 0.54560, 0.53904, 0.53266, 0.52639, 0.52021, 0.51407, 0.50790, 0.50173, 0.49557, 0.48945, 0.48338, 0.47740, 0.47151, 0.46573, 0.46006, 0.45441, 0.44883, 0.44330
//    };
//
//    float foodData[200] = {
//        0.16558, 0.20948, 0.19683, 0.19865, 0.20282, 0.20732, 0.21191, 0.21660, 0.22152, 0.22669, 0.23213, 0.23782, 0.24377, 0.24998, 0.25696, 0.26456, 0.27229, 0.28010, 0.28788, 0.29580, 0.30391, 0.31221, 0.32073, 0.32946, 0.33842, 0.34761, 0.35705, 0.36673, 0.37667, 0.38686, 0.39731, 0.40803, 0.41908, 0.43045, 0.44214, 0.45417, 0.46653, 0.47923, 0.49227, 0.50565, 0.51939, 0.53347, 0.53924, 0.54853, 0.55769, 0.56691, 0.57646, 0.58624, 0.59623, 0.60635, 0.61662, 0.62702, 0.63756, 0.64821, 0.65897, 0.66983, 0.68109, 0.69474, 0.70722, 0.72005, 0.73262, 0.74523, 0.75775, 0.77024, 0.78267, 0.79503, 0.80692, 0.81801, 0.82239, 0.82341, 0.82759, 0.83182, 0.83601, 0.84006, 0.84397, 0.84780, 0.85176, 0.85580, 0.86020, 0.86483, 0.86935, 0.87383, 0.87826, 0.88265, 0.88702, 0.89137, 0.89561, 0.89975, 0.90379, 0.90785, 0.91194, 0.91606, 0.92021, 0.92439, 0.92916, 0.93318, 0.93598, 0.94205, 0.94783, 0.95339, 0.95880, 0.96398, 0.96890, 0.97361, 0.97812, 0.98246, 0.98663, 0.99092, 0.99671, 0.99957, 1.00000, 0.99967, 0.99828, 0.99582, 0.99232, 0.98570, 0.97602, 0.96516, 0.95291, 0.93922, 0.92400, 0.90329, 0.86643, 0.82601, 0.78608, 0.74339, 0.69982, 0.65560, 0.61208, 0.57030, 0.53090, 0.49283, 0.45618, 0.41470, 0.37594, 0.33880, 0.30411, 0.27249, 0.24383, 0.21859, 0.19694, 0.17896, 0.16468, 0.15404, 0.14695, 0.14213, 0.13830, 0.13594, 0.13513, 0.12826, 0.11034, 0.09887, 0.08867, 0.07923, 0.07051, 0.06241, 0.05493, 0.04804, 0.04174, 0.03599, 0.03070, 0.02588, 0.02153, 0.01775, 0.01450, 0.01159, 0.00908, 0.00697, 0.00517, 0.00364, 0.00240, 0.00144, 0.00075, 0.00032, 0.00011, 0.00000, 0.00004, 0.00024, 0.00059, 0.00105, 0.00161, 0.00236, 0.00329, 0.00437, 0.00566, 0.00715, 0.00868, 0.01025, 0.01193, 0.01378, 0.01582, 0.01806, 0.02050, 0.02313, 0.02593, 0.02888, 0.03209, 0.03550, 0.03908, 0.04284
//    };
//
//    float industryData[200] = {
//        0.08087, 0.08283, 0.08746, 0.09135, 0.09495, 0.09839, 0.10173, 0.10511, 0.10856, 0.11209, 0.11570, 0.11941, 0.12322, 0.12713, 0.13116, 0.13532, 0.13960, 0.14402, 0.14857, 0.15326, 0.15809, 0.16307, 0.16819, 0.17346, 0.17889, 0.18447, 0.19021, 0.19611, 0.20218, 0.20841, 0.21482, 0.22139, 0.22815, 0.23511, 0.24226, 0.24961, 0.25716, 0.26493, 0.27290, 0.28109, 0.28950, 0.29813, 0.30482, 0.31171, 0.31878, 0.32602, 0.33339, 0.34089, 0.34851, 0.35624, 0.36407, 0.37202, 0.38007, 0.38822, 0.39647, 0.40482, 0.41327, 0.42182, 0.43057, 0.43948, 0.44855, 0.45777, 0.46714, 0.47665, 0.48630, 0.49607, 0.50596, 0.51590, 0.52592, 0.53594, 0.54591, 0.55592, 0.56597, 0.57601, 0.58605, 0.59608, 0.60619, 0.61638, 0.62666, 0.63702, 0.64747, 0.65801, 0.66864, 0.67935, 0.69015, 0.70104, 0.71198, 0.72296, 0.73398, 0.74511, 0.75634, 0.76769, 0.77915, 0.79073, 0.80243, 0.81427, 0.82624, 0.83832, 0.85058, 0.86302, 0.87564, 0.88837, 0.90119, 0.91409, 0.92707, 0.94015, 0.95332, 0.96659, 0.97569, 0.98116, 0.98642, 0.99125, 0.99552, 0.99916, 1.00000, 0.99264, 0.98481, 0.97604, 0.96601, 0.95480, 0.93298, 0.88011, 0.82954, 0.78093, 0.73441, 0.69025, 0.64857, 0.60944, 0.57603, 0.55146, 0.52769, 0.50489, 0.48316, 0.46225, 0.44186, 0.42207, 0.40298, 0.38470, 0.36719, 0.35030, 0.33407, 0.31862, 0.30392, 0.28991, 0.27655, 0.26383, 0.25172, 0.24019, 0.22921, 0.21875, 0.20879, 0.19928, 0.19021, 0.18156, 0.17330, 0.16539, 0.15781, 0.15054, 0.14357, 0.13684, 0.13037, 0.12414, 0.11816, 0.11244, 0.10697, 0.10173, 0.09671, 0.09191, 0.08727, 0.08279, 0.07848, 0.07434, 0.07034, 0.06649, 0.06276, 0.05917, 0.05569, 0.05235, 0.04912, 0.04601, 0.04301, 0.04012, 0.03735, 0.03468, 0.03213, 0.02953, 0.02693, 0.02439, 0.02194, 0.01959, 0.01735, 0.01521, 0.01317, 0.01122, 0.00937, 0.00760, 0.00593, 0.00433, 0.00282, 0.00137
//    };
//
//    float resourceData[200] = {
//        1.00000, 0.99967, 0.99933, 0.99897, 0.99860, 0.99821, 0.99781, 0.99739, 0.99696, 0.99652, 0.99606, 0.99558, 0.99509, 0.99458, 0.99405, 0.99350, 0.99294, 0.99236, 0.99175, 0.99113, 0.99048, 0.98981, 0.98912, 0.98840, 0.98766, 0.98689, 0.98609, 0.98527, 0.98442, 0.98353, 0.98261, 0.98166, 0.98068, 0.97966, 0.97861, 0.97751, 0.97638, 0.97520, 0.97398, 0.97272, 0.97141, 0.97005, 0.96864, 0.96718, 0.96566, 0.96409, 0.96246, 0.96077, 0.95902, 0.95721, 0.95533, 0.95338, 0.95136, 0.94927, 0.94710, 0.94485, 0.94252, 0.94010, 0.93760, 0.93501, 0.93232, 0.92954, 0.92665, 0.92366, 0.92056, 0.91735, 0.91401, 0.91047, 0.90674, 0.90282, 0.89868, 0.89434, 0.88978, 0.88500, 0.87999, 0.87475, 0.86926, 0.86353, 0.85755, 0.85130, 0.84478, 0.83799, 0.83091, 0.82354, 0.81588, 0.80790, 0.79961, 0.79099, 0.78204, 0.77275, 0.76312, 0.75312, 0.74276, 0.73202, 0.72090, 0.70939, 0.69747, 0.68514, 0.67239, 0.65920, 0.64558, 0.63150, 0.61696, 0.60194, 0.58645, 0.57046, 0.55397, 0.53697, 0.51944, 0.50149, 0.48320, 0.46458, 0.44564, 0.42638, 0.40684, 0.38707, 0.36730, 0.34756, 0.32788, 0.30831, 0.28889, 0.26992, 0.25228, 0.23594, 0.22089, 0.20708, 0.19448, 0.18302, 0.17266, 0.16323, 0.15449, 0.14643, 0.13901, 0.13206, 0.12541, 0.11905, 0.11297, 0.10717, 0.10164, 0.09637, 0.09136, 0.08659, 0.08206, 0.07775, 0.07366, 0.06977, 0.06608, 0.06257, 0.05924, 0.05607, 0.05306, 0.05020, 0.04748, 0.04490, 0.04244, 0.04011, 0.03789, 0.03579, 0.03378, 0.03188, 0.03007, 0.02835, 0.02672, 0.02517, 0.02369, 0.02229, 0.02096, 0.01969, 0.01849, 0.01734, 0.01625, 0.01522, 0.01424, 0.01330, 0.01241, 0.01156, 0.01076, 0.00999, 0.00926, 0.00857, 0.00791, 0.00729, 0.00669, 0.00613, 0.00559, 0.00507, 0.00459, 0.00412, 0.00369, 0.00328, 0.00289, 0.00252, 0.00217, 0.00184, 0.00153, 0.00124, 0.00096, 0.00070, 0.00045, 0.00022
//    };
    
};
