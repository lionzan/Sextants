/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

#include <TimeLib.h>

#include "Fonts.h"

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" 
#include <ds3231.h>

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

// Initialize the OLED display using Wire library
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
OLEDDisplayUi ui ( &display );

// modes
#define M_STARFINDER 0x00
#define M_TAKE_SIGHT 0x01
#define M_POS_FIX    0x02
#define M_SETUP      0x03

#define NUMBUTTONS 4    // 3 physical buttons + 1 combination of 2
#define DEBOUNCE 5      // how many ms to debounce, 5+ ms is usually plenty
#define BUZZPIN GPIO_NUM_5       // buzzer pin
#define BUZZLOW 659     // 5th note of A 440Hz
#define BUZZHIGH 880    // octave of A 440Hz

// buttons
#define BUTT_CONFIRM 0x00 // 00
#define BUTT_DOWN    0x01 // 01
#define BUTT_UP      0x02 // 10
#define BUTT_JOINT   0x03 // 11

// buttons ports
#define BUTT_GPIO_CONFIRM GPIO_NUM_15 // was 3
#define BUTT_GPIO_DOWN    GPIO_NUM_2  // was 4
#define BUTT_GPIO_UP      GPIO_NUM_4  // was 5

// buttons status
#define ST_RELEASED   0x00 // 00
#define ST_PRESSED    0x01 // 01
#define ST_ACTIVE     0x02 // 10
#define ST_LONGPRESS  0x03 // 11
#define ST_INHIBITED  0xFF

// delays for button status
#define INHIBIT_JOINT 200     //delay to become a genuine short press, not possible to do joint anymore
#define LONG_START    500     //delay to become a long press
#define MAXINT 4294967295 // the max value of an int after which millis rolls over

// button actions: bits 8-5 button, bit 4 isStart, bits 3-1 startus
#define EVENT_END_PRESS   0x01 //0001 relevant
#define EVENT_END_SHORT   0x02 //0010 relevant and same action as END_PRESS
#define EVENT_END_LONG    0x03 //0011 relevant
#define EVENT_START_SHORT 0x06 //0110
#define EVENT_START_LONG  0x07 //0111 relevant

// alternative button actions: bit 1 (SHORT=0; LONG=1) , bit 0 (0=START; 1=END)
#define ACT_SHORT       0x00 // 00
#define ACT_START_LONG  0x01 // 01
#define ACT_END_LONG    0x02 // 10

ts t; //ts is a struct findable in ds3231.h

byte mode = 0x00;
// define modes names
char m00[]="Starfinder";
char m01[]="Take Sight";
char m02[]="Fix Position";
char m03[]="Setup";
char* modes [] = { m00, m01, m02, m03 };

//define the buttons
byte buttons[] = {BUTT_GPIO_CONFIRM, BUTT_GPIO_DOWN, BUTT_GPIO_CONFIRM}; // button pins

//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte buttStatus[NUMBUTTONS] = {ST_RELEASED,ST_RELEASED,ST_RELEASED,ST_RELEASED};
unsigned long pressedSince[NUMBUTTONS] = {0,0,0,0};

// main variables
byte horizonElev[2];     //deg, min
byte horizonTilt[2];     //deg, min
byte eyeHeight[2];       //m, cm
byte posLat[3];          //deg, min, sign{W,E}
byte posLon[3];          //deg, min, sign{N,S}
byte heading[1];         //deg
byte speedKn[2];         //kn, tens of knot
long timeNow;            //time now from J2000
byte timeGMT[3];         //hr, min, sec for visualisation

// stars data (fixed, could stay in EEPROM)
float starSHA[100];      //up to 100 stars SHA
float starDecl[100];
float starAppMag[100];
byte starIdent[100];     // up to 100 poiners to char singleStarIdent[6];
byte starName[100];      // up to 110 pointers to chars singleStarName[13];

// planets data (fixed, could stay in EEPROM)
float planetIncl[9];     // 9 planets Inclination (deg)
float planetLAN[9];      // 9 planets Longitude Ascensino Node (deg)
float planetLonPeri[9];  // 9 planets Longitude of Perihelion (deg)
float planetMeanDist[9]; // 9 planets Mean Distance (au)
float planetDailyMot[9]; // 9 planets Daily Motion (deg)
float planetEccen[9];    // 9 planets Eccentricity (1)
float planetMeanLong[9]; // 9 planets Mean Longitude (deg)
byte planetName[9];      // 9 planets pointers to Names as char[10]

// moon data (fixed, could stay in EEPROM)
//...

// asters current position
float starLon[100];      // up to 100 stars current longitude
float starLat[100];      // up to 100 stars current latitude
byte starSort[100];      // index of visible stars sorted by magnitude
float planetLon[10];     // 9 planets + moon Longitude
float planetLat[10];     // 9 planets + moon Latitude

// sights
byte sights[100];        // up to 100 sights, structure int index, float(or unsigned integer? seconds) sightTime, float sightEle, float sightAzi (2+4+4+4)
float sightTime;         // sight Time (or integer seconds from J2000?)
float sightEle;          // sight Elevation
float sightAzi;          // sight Azimuth
byte sightAster;         // star index or planet index+100

// action table. Each action has 2 components: switch to mode (3 upper bytes) and perform procedure (5 lowefr bytes)
byte action[4][4][3] = {   // alternative solution with ACTION states
  { // current mode M_STARFINDER - find stars and sets star for Sight Taking
    { // button CONFIRM
      (M_TAKE_SIGHT << 5), // PRESS      - switch to M_TAKE_SIGHT with selected star
      0xFF, // START_LONG - nil
      (M_POS_FIX << 5)  // END_LONG   - switch to M_POS_FIX
    },
    { // button pressed UP
      0xFF, // SHORT       - scroll aross stars or increase n. of stars in list
      0xFF, // START_LONG  - start scrolling aross stars or increasing n. of stars in list
      0xFF  // END_LONG    - finish scrolling aross stars or increasing n. of stars in list
    },
    { // button pressed DOWN
      0xFF, // SHORT       - scroll aross stars or decrease n. of stars in list
      0xFF, // START_LONG  - start scrolling aross stars or decreasing n. of stars in list
      0xFF  // END_LONG    - finish scrolling aross stars or decreasing n. of stars in list
    },
    { // button pressed JOINT
      0xFF, // SHORT       - switch between selection of stars and selection of number of stars included
      0xFF, // START_LONG  - nil
      (M_SETUP << 5) // END_LONG   - switch to M_SETUP
    }
  },
  { // current mode M_TAKE_SIGHT
    { // button CONFIRM
      (M_STARFINDER << 5), // SHORT      - take sight and switch to M_STARFINDER
      0xFF, // START_LONG - nil
      0xFF  // END_LONG   - nil
    },
    { // button pressed UP
      0xFF, // SHORT      - increase reading stepwise and update crosshatch
      0xFF, // START_LONG - start increasing reading fast
      0xFF  // END_LONG   - stop increasing reading fast
    },
    { // button pressed DOWN
      0xFF, // SHORT      - decrease reading stepwise and update crosshatch
      0xFF, // START_LONG - start increasing reading fast
      0xFF  // END_LONG   - stop increasing reading fast
    },
    { // button pressed JOINT
      0xFF, // SHORT      - swap Elevation/Azimuth/Star
      0xFF, // START_LONG - nil
      (M_STARFINDER << 5) // END_LONG   - exit without changes, switch to Starfinder mode (to find another star)
    }
  },
  { // current mode M_POS_FIX
    { // button CONFIRM
      (M_STARFINDER << 5), // PRESS      - keep readings and switch to M_STARFINDER
      0xFF, // START_LONG - nil
      (M_STARFINDER << 5)  // END_LONG   - store readings, reset readings, set estimated position to current, compute and set avg. heading and speed since last reading, switch to Starfinder
    },
    { // button pressed UP
      0xFF, // PRESS      - scroll across results
      0xFF, // START_LONG - scroll fast through results
      0xFF  // END_LONG   - stop scrolling through results
    },
    { // button pressed DOWN
      0xFF, // PRESS      - scroll across results
      0xFF, // START_LONG - scroll fast through results
      0xFF  // END_LONG   - stop scrolling through results
    },
    { // button pressed JOINT
      0xFF, // PRESS      -
      0xFF, // START_LONG -
      (M_SETUP << 5) // END_LONG   - switch to SETUP
    }
  },
  { // current mode M_SETUP
    { // button CONFIRM
      0xFF, // PRESS      - confirm and switch to next item in group
      0xFF, // START_LONG - nil
      0xFF  // END_LONG   - switch to last Setup screen to Confirm or Cancel
    },
    { // button pressed UP
      0xFF, // PRESS      - increase value or scroll to next group
      0xFF, // START_LONG - start fast increase value
      0xFF  // END_LONG   - stop fast increase value or scroll to next group
    },
    { // button pressed DOWN
      0xFF, // PRESS      - decrease value or scroll to previus group
      0xFF, // START_LONG - start fast decrease value
      0xFF  // END_LONG   - stop fast decrease value or scroll to previous group
    },
    { // button pressed JOINT
      0xFF, // PRESS      - AVAILABLE
      0xFF, // START_LONG - AVAILABLE
      0xFF // END_LONG   - AVAILABLE
    }
  }
};


int screenW = 128;
int screenH = 64;
int centerX = screenW/2;
int centerY = screenH/2;  

int frame = 0;
int select = 0;
int selectPerFrame [] = {0,0,0,3,0,8,4,8};
int millisNow = millis();
boolean selBar = false;

unsigned char PROGMEM buttonUp[] =
{ B11111100,
  B10000010,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000010,
  B11111100};
unsigned char PROGMEM buttonDown[] =
{ B00111111,
  B01000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B10000001,
  B01000001,
  B00111111};
unsigned char PROGMEM scrollUp[] =
{ B11111100,
  B10000010,
  B10110001,
  B10111101,
  B10111101,
  B10110001,
  B10000010,
  B11111100};
unsigned char PROGMEM scrollDown[] =
{ B00111111,
  B01000001,
  B10001101,
  B10111101,
  B10111101,
  B10001101,
  B01000001,
  B00111111};
unsigned char PROGMEM space[] =
{ B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000};
unsigned char PROGMEM circleArrowUp[] =
{ B00111100,
  B01000010,
  B10110001,
  B10111101,
  B10111101,
  B10110001,
  B01000010,
  B00111100};
unsigned char PROGMEM circleArrowDown[] =
{ B00111100,
  B01000010,
  B10001101,
  B10111101,
  B10111101,
  B10001101,
  B01000010,
  B00111100};
unsigned char PROGMEM arrowUp[] =
{ B00100000,
  B00010000,
  B00001000,
  B00000100,
  B00001000,
  B00010000,
  B00100000,
  B00000000};
unsigned char PROGMEM arrowDown[] =
{ B00000100,
  B00001000,
  B00010000,
  B00100000,
  B00010000,
  B00001000,
  B00000100,
  B00000000};
unsigned char PROGMEM rotRight[] =
{ B00000000,
  B00110000,
  B00001000,
  B00001000,
  B00101000,
  B00110000,
  B00111000,
  B00000000};
unsigned char PROGMEM rotLeft[] =
{ B00000000,
  B00001000,
  B00010000,
  B00010000,
  B00010100,
  B00001100,
  B00011100,
  B00000000};
unsigned char PROGMEM checkY[] =
{ B00000000,
  B00010000,
  B00100000,
  B01100000,
  B00011000,
  B00000100,
  B00000010,
  B00000000};
unsigned char PROGMEM checkX[] =
{ B00000000,
  B00000000,
  B01100100,
  B00011000,
  B00100100,
  B01000010,
  B00000000,
  B00000000};

// utility function for digital clock display: prints leading 0
String twoDigits(int digits){
  if(digits < 10) {
    String i = '0'+String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {

}


void drawPointer (OLEDDisplay *display, int16_t x, int16_t y, int16_t dir, int16_t r = 10, int8_t s = 2) {
  float dirRad = DEG_TO_RAD * dir;
  int16_t ax, ay, bx, by, cx, cy;
  ax = x + cos(dirRad) * (r + s);
  ay = y - sin(dirRad) * (r + s);
  bx = x + cos(dirRad) * (r - s) + sin(dirRad) * s;
  by = y - sin(dirRad) * (r - s) + cos(dirRad) * s;
  cx = x + cos(dirRad) * (r - s) - sin(dirRad) * s;
  cy = y - sin(dirRad) * (r - s) - cos(dirRad) * s;
  display->drawLine(ax,ay,bx,by);
  display->drawLine(ax,ay,cx,cy);
//  display->drawLine(cx,cy,bx,by);
}

void drawScrollBar (OLEDDisplay *display, int16_t sec, int16_t tot, boolean active) {
  int16_t secH = (screenH-16)/tot;
  display->setColor(WHITE);  
  display->drawRect(0,8,8,screenH-16);
  display->fillRect(0, 8+sec*secH, 8, secH);
  if (active) {
    display->setColor(INVERSE);
    display->fillRect(1,9,6,screenH-18);
    display->setColor(WHITE);  
    display->drawFastImage(0,0,8,8,scrollUp);
    display->drawFastImage(0,screenH-8,8,8,scrollDown);
  }
}

void drawData (OLEDDisplay *display, int16_t x, int16_t y, String title, String value) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->drawString(x, y-16, title);
  display->drawString(x, y, value);
}

void drawButton (OLEDDisplay *display, int16_t x, int16_t y, unsigned char* symbolUp = space, unsigned char* symbolDown = space) {
  display->drawFastImage(x-8,y-8-4,16,8,buttonUp);
  display->drawFastImage(x-8,y+8-4,16,8,buttonDown);
  display->drawFastImage(x-4,y-8-4,8,8,symbolUp);
  display->drawFastImage(x-4,y+8-4,8,8,symbolDown);
  display->setColor(INVERSE);
  display->fillRect(x-8,y-4,16,8);
  display->setColor(WHITE);  
}

void cancelButton (OLEDDisplay *display, int16_t x, int16_t y) {
  display->setColor(BLACK);
  display->fillRect(x-8,y-8-4,16,8);
  display->fillRect(x-8,y+8-4,16,8);
  display->setColor(INVERSE);
  display->fillRect(x-8,y-4,16,8);
  display->setColor(WHITE);  
}

void starfinderFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Starfinder" );
  // star
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Betelgeuse" );
  //azimuth measure
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,screenH-8,"A 155º32'");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW,screenH-8,"E 65º23'");
  //target
  display->drawCircle(centerX, centerY, 3);
  //star crosshatch
  display->drawLine(centerX-8,centerY,centerX-5,centerY);
  display->drawLine(centerX+5,centerY,centerX+8,centerY);
  display->drawLine(centerX,centerY-8,centerX,centerY-5);
  display->drawLine(centerX,centerY+5,centerX,centerY+8);
  //direction
  drawPointer(display, centerX, centerY, 315, 20, 4);
}

void takeSightFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Take Sight" );
  // star
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Betelgeuse" );
  //azimuth/elevation
  display->fillCircle(centerX-4,screenH-3,2);
  display->drawCircle(centerX+4,screenH-3,2);
  //azimuth measure
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,screenH-8,"A 155º32'");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW,screenH-8,"E 65º23'");
  //horizon line
  display->drawLine(0, centerY, screenW,  centerY);
  //azimuth line
  display->drawLine(centerX,centerY-5,centerX,centerY+5);
}

void positionFixFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Position Fix" );
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Readings" );
  //column titles
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,10,"Star");
  display->drawString(screenW/2,10,"Azi");
  display->drawString(screenW*3/4,10,"Ele");
  //readings
  display->drawString(0,20,"Betelgeuse");
  display->drawString(screenW/2,20,"155º25'");
  display->drawString(screenW*3/4,20,"45º56'");
  display->drawString(0,30,"Riegel");
  display->drawString(screenW/2,30,"285º51'");
  display->drawString(screenW*3/4,30,"25º12'");  
}

void eyeHeightSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  if (select == selectPerFrame[frame]-1) {
    iconUp = checkY;
    iconDown = checkX;
  }
  drawScrollBar(display, 0, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "Eye Height Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Estimate" );
  //buttons
  drawData (display, 32, centerY, "Eye Height", "5.2m");
  drawData (display, 64, centerY, "", "OK");
  if (select!=0) {drawButton (display, 32+32*(select-1), centerY, iconUp, iconDown);}
}

void zeroSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawScrollBar(display, 1, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "Zero Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Align Horizon" );
  //azimuth/elevation
  display->fillCircle(centerX-4,screenH-3,2);
  display->drawCircle(centerX+4,screenH-3,2);
  //azimuth measure
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8,screenH-8,"Height");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW,screenH-8,"Tilt");
  //horizon line
  display->drawLine(0, centerY, screenW,  centerY);
  //azimuth line
  display->drawLine(centerX,centerY-5,centerX,centerY+5);
}

void latLonSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  if (select == selectPerFrame[frame]-1) {
    iconUp = checkY;
    iconDown = checkX;
  }
  drawScrollBar(display, 2, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "Lat/Lon Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Estimate" );
  //buttons
  drawData (display, 16, centerY, "Lat.", "55º");
  drawData (display, 32, centerY, "", "37'");
  drawData (display, 48, centerY, "", "N");
  drawData (display, 64, centerY, "Lon.", "05º");
  drawData (display, 80, centerY, "", "12'");
  drawData (display, 96, centerY, "", "E");
  drawData (display, 112, centerY, "", "OK");
  if (select!=0) {drawButton (display, 16+16*(select-1), centerY, iconUp, iconDown);}
}

void headingSpeedSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  if (select == selectPerFrame[frame]-1) {
    iconUp = checkY;
    iconDown = checkX;
  }
  drawScrollBar(display, 3, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "Heading/Speed Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Estimate" );
  //buttons
  drawData (display, 32, centerY, "Heading", "215º");
  drawData (display, 64, centerY, "Speed", "4.3kt");
  drawData (display, 96, centerY, "", "OK");
  if (select!=0) {drawButton (display, 32+32*(select-1), centerY, iconUp, iconDown);}
}

void timeGMTSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  if (select == selectPerFrame[frame]-1) {
    iconUp = checkY;
    iconDown = checkX;
  }
  drawScrollBar(display, 4, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "GMT Time Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "OK resets sec." );
  //buttons
  drawData (display, 16, centerY, "Y", "19");
  drawData (display, 32, centerY, "M", "05");
  drawData (display, 48, centerY, "D", "27");
  drawData (display, 64, centerY, "h", "13");
  drawData (display, 80, centerY, "m", "56");
  drawData (display, 96, centerY, "s", "45");
  drawData (display, 112, centerY, "", "OK");
  if (select!=0) {drawButton (display, 16+16*(select-1), centerY, iconUp, iconDown);}
}


bool checkSwitches() {
  bool event = false;
  static byte previousState[NUMBUTTONS];
  static byte currentState[NUMBUTTONS];
  static unsigned long lastTime;
  byte index;
  if (millis() < lastTime) {
    // we wrapped around, skip this iteration
    lastTime = millis();
  }
  if ((lastTime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return event;
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lastTime = millis();
  for (index = 0; index < NUMBUTTONS; index++) {
    justPressed[index] = 0;       //when we start, we clear out the "just" indicators
    justReleased[index] = 0;
    if (index == 3) {
      currentState[index] = digitalRead(buttons[1]) || digitalRead(buttons[2]); // it's an AND of LOW which is when button pressed
    } else {
      currentState[index] = digitalRead(buttons[index]);   //read the button
    }
    if (currentState[index] == previousState[index]) {
      if ((pressed[index] == LOW) && (currentState[index] == LOW)) { //Watch out here LOW has opposite meanings
        // just pressed
        justPressed[index] = 1;
        event=true;
      }
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) {
        justReleased[index] = 1; // just released
        event=true;
      }
      pressed[index] = !currentState[index];  //remember, digital HIGH means NOT pressed
    }
    previousState[index] = currentState[index]; //keep a running tally of the buttons
  }
  return event;
}

byte checkEvent() {
  // update needed: respond to multiple events happening in the same cycle
//  Serial.println("enter checkEvent");
  byte event = 0xFF;
  if (checkSwitches()==true) {
    for (int i=BUTT_JOINT; i>=BUTT_CONFIRM; i--) {
      if ((justPressed[i]==1) && (buttStatus[i]!=ST_INHIBITED)) {
        buttStatus[i]=ST_PRESSED;
        pressedSince[i] = millis();
        if (i== BUTT_JOINT) {
          buttStatus[BUTT_UP] = ST_INHIBITED;
          buttStatus[BUTT_DOWN] = ST_INHIBITED;
          buttStatus[BUTT_JOINT] = ST_PRESSED;
        }
      } else if (justReleased[i]==1) {
        if (buttStatus[i]!=ST_INHIBITED) {
//          event = ((byte)i << 4) + ((byte)buttStatus[i]);
          if (buttStatus[i]!=ST_ACTIVE) { // because ST_ACTIVE has already returned ACT_SHORT
            event = ((byte)i << 4) + (((byte)buttStatus[i]) & 0x02); // ACT_SHORT or ACT_END_LONG
            Serial.println();
            Serial.print("justReleased button ");
            Serial.print(i);
            Serial.print(" - Status ");
            Serial.println(buttStatus[i]);
            Serial.println();
          }
          if (i==BUTT_UP) {
            buttStatus[BUTT_DOWN]=ST_RELEASED;
          } else if (i==BUTT_DOWN) {
            buttStatus[BUTT_UP]=ST_RELEASED;
          }
        }
        buttStatus[i]=ST_RELEASED;
      }
    }
  }
  // check status function of time
  for (int i=BUTT_JOINT; i>=BUTT_CONFIRM; i--) {
    if ((millis()-pressedSince[i]>LONG_START) && buttStatus[i]==ST_ACTIVE) {
      buttStatus[i] = ST_LONGPRESS;
//      event = ((byte)i << 4) + ((byte)buttStatus[i]) + 4;
      event = ((byte)i << 4) + ACT_START_LONG; // with new ACTION states
    } else if ((millis()-pressedSince[i]>INHIBIT_JOINT) && (buttStatus[i]==ST_PRESSED)) {
      buttStatus[i] = ST_ACTIVE;
//      event = ((byte)i << 4) + ((byte)buttStatus[i]) + 4;
      event = ((byte)i << 4) + ACT_SHORT; // with new ACTION states
      switch (i) {
        case BUTT_UP :{
          buttStatus[BUTT_JOINT] = ST_INHIBITED;
          buttStatus[BUTT_DOWN] = ST_INHIBITED;
          break;
        }
        case BUTT_DOWN :{
          buttStatus[BUTT_JOINT] = ST_INHIBITED;
          buttStatus[BUTT_UP] = ST_INHIBITED;
          break;
        }
      }
    }
  }
//  Serial.println("exit checkEvent");
  return event;
}

void buzz (int freq, int len) {
//  tone(BUZZPIN, freq);
//  delay(len);
//  noTone(BUZZPIN);
}


// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { starfinderFrame, takeSightFrame, positionFixFrame, eyeHeightSetFrame, zeroSetFrame, latLonSetFrame, headingSpeedSetFrame, timeGMTSetFrame };

// how many frames are there?
int frameCount = 8;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Wire.begin(); //start i2c (required for connection)
  DS3231_init(DS3231_INTCN); //register the ds3231 (DS3231_INTCN is the default address of ds3231, this is set by macro for no performance loss)
  // Make input & enable pull-up resistors on switch pins
  for (byte i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
    pinMode(BUZZPIN, OUTPUT); // Set buzzer - pin 9 as an output
  }

	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
//  ui.setActiveSymbol(activeSymbol);
//  ui.setInactiveSymbol(inactiveSymbol);

  ui.disableAutoTransition();
//  ui.setTimePerFrame(2000);
//  ui.setTimePerTransition(0);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
//  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
//  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
//  ui.setFrameAnimation(SLIDE_LEFT);

  ui.disableAllIndicators();

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  //ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  ui.switchToFrame(0);

  // Turn the display upside down
  display.flipScreenVertically();

  // high brightness
//    display.setContrast(100);
  
  // low brightness
//  display.setContrast(10, 50);

  //display.mirrorScreen();
  
  unsigned long secsSinceStart = millis();
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSinceStart - seventyYears * SECS_PER_HOUR;
  setTime(epoch);

}


void loop() {
  byte event;
  byte button;
  byte act;
  int freq = BUZZHIGH;

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    
    if (millis()-millisNow > 3000) {
      millisNow=millis();
      if (selectPerFrame[frame] == 0) {
        select = 0;
        frame = (frame+1)%frameCount;
      }
      else {
        select = (select+1)%selectPerFrame[frame];
        if (select==0) {frame = (frame+1)%frameCount;}
      }
      ui.switchToFrame(frame);
      if (select == 0) {selBar=!selBar;}
    }

  event = checkEvent();
//  Serial.println(event);
  if (event!=0xFF) {
    Serial.print("Event : ");
    Serial.print(event,BIN);
    DS3231_get(&t); //get the value and pass to the function the pointer to t, that make an lower memory fingerprint and faster execution than use return
  #ifdef CONFIG_UNIXTIME
    Serial.print(" Timestamp : ");
    Serial.println(t.unixtime);
  #endif
    button = (event >> 4);
    if (button == BUTT_JOINT) {
      freq = BUZZLOW;
    }
    Serial.print("Button : ");
    Serial.println(button);
    act = action[(byte)mode][(byte)event>>4][((byte)event & 0x03)];
    Serial.print ("Mode:");
    Serial.print(mode);
    Serial.print (" Button:");
    Serial.print (event>>4,HEX);
    Serial.print (" Action:");
    Serial.print ((event&0x03),HEX);
    Serial.print (" Action:");
    Serial.print (act,BIN);
    Serial.print (" New mode:");
    Serial.println (act>>5,HEX);
    if (act!=0xFF) {    mode = act>>5; }
  }
    
//    delay(remainingTimeBudget);

  }


}
