/**********************************************
 *
 * Control module of Sextant
 * 
 * 9 March 2019
 * 
 * @lionzan
 * 
 **********************************************/

#include "quaternionFilters.h"
#include "MPU9250.h"
#include <U8glib.h> // U8glib - Version: 1.19.1

//U8GLIB_SSD1309_128X64 u8g(13,11,255,9,8); //SPI OLED board connection setup: SPI Com: SCK = 13, MOSI = 11, CS = NONE, A0 = 9, RST = 8
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);	// I2C / TWI 

#define NUMBUTTONS 4 // 3 physical buttons + 1 combination of 2
#define DEBOUNCE 5  // how many ms to debounce, 5+ ms is usually plenty
#define BUZZFREQ 1000
#define BUZZLEN 50

#define MODE_STARFINDER 0X01
#define MODE_MEASURE    0X02
#define MODE_POSITION   0X03
#define MODE_SETUP      0X04

#define EVENT_PRESS         0x01
#define EVENT_RELEASE_SHORT 0x02
#define EVENT_START_LONG    0x03
#define EVENT_RELEASE_LONG  0x04

#define BUTT_CONFIRM 0x10
#define BUTT_DOWN    0x20
#define BUTT_UP      0x30
#define BUTT_JOINT   0x40


const byte buzzer = 9; //buzzer to arduino pin 9

float dyaw = 45.5;
float dpitch = 13.5;
float droll = 5.0;
float dele = 127.12;
float dazi = 135.2;
char starName[] = "Zubenelgenubi";
char m00[]="";
char m01[]="Starfinder";
char m02[]="Take Sight";
char m03[]="Fix Position";
char m04[]="Setup";
char* modes [] = { m00, m01, m02, m03, m04 };

int avgDur = 0;
int cycles = 0;

//define the buttons
byte buttons[] = {4, 5, 6}; // physical buttons CONFIRM, UP, DOWN
char bNames[] ="CUDJ"; // buttons names CONFIRM, UP,  DOWN, UP+DOWN
char bEvents[] ="pslr"; // event names PRESS, RELEASE AFTER SHORT, BEGIN LONG, RELEASE AFTER LONG
int longStart = 500; // delay before considered long press
boolean noButtons = true; // no buttons pressed
 
//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte longPress[NUMBUTTONS], oldLongPress[NUMBUTTONS];

// current mode
char modeNames[] = "FMPS"; // FIND, MEASURE, POSITION, SETUP . SF, TS, PF, SU
byte mode = 1; //start from mode STARFINDER

void setup() {
  byte i;
  Serial.begin(9600); //set up serial port
  Serial.print("Button checker with ");
  Serial.print(NUMBUTTONS, DEC);
  Serial.println(" buttons");
  // Make input & enable pull-up resistors on switch pins
  for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
    pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output
//// assign default color value and font
  u8g.setColorIndex(1);         // pixel on
//  u8g.setFont(u8g_font_04b_03);
  u8g.setFont(u8g_font_profont10);
  u8g.setRot270();

  }
}

void loop() {
  u8g.firstPage();  
  do {
    drawYPR();
  } while( u8g.nextPage() );

  byte event = checkEvent();

  if (event != 255) {
    Serial.print(event, HEX);
    Serial.print(" ");
    Serial.print(modeNames[mode-1]);
    Serial.print(" ");
    switch (mode) {
      case MODE_STARFINDER: {  // STARFINDER
        switch (event) {
          case 0x12 : { //  Confirm short release 
            // switch to Reading mode
            mode = 2;
            buzz(1000,20);
            break;
          }
          case 0x14 : { //  Confirm long release 
            //calculate position from readings
            
            //switch to Results mode
            mode = 3;
            buzz(1000,20);
            break;
          }
          case 0x44 : { //  Joint long release 
            // switch to Setup mode
            mode = 4;
            buzz(500,200);
            break;
          }
          case 0x22 : { //  Down short release 
            // scroll across stars - previous star
            break;
          }
          case 0x24 : { //  Down long release 
            // scroll across star groups - previous group
            break;
          }
          case 0x32 : { //  Up short release 
            // scroll across stars - next star
            break;
          }
          case 0x34 : { //  Up long release 
            // scroll across star groups - next group
            break;
          }
        }
        break;
      }
      case MODE_MEASURE: {  // TAKE SIGHT OR MEASURE
        switch (event) {
          case 0x12 : { // 00 Confirm short release 
            // take reading
          
            // switch to Starfinder mode
            mode = 1;
            buzz(1000,20);
            break;
          }
          case 0x44 : { // 00 Joint long release 
            // exit without changes, switch to Starfinder mode
            mode = 1;
            buzz(1000,20);
            break;
          }
          case 0x22 : { //  Down short release 
            // reduce value stepwise (once per event)
            break;
          }
          case 0x23 : { //  Down long start 
            // reduce value fast (until release)
            break;
          }
          case 0x24 : { //  Down long release 
            // stop value change
            break;
          }
          case 0x32 : { //  Up short release 
            // increase value stepwise (once per event)
            break;
          }
          case 0x33 : { //  Up long start 
            // increase value fast (until release)
            break;
          }
          case 0x34 : { //  Up long release 
            // stop value change
            break;
          }
          case 0x42 : { //  Up+Down short release 
            // swap control of elevation/azimuth
            break;
          }
        }
        break;
      }
      case MODE_POSITION: {  // POSITION FIXING
        switch (event) {
          case 0x12 : { // Confirm short release 
            // keep readings and switch to Starfinder mode (for more readings)
            mode = 1;
            buzz(1000,20);
            break;
          }
          case 0x14 : { // Confirm long release 
            // store readings
            // reset readings
            // set estimated position to current
            // compute and set avg. heading and speed since last reading
            // switch to Starfinder
            mode = 1;
            buzz(1000,20);
            break;
          }
          case 0x44 : { //  Joint long release 
            // switch to Setup mode
            mode = 4;
            buzz(1000,20);
            break;
          }
          case 0x22 : { //  Down short release 
            // skip to previous result
            break;
          }
          case 0x23 : { //  Down long start 
            // continuous skip to previous (until release)
            break;
          }
          case 0x24 : { //  Down long release 
            // stop skipping
            break;
          }
          case 0x32 : { //  Up short release 
            // skip to next result
            break;
          }
          case 0x33 : { //  Up long start 
            // continuous skip to next (until release)
            break;
          }
          case 0x34 : { //  Up long release 
            // stop skipping
            break;
          }
        }
        break;
      }
      case MODE_SETUP: {  // SETUP - TAKE HORIZON
        switch (event) {
          case 0x44 : { // 00 Joint long release 
            mode = 1;
            buzz(1000,20);
            break;
          }
          case 0x12 : { // Confirm short release 
            // confirm values
            // switch to next digit
            break;
          }
          case 0x14 : { // Confirm long release 
            // confirm values
            // switch to next setup parameter
            break;
          }
          case 0x22 : { //  Down short release 
            // reduce parameter by one step
            break;
          }
          case 0x23 : { //  Down long start 
            // continuous reduce parameter (until release)
            break;
          }
          case 0x24 : { //  Down long release 
            // stop reducing
            break;
          }
          case 0x32 : { //  Up short release 
            // increase parameter by one step
            break;
          }
          case 0x33 : { //  Up long start 
            // continuous increase parameter (until release)
            break;
          }
          case 0x34 : { //  Up long release 
            // stop increasing
            break;
          }
        }
        break;
      }

    }
//    Serial.print(event[0]);
//    Serial.print(event[1]);
    Serial.println(modeNames[mode-1]);
  }
}
 
void checkSwitches() {
  static byte previousState[NUMBUTTONS];
  static byte currentState[NUMBUTTONS];
  static long lastTime;
  static long stateSince[NUMBUTTONS];
  byte index;
  if (millis() < lastTime) {
    // we wrapped around, lets just try again
    lastTime = millis();
  }
  if ((lastTime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lastTime = millis();
  for (index = 0; index < NUMBUTTONS; index++) {
    justPressed[index] = 0;       //when we start, we clear out the "just" indicators
    justReleased[index] = 0;
    if (index == 3) {
      currentState[index] = digitalRead(buttons[1]) || digitalRead(buttons[2]);
    } else {
      currentState[index] = digitalRead(buttons[index]);   //read the button
    }
    if (currentState[index] == previousState[index]) {
      if ((pressed[index] == LOW) && (currentState[index] == LOW)) { //Watch out here LOW has opposite meanings
        // just pressed
        justPressed[index] = 1;
        stateSince[index] = millis();
      }
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) {
        justReleased[index] = 1; // just released
        stateSince[index] = millis();
      }
      pressed[index] = !currentState[index];  //remember, digital HIGH means NOT pressed
      if (millis() - stateSince[index] > longStart) {
        longPress[index] = pressed[index];
      }else{
        longPress[index] = 0;
      }
    }
    previousState[index] = currentState[index]; //keep a running tally of the buttons
  }
}

 byte checkEvent() {
  byte event = 255; // if no event return 255
  checkSwitches();
  for (int i=4; i>0; i--) { 
    if (longPress[i-1]!=oldLongPress[i-1]) {
      if (longPress[i-1]==1) { // start long
        event = 3 + 16 * i;
        break;
      }
      if (longPress[i-1]==0) { // released long
        event = 4 + 16 * i;
        break;
      }
    }
    if (pressed[i-1]!=oldPressed[i-1]) {
      if (pressed[i-1]==1) {// pressed
        event = 1 + 16 * i;
        break;
      }
      if (pressed[i-1]==0) {// released
        event = 2 + 16 * i;
        break;
      }
    }
  }
  for (int i=0; i<4; i++) {
    oldPressed[i]=pressed[i];
    oldLongPress[i]=longPress[i];
  }
  noButtons = ((pressed[0]==0) && (pressed[1]==0) && (pressed[2]==0) && (pressed[3]==0));
  return event;
}

void buttonBeep () {
  tone(buzzer, BUZZFREQ); // Send 1KHz sound signal...
  delay(BUZZLEN);        // ...for 1 sec
  noTone(buzzer);     // Stop sound...
}

void buzz (int freq, int len) {
  tone(buzzer, freq); //
  delay(len);        //
  noTone(buzzer);     //
}

void drawYPR(){
  char show[8];
  if (mode == 0) {
    
  }
  int xr = 32 * cos(droll*DEG_TO_RAD);
  int yr = -32 * sin(droll*DEG_TO_RAD);
  int zr = 160 * sin(dpitch*DEG_TO_RAD); // arbitrary number
  int xy = 32 * sin(dyaw*DEG_TO_RAD);
  int yy = -32 * cos(dyaw*DEG_TO_RAD);
  int xa = 32;
  int ya = 64;
  int xb = 32;
  int yb = 64;
// modes to be implemented

// write mode name
  u8g.setFontPosBottom(); 
  u8g.drawStr( 32 - u8g.getStrWidth(modes[mode])/2, 120, modes[mode]);
// draw compass needle
  u8g.drawLine(xa,ya,xa+xy,ya+yy);
// draw horizon
  u8g.drawLine(xb-xr,yb-yr+zr,xb+xr,yb+yr+zr);
// write elevation and azimuth
  u8g.setFontPosBottom(); 
  dtostrf(dazi, 6, 2, show);
  u8g.drawStr( 64 - u8g.getStrWidth(show), 127, show);
  dtostrf(dele, 6, 2, show);
  u8g.drawStr( 0, 127, show);
// write star name
  u8g.setFontPosTop(); 
  u8g.drawStr(xa - u8g.getStrWidth(starName)/2, 0, starName);
}

void drawTxt(String str) {
  char show[100];
  str.toCharArray(show, 100);
  u8g.drawStr( 10, 10, show);
}

