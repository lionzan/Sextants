/**********************************************
 *
 * Buttons Slave module of Sextant
 * 
 * 17 March 2019
 * 
 * @lionzan
 * 
 * connect buttons between button pins and to ground
 * connect the buzzer between buzzer pin and ground
 * 
 **********************************************/
#include <Arduino.h> //needed ???
#include <Wire.h>
#include <ds3231.h>
#include <U8g2lib.h> // U8glib - Version: 1.19.1

ts t; //ts is a struct findable in ds3231.h

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_MIRROR, 5, 4);

// modes
#define M_STARFINDER 0x00
#define M_TAKE_SIGHT 0x01
#define M_POS_FIX    0x02
#define M_SETUP      0x03

#define NUMBUTTONS 4    // 3 physical buttons + 1 combination of 2
#define DEBOUNCE 5      // how many ms to debounce, 5+ ms is usually plenty
#define BUZZPIN 9       // buzzer pin
#define BUZZLOW 659     // 5th note of A 440Hz
#define BUZZHIGH 880    // octave of A 440Hz

// buttons
#define BUTT_CONFIRM 0x00 // 00
#define BUTT_DOWN    0x01 // 01
#define BUTT_UP      0x02 // 10
#define BUTT_JOINT   0x03 // 11

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

byte mode = 0x00;
// define modes names
char m00[]="Starfinder";
char m01[]="Take Sight";
char m02[]="Fix Position";
char m03[]="Setup";
char* modes [] = { m00, m01, m02, m03 };

//define the buttons
byte buttons[] = {4, 5, 6}; // button pins

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
      0xFF, // START_LONG - AVAILABLE
      0xFF  // END_LONG   - AVAILABLE
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
      0xFF, // SHORT      - swap Elevation/Azimuth 
      0xFF, // START_LONG - nil
      (M_STARFINDER << 5) // END_LONG   - ? exit without changes, switch to Starfinder mode ?
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

/*
 * setup groups:
 * 
 * function byte setup (byte *byt1, char *sep1, byte *byt2, char *sep2, byte *byt3, char *sep3, char chr4[], bool OKactive) {} // return prev or next
 * example next = setup(0x00, 0x00, &latDeg, "º", &latMin, "'", {"E", "W"}, false)
 * layout: 3 numerical (with up to 1 decimal), up to 1 alphanum, up to 1 OK/canc, 1 scroll prev/next
 * byt1 byt2 byt3 chr4[]
 *  ^    ^    ^    ^   ok   prev
 * 000  000  00.0  A  ----  ----
 *  v    v    v    v        next
 * w3   w3   w3   w2   w4    w4
 * Group
 * - Variables
 * 
 * Take Horizon Elevation (graphically against real)
 * - degrees 00º w3
 * - minutes 00' w3
 * - scroll      w4
 * Take Horizon Tilt
 * - degrees 00º w3
 * - minutes 00' w3
 * - scroll      w4
 * Set Eye Height (numerically)
 * - meters 00.0 w4 (in 1/10ths of metre)
 * - scroll      w4
 * Estimated Latitude (numerically from last Position Fix)
 * - Degrees 000ºw4
 * - minutes 00' w3
 * - N/S         w1
 * - scroll      w4
 * Estimated Longitude (numerically fron last Position Fix)
 * - Degrees 000ªw4
 * - Minutes 00' w3
 * - E/W         w1
 * - scroll      w4
 * Estimated Heading (numerically or graphically "point ahead")
 * - Degrees 000ªw4
 * Estimated Speed (numerically from last Position Fix change)
 * - Knots 00.0  w4
 * GMT time (numerically)
 * - Hours 00    w2
 * - Minutes 00  w2
 * - Seconds 00  w2
 * - NOW (to get it going now) w4
 * - scroll      w4
 * Setup Confirm
 * - Confirm all and Exit
 * - Cancel all and Exit
 * 
 */

void setup() {
  byte i;
  Serial.begin(9600); //set up serial port
  Serial.println("start setup");
  Wire.begin(); //start i2c (required for connection)
  DS3231_init(DS3231_INTCN); //register the ds3231 (DS3231_INTCN is the default address of ds3231, this is set by macro for no performance loss)
  // Make input & enable pull-up resistors on switch pins
  for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
    pinMode(BUZZPIN, OUTPUT); // Set buzzer - pin 9 as an output
  }
//// assign default color value and font
//  u8g2.setColorIndex(1);         // pixel on
//  u8g.setFont(u8g_font_04b_03);
//  u8g2.setFont(u8g2_font_profont10);
//  u8g2.setRot270();
//  u8g2.setFontPosBottom(); 
//  u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
//  u8g2_SetPowerSave(&u8g2, 0); // wake up display  u8g2.begin();
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.setFontDirection(1);
  Serial.println("finish setup");
}

void loop() {
  byte event;
  byte button;
  byte act;
  int freq = BUZZHIGH;

  Serial.println("start loop");
  // update screen
  u8g2.firstPage();  
  do {
    draw();
  } while( u8g2.nextPage() );

  event = checkEvent();
  Serial.println(event);
  if (false) { // was: if (event!=0xFF) {
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
  Serial.println("enter checkEvent");
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
  Serial.println("exit checkEvent");
  return event;
}

void buzz (int freq, int len) {
//  tone(BUZZPIN, freq);
//  delay(len);
//  noTone(BUZZPIN);
}

void draw () {
  char buff[0xFF];
// modes to be implemented

// write mode name
  u8g2.drawStr(0, 32 - u8g2.getStrWidth(modes[mode])/2, modes[mode]);

  // display current time
  DS3231_get(&t);
  snprintf(buff, 0xFF, "%02d:%02d:%02d", t.hour, t.min, t.sec);
  u8g2.drawStr(10, 0, buff);
// draw compass needle
//  u8g.drawLine(xa,ya,xa+xy,ya+yy);
// draw horizon
//  u8g.drawLine(xb-xr,yb-yr+zr,xb+xr,yb+yr+zr);
// write elevation and azimuth
//  u8g.setFontPosBottom(); 
//  dtostrf(dazi, 6, 2, show);
//  u8g.drawStr( 64 - u8g.getStrWidth(show), 127, show);
//  dtostrf(dele, 6, 2, show);
//  u8g.drawStr( 0, 127, show);
// write star name
//  u8g.setFontPosTop(); 
//  u8g.drawStr(xa - u8g.getStrWidth(starName)/2, 0, starName);
}

void setTime() {
/*  t.sec = inp2toi(cmd, 1);
  t.min = inp2toi(cmd, 3);
  t.hour = inp2toi(cmd, 5);
  t.wday = cmd[7] - 48;
  t.mday = inp2toi(cmd, 8);
  t.mon = inp2toi(cmd, 10);
  t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
  DS3231_set(t); */
}
