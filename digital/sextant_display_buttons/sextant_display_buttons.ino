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
#include <EEPROM.h>

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
#define BUZZPIN GPIO_NUM_15       // buzzer pin
#define BUZZLOW 659     // 5th note of A 440Hz
#define BUZZHIGH 880    // octave of A 440Hz

// buttons
#define BUTT_CONFIRM 0x00 // 00
#define BUTT_DOWN    0x01 // 01
#define BUTT_UP      0x02 // 10
#define BUTT_JOINT   0x03 // 11

// buttons ports
#define BUTT_GPIO_CONFIRM GPIO_NUM_5 
#define BUTT_GPIO_DOWN    GPIO_NUM_19  
#define BUTT_GPIO_UP      GPIO_NUM_18  

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

// button actions: bits 8-5 button, bit 4 isStart, bits 3-1 status
#define EVENT_END_PRESS   0x01 //0001 relevant
#define EVENT_END_SHORT   0x02 //0010 relevant and same action as END_PRESS
#define EVENT_END_LONG    0x03 //0011 relevant
#define EVENT_START_SHORT 0x06 //0110
#define EVENT_START_LONG  0x07 //0111 relevant

// alternative button actions: bit 1 (SHORT=0; LONG=1) , bit 0 (0=START; 1=END)
#define ACT_SHORT       0x00 // 00
#define ACT_START_LONG  0x01 // 01
#define ACT_END_LONG    0x02 // 10

typedef void (*actionFunction)();
actionFunction* actionFunctions;

ts t; //ts is a struct findable in ds3231.h

byte mode = 0x00;
// define modes names
char m00[]="Starfinder";
char m01[]="Take Sight";
char m02[]="Fix Position";
char m03[]="Setup";
char* modes [] = { m00, m01, m02, m03 };

String s01 = "Starfinder";
String s02 = "Take Sight";
String s03 = "Fix position";
String s04 = "Setup";

String strings [] = {s01, s02, s03, s04};

//define the buttons
byte buttons[] = {BUTT_GPIO_CONFIRM, BUTT_GPIO_DOWN, BUTT_GPIO_UP}; // button pins

//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte buttStatus[NUMBUTTONS] = {ST_RELEASED,ST_RELEASED,ST_RELEASED,ST_RELEASED};
unsigned long pressedSince[NUMBUTTONS] = {0,0,0,0};

// main variables - value, min, max
// STARFINDER mode
uint16_t iaster[] = {0, 0, 0}; // index of current aster in starfinder. MAX to be updated according to current list
// TAKE_SIGHT mode
float asterEle[] = {0.0,0,90};
float asterAzi[] = {0.0,0,360};
// POS_FIX  variables
uint16_t isight[] = {0, 0, 0}; // index of currrent sight. MAX to be updated according to current list
// SETUP variables
float eyeHeight[] = {1.5,0,99};     //
float horizonElev [] = {0,-90,90};     //
float horizonTilt[] = {0,-90,90};     //
float posLat[] = {51.473762,-90,90};     //
float posLon[] = {-0.224904,-180,180};     //
float heading[] = {32.0,0,360};         //
float speedKn[] = {4.0,0,20};         //
uint32_t timeNow[] = {631152000,0,2147483647};            //time now, seconds from J2000 , max size in uint32, approx to 2068
float* mainVariables [] = {eyeHeight, horizonElev, horizonTilt, posLat, posLon, heading, speedKn};

// stars data (fixed, could stay in EEPROM)
struct star {
  float sha; // SHA
  float dec; // declination
  float ama; // apparent magnitude
  char sid [6]; // star ID
  String sName; // star Name
};
star stars[100];

// planets data (fixed, could stay in EEPROM)
struct planet {
  float inc; // Inclination (deg)
  float lan; // Longitude Ascension Node (deg)
  float lpe; // Longitude of Perihelion (deg)
  float mdi; // Mean Distance (au)
  float dmo; // Daily Motion (deg)
  float pec; // Eccentricity (1)
  float pml; // Mean Longitude (deg)
  char pid [6]; // planet ID
  String pName; // Planet Name
};
planet planets[9];

// moon data (fixed, could stay in EEPROM)
//...

struct aster { // position of asters in sky Now
  uint32_t tas; // time of position
  float lon; // longitude
  float lat; // latitude
  int ast; // index of aster (9 planets + moon + stars)
};
aster asters [0]; // the currently visible asters sorted eastward (last aster in list is first West of us)

// sights
struct sight {
  uint32_t tsi; // time of sight
  float ele; // elevation
  float gaz; // geographical azimuth
  float lon; // aster longitude
  float lat; // aster latitude
  int ast; // index of aster
};
sight sights[0]; // last sights
// should be stored in EEPROM and downloaded via WiFi to computer for storage
// or stored on file in SD card

/* action table. 
Each action has 2 components: 
switch to mode (3 upper bytes) and 
perform procedure (5 lower bytes) */
byte action[4][4][3] = {   // alternative solution with ACTION states
  { // current mode M_STARFINDER - find stars and sets star for Sight Taking
    { // button CONFIRM
      (M_TAKE_SIGHT << 5), // PRESS      - switch to M_TAKE_SIGHT with selected star
      (M_POS_FIX << 5), // START_LONG - switch to M_POS_FIX
      0xFF  // END_LONG   - nil
    },
    { // button pressed UP
      0xFF, // SHORT       - scroll aross visible stars
      0xFF, // START_LONG  - start scrolling aross stars
      0xFF  // END_LONG    - finish scrolling aross stars
    },
    { // button pressed DOWN
      0xFF, // SHORT       - scroll aross visible stars
      0xFF, // START_LONG  - start scrolling aross stars
      0xFF  // END_LONG    - finish scrolling aross stars
    },
    { // button pressed JOINT
      0xFF, // SHORT       - select closest aster (then UP and DOWN scroll East and West in list of asters)
      (M_SETUP << 5), // START_LONG -switch to SETUP
      0xFF // END_LONG   - nil
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
      0xFF, // START_LONG - start increase reading fast
      0xFF  // END_LONG   - stop increase reading fast
    },
    { // button pressed DOWN
      0xFF, // SHORT      - decrease reading stepwise and update crosshatch
      0xFF, // START_LONG - start increase reading fast
      0xFF  // END_LONG   - stop increase reading fast
    },
    { // button pressed JOINT
      0xFF, // SHORT      - swap Elevation/Azimuth/Star
      (M_STARFINDER << 5), // START_LONG - exit without changes, switch to Starfinder mode (to find another star)
      0xFF // END_LONG   - nil
    }
  },
  { // current mode M_POS_FIX
    { // button CONFIRM
      (M_STARFINDER << 5), // PRESS      - keep readings and switch to M_STARFINDER
      (M_STARFINDER << 5), // START_LONG - store readings, reset readings, set estimated position to current, compute and set avg. heading and speed since last reading, switch to Starfinder
      0xFF  // END_LONG   - nil
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
      (M_SETUP << 5), // START_LONG -switch to SETUP
      0xFF // END_LONG   - nil
    }
  },
  { // current mode M_SETUP - eyeHeightSet
    { // button CONFIRM
      (0x7<<5)|0x01, // PRESS      - switch to next item in group
      0xFF, // START_LONG - nil
      (0x7<<5)|0x01  // END_LONG   - nil
    },
    { // button pressed UP
      (0x7<<5)|0x02, // PRESS      - increase value or scroll to next group
      (0x7<<5)|0x03, // START_LONG - start fast increase value
      (0x7<<5)|0x04  // END_LONG   - stop fast increase value or scroll to next group
    },
    { // button pressed DOWN
      (0x7<<5)|0x05, // PRESS      - decrease value or scroll to previus group
      (0x7<<5)|0x06, // START_LONG - start fast decrease value
      (0x7<<5)|0x07  // END_LONG   - stop fast decrease value or scroll to previous group
    },
    { // button pressed JOINT
      0xFF, // PRESS      - AVAILABLE
      (M_STARFINDER << 5), // START_LONG - switch back to STARFINDER
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
int selectInFrame = 0;
int selectPerFrame [] = {2,3,7,3,7}; // including scroll bar
int frameFirstIndex[] = {0,2,5,12,15};
//float* mainVariables [] = {eyeHeight, horizonElev, horizonTilt, posLat, posLon, heading, speedKn, timeNow};
//eyeHeightSetFrame, horizonSetFrame, latLonSetFrame, headingSpeedSetFrame, timeGMTSetFrame
int varIndex;
int varSelect [] = {-1, 0, -1, 1, 2, -1, 3,  3,  3, 4,  4,  4, -1, 5,  6, -1,  7,  7,     7,    7,  7, 7}; // variable index for each select
int32_t varMulti [] = {0, 10,  0, 1, 1,  0, 1, 60, -1, 1, 60, -1,  0, 1, 10,  0, 31536000, 2592000, 86400, 3600, 60, 1}; // variable multiplier for each select
int dSign; //sign of increment
/*
int varSelectFrame00 [] = 
variable[select][frame]
factor[select][frame]
*/
int setupFrames = 5; // total frames in SETUP
int setupFrame = 0; //current frame in SETUP
int call; // function to call
int millisNow = millis();
int lastFastUpdate;
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


uint32_t DtoJ2000(ts t) {
    uint16_t y;
    uint32_t tJ2000;
    int dayMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    Serial.print("DtoJ200 input ");
    Serial.print(t.year);
    Serial.print(" ");
    Serial.print(t.mon);
    Serial.print(" ");
    Serial.print(t.mday);
    Serial.print(" ");
    Serial.print(t.hour);
    Serial.print(" ");
    Serial.print(t.min);
    Serial.print(" ");
    Serial.print(t.sec);
    Serial.print(" = ");
    y = t.year - 2000; // cause this is the first year < at 1970 where year % 400 = 0
    tJ2000 = y * 31536000 - 43200 + (dayMonth[t.mon-1] + t.mday + (y/4) - ((y%4==0)&&(t.mday<3) ? 1 : 0) ) * 86400 + t.hour * 3600 + t.min * 60 + t.sec;
    Serial.println(tJ2000);
    if (tJ2000 < 0) {tJ2000 = 0;}
    return tJ2000;
}



/**
 * Convert from unix timestamp
 */
void setJ2000(uint32_t timestamp) {
  ts setT;
  ts getT;  
  //timestamp-= T2000UTC;
  timestamp += 43200; // convert J2000 in seconds from 1/1/2000 at 00:00
  uint8_t second = (int)(timestamp % 60);
  timestamp = timestamp / 60;
  uint8_t minute = (int)(timestamp % 60);
  timestamp = timestamp / 60;
  uint8_t hour = (int)(timestamp % 24);
  timestamp = timestamp / 24;
  uint8_t year = 0;
  int dYear = 0;
  if (year % 4 == 0)
    dYear = 366;
  else
    dYear = 365;
  while (timestamp >= dYear) {
    year++;
    timestamp-=dYear;
    if (year % 4 == 0)
      dYear = 366;
    else
      dYear = 365;
  }
  uint8_t day = (int) timestamp;
  int dayMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  uint8_t month = 0;
  while (day > dayMonth[month+1])
    month++;
  day-=dayMonth[month];
  month++;
  day++;
  setT.sec = second;
  setT.min = minute;
  setT.hour = hour;
  setT.mday = day;
  setT.mon = month;
  setT.year = year + 2000;
  DS3231_set(setT);
  Serial.print(DtoJ2000(setT));
/*    Serial.print(" ");
    Serial.print(setT.year);
    Serial.print(" ");
    Serial.print(setT.mon);
    Serial.print(" ");
    Serial.print(setT.mday);
    Serial.print(" ");
    Serial.print(setT.hour);
    Serial.print(" ");
    Serial.print(setT.min);
    Serial.print(" ");
    Serial.print(setT.sec);
    Serial.println(" ");
    DS3231_get(&getT);
    Serial.print("Get ");
    Serial.print(DtoJ2000(getT));
    Serial.print(" ");
    Serial.print(getT.year);
    Serial.print(" ");
    Serial.print(getT.mon);
    Serial.print(" ");
    Serial.print(getT.mday);
    Serial.print(" ");
    Serial.print(getT.hour);
    Serial.print(" ");
    Serial.print(getT.min);
    Serial.print(" ");
    Serial.print(getT.sec);
    Serial.println(" ");
    Serial.println(" "); */
}


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

void updateVariable (int varIndex, int dSign) {
/*  
 *   variable - pointer to the variable to update
 *   unitFraction - fraction of integer used as unit of change e.g. 1=>units 10=>10ths of unit 60=>60th of units
 *   dVal - step of change in unitFractions 
 *   txtValues - list of possible text values as Strings
 *   txtValuesCount - length of list
 */
  ts upT;
  if (varSelect[varIndex] == 7) {
    uint32_t minV = timeNow[1];
    uint32_t maxV = timeNow[2];
    int32_t unitFraction = varMulti[varIndex] * dSign;
    int32_t signedTime;
    DS3231_get(&upT);
    Serial.print("Update ");
    Serial.print(upT.year);
    Serial.print(" ");
    Serial.print(upT.mon);
    Serial.print(" ");
    Serial.print(upT.mday);
    Serial.print(" ");
    Serial.print(upT.hour);
    Serial.print(" ");
    Serial.print(upT.min);
    Serial.print(" ");
    Serial.print(upT.sec);   
    signedTime = DtoJ2000(upT);
    Serial.print(" ");
    Serial.print(signedTime);   
    Serial.print("+");
    Serial.print(unitFraction);   
    signedTime +=  unitFraction;
    Serial.print("=");
    Serial.println(signedTime);   
    if (signedTime > maxV) {signedTime = signedTime - maxV + minV;}
    else if (signedTime < minV) {signedTime = signedTime + maxV - minV;}    
    timeNow[0] = signedTime;
    setJ2000 (signedTime);    
  } else {
    float* variable = mainVariables[varSelect[varIndex]];
    int varSign = (*variable>=0) ? 1 :-1;
    *variable *= varSign;
    float minV =  0;
    float maxV = (varSign==1) ? mainVariables[varSelect[varIndex]][2] : -mainVariables[varSelect[varIndex]][1];
    if (varMulti[varIndex] == -1) { // change sign
      *variable *= -1.0;
    } else {
      int16_t unitFraction = varMulti[varIndex] * dSign;
      *variable += (1.0 / unitFraction);
      if (*variable > maxV) {*variable = *variable - maxV + minV;}
      else if (*variable < minV) {*variable = *variable + maxV - minV;}
    }
    *variable *= varSign;
  }
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
  
  drawData (display, 32, centerY, "Eye Height", String(mainVariables[0][0]));
//  drawData (display, 64, centerY, "", "OK");
  if (select!=0) {drawButton (display, 32+32*(select-1), centerY, iconUp, iconDown);}
}

void horizonSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawScrollBar(display, 1, 5, (select==0));
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 , 0, "Horizon Set" );
  // comment
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW , 0, "Line = Horizon" );
  //azimuth/elevation
  display->drawCircle(centerX-4,screenH-3,2);
  display->drawCircle(centerX+4,screenH-3,2);
  if (select != 0) {
    display->fillCircle(centerX+4*(2*select-3),screenH-3,2);    
  }
  //azimuth measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(centerX-8,screenH-8,"Height");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(centerX+8,screenH-8,"Tilt");
  //horizon line
  display->drawLine(0, centerY, screenW,  centerY);
  //azimuth line
  display->drawLine(centerX,centerY-5,centerX,centerY+5);
}

void latLonSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  float lat = mainVariables[3][0];
  float ulat = (lat >= 0.0) ? lat: -lat;
  char NS = (lat >= 0.0) ? 'N': 'S';
  float lon = mainVariables[4][0];
  float ulon = (lon >= 0.0) ? lon: -lon;
  char EW = (lon >= 0.0) ? 'E': 'W';
  int latDeg = int(ulat);
  int latMin = int(60.0*(ulat-latDeg));
  int lonDeg = int(ulon);
  int lonMin = int(60.0*(ulon-lonDeg));
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
  drawData (display, 16, centerY, "Lat.", String(latDeg)+"º");
  drawData (display, 32, centerY, "", twoDigits(latMin)+"'");
  drawData (display, 48, centerY, "", String(NS));
  drawData (display, 64, centerY, "Lon.", String(lonDeg)+"º");
  drawData (display, 80, centerY, "", twoDigits(lonMin)+"'");
  drawData (display, 96, centerY, "", String(EW));
//  drawData (display, 112, centerY, "", "OK");
  if (select!=0) {drawButton (display, 16+16*(select-1), centerY, iconUp, iconDown);}
}

void headingSpeedSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  int iHeading = int(mainVariables[5][0]);
  int iSpeed =  int(mainVariables[6][0]);
  int dSpeed = int(10*(mainVariables[6][0]-iSpeed));
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
  drawData (display, 32, centerY, "Heading", String(iHeading)+"º");
  drawData (display, 80, centerY, "Speed (kn)", String(iSpeed)+"."+String(dSpeed));
//  drawData (display, 96, centerY, "", "OK");
  if (select!=0) {drawButton (display, 32+48*(select-1), centerY, iconUp, iconDown);}
}

void timeGMTSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  unsigned char* iconUp = arrowUp;
  unsigned char* iconDown = arrowDown;
  DS3231_get(&t);
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
  drawData (display, 16, centerY, "Y", String(twoDigits(t.year_s)));
  drawData (display, 32, centerY, "M", String(twoDigits(t.mon)));
  drawData (display, 48, centerY, "D", String(twoDigits(t.mday)));
  drawData (display, 64, centerY, "h", String(twoDigits(t.hour)));
  drawData (display, 80, centerY, "m", String(twoDigits(t.min)));
  drawData (display, 96, centerY, "s", String(twoDigits(t.sec)));
//  drawData (display, 112, centerY, "", "OK");
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
//          if (buttStatus[i]!=ST_ACTIVE) { // because ST_ACTIVE has already returned ACT_SHORT
//            event = ((byte)i << 4) + (((byte)buttStatus[i]) & 0x02); // ACT_SHORT or ACT_END_LONG
            event = ((byte)i << 4) + 2*(((byte)buttStatus[i]) == ST_LONGPRESS); // ACT_SHORT or ACT_END_LONG
//          }
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
      event = ((byte)i << 4) + ACT_START_LONG; // with new ACTION states
    } else if ((millis()-pressedSince[i]>INHIBIT_JOINT) && (buttStatus[i]==ST_PRESSED)) {
      buttStatus[i] = ST_ACTIVE;
//      event = ((byte)i << 4) + ACT_SHORT;
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

void editValue (byte mode, byte command) {
  if (mode == 3) {
    if (select == 0) {
      
    }
  }
}


// This array keeps function pointers to all frames
FrameCallback frames[] = { starfinderFrame, takeSightFrame, positionFixFrame, eyeHeightSetFrame, horizonSetFrame, latLonSetFrame, headingSpeedSetFrame, timeGMTSetFrame };
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
  for (byte i=0; i< NUMBUTTONS-1; i++) {
    pinMode(buttons[i], INPUT);
    pinMode(buttons[i], INPUT_PULLUP);
  }
  pinMode(BUZZPIN, OUTPUT); // Set buzzer - pin 9 as an output

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
/*    if (millis()-millisNow > 1000) {
      DS3231_get(&t);
      timeNow[0] = dToJ2000(t);
      millisNow = millis();
    }
*/
    
/*    if (millis()-millisNow > 3000) {
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
*/
    event = checkEvent();
    if (event!=0xFF) {
      button = (event >> 4);
      if (button == BUTT_JOINT) {
        freq = BUZZLOW;
      }
      act = action[(byte)mode][(byte)event>>4][((byte)event & 0x03)];
      call = act & 0x1F;
      if (mode==M_SETUP) {
        switch (call) {
          case 0x01: { // CONFIRM short
            select = (select+1) % selectInFrame;
            varIndex = frameFirstIndex[setupFrame] + select; 
            break;
          }
          case 0x02: { // UP short
            if (select == 0) { //scoll to previous setupFrame
              setupFrame = (setupFrames + setupFrame - 1) % setupFrames;
              selectInFrame = selectPerFrame[setupFrame];
              ui.switchToFrame(mode+setupFrame);
            } else { //set unit increase for select variable
              dSign = 1;
              updateVariable(varIndex, dSign);
              dSign = 0; 
            }
            break;
          }
          case 0x03: { //UP long start
            if (select == 0) { //scoll to previous setupFrame
              setupFrame = (setupFrames + setupFrame - 1) % setupFrames;
              selectInFrame = selectPerFrame[setupFrame];
              ui.switchToFrame(mode+setupFrame);
            } else { //set unit increase for select variable
              dSign = 1;
              lastFastUpdate = millis();
            }
            break;
          }
          case 0x04: { //UP long end
            if (select == 0) { //do nothing
            } else { //set unit increase for select variable
              dSign = 0;
            }
            break;
          }
          case 0x05: { // DOWN short
            if (select == 0) { //scoll to next setupFrame
              setupFrame = (setupFrame + 1) % setupFrames;
              selectInFrame = selectPerFrame[setupFrame];
              ui.switchToFrame(mode+setupFrame);
            } else { // set unit decrease for select variable
              dSign = -1;
              updateVariable(varIndex, dSign);
              dSign = 0;
            }
            break;
          }
          case 0x06: { //DOWN long start
            if (select == 0) { //scoll to previous setupFrame
              setupFrame = (setupFrames + setupFrame - 1) % setupFrames;
              selectInFrame = selectPerFrame[setupFrame];
              ui.switchToFrame(mode+setupFrame);
            } else { //set unit increase for select variable
              dSign = -1;
              lastFastUpdate = millis();
            }
            break;
          }
          case 0x07: { //DOWN long end
            if (select == 0) { //do nothing
            } else { //set unit increase for select variable
              dSign = 0;
            }
            break;
          }
        }
      }
      // change mode after running functions
      if ( (act>>5) != 0x7 ) {
        mode = act>>5;
        if (mode!=3) { ui.switchToFrame(mode); } 
        else { 
          ui.switchToFrame(mode+setupFrame);
          selectInFrame = selectPerFrame[setupFrame];
        }
      }
    }
    // update variable
    //  if long pressed then change by unit every 100ms 
    if ((dSign !=0) && (millis() - lastFastUpdate > 100)) {
      lastFastUpdate = millis();
      updateVariable(varIndex, dSign);
    }
  }
}

/*
int frame = 0;
int select = 0;
int selectInFrame = 0;
int selectPerFrame [] = {0,0,0,2,0,7,3,7};
byte* selectVariablesFrame3 [] = {};  
int setupFrame = 0; //number of Frame within SETUP
*/
