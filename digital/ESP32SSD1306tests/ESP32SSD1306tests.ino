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

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

// Initialize the OLED display using Wire library
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
OLEDDisplayUi ui ( &display );

int screenW = 128;
int screenH = 64;
int centerX = screenW/2;
int centerY = screenH/2;  

int select = 0;
int timeNow = millis();
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

void drawData (OLEDDisplay *display, int16_t x, int16_t y, String title, String value, String unit) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->drawString(x, y-16, title);
  display->drawString(x, y, value+unit);
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
  display->drawString(0,screenH-8,"A 155º32'22\"");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW,screenH-8,"E 65º23'45\"");
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
  display->drawString(0,screenH-8,"A 155º32'22\"");
  //elevation measure
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(screenW,screenH-8,"E 65º23'45\"");
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

void zeroSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
//  ui.enableIndicator();
  unsigned char* iconUp[] = { space, arrowUp, arrowUp, rotRight, checkY};
  unsigned char* iconDown[] = { space, arrowDown, arrowDown, rotLeft, checkX};
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(16 , 0, "Zero Set");
  //buttons
  
  drawData (display, centerX-18, centerY, "EyeH", "9", "m");
  drawData (display, centerX, centerY, "Elev", "295", "º");
  drawData (display, centerX+18, centerY, "Tilt", "-2", "º");
  drawData (display, centerX+36, centerY, "", "OK", "");
  if (select==0) {
    drawScrollBar(display, 3, 4, true);
  } else {
    drawButton (display, centerX-36+18*select, centerY, iconUp[select], iconDown[select]);    
    drawScrollBar(display, select-1, 4, false);
  }
}

void latitudeSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  ui.enableIndicator();
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Latitude Position Set" );
  display->setTextAlignment(TEXT_ALIGN_CENTER);

}

void longitudeSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  ui.enableIndicator();
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Longitude Position Set" );
  display->setTextAlignment(TEXT_ALIGN_CENTER);

}

void headingSpeedSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  ui.enableIndicator();
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Heading / Speed Set" );
  display->setTextAlignment(TEXT_ALIGN_CENTER);

}

void timeGMTSetFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  ui.enableIndicator();
  display->setFont(Dialog_plain_8);
  //mode
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 , 0, "Time GMT Set" );
  display->setTextAlignment(TEXT_ALIGN_CENTER);

}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { starfinderFrame, takeSightFrame, positionFixFrame, zeroSetFrame, latitudeSetFrame, longitudeSetFrame, headingSpeedSetFrame, timeGMTSetFrame };

// how many frames are there?
int frameCount = 8;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(9600);
  Serial.println();

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

  ui.switchToFrame(3);

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
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    if (millis()-timeNow > 1000) {
      timeNow=millis();
      select = (select+1)%5;
      if (select == 0) {selBar=!selBar;}
    }
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
//    delay(remainingTimeBudget);

  }


}
