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
#include <Wire.h>
#include <ds3231.h>

ts t; //ts is a struct findable in ds3231.h

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
#define ST_RELEASED   0x00
#define ST_PRESSED    0x01
#define ST_ACTIVE     0x02
#define ST_LONGPRESS  0x03
#define ST_INHIBITED  0xFF

// event byte structure
// button - isStart - status
// 0000     0         000

// button actions
#define EVENT_END_PRESS   0x01 //0001 relevant
#define EVENT_END_SHORT   0x02 //0010 relevant and same action as END_PRESS
#define EVENT_END_LONG    0x03 //0011 relevant
#define EVENT_START_SHORT 0x06 //0110 
#define EVENT_START_LONG  0x07 //0111 relevant

// alternative button actions: bit 0 (SHORT=0; LONG=1) , bit 1 (0=END; 1=START) 
#define ACT_END_SHORT   Ox00 // 00
#define ACT_END_LONG    Ox01 // 01
#define ACT_START_SHORT Ox02 // 10
#define ACT_START_LONG  Ox03 // 11


#define INHIBIT_JOINT 200     //delay to become a genuine short press, not possible to do joint anymore
#define LONG_START    500     //delay to become a long press
#define MAXINT 4294967295 // the max value of an int after which millis rolls over

byte mode = 0x00;

//define the buttons
byte buttons[] = {4, 5, 6}; // button pins

//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte buttStatus[NUMBUTTONS] = {ST_RELEASED,ST_RELEASED,ST_RELEASED,ST_RELEASED};
unsigned long pressedSince[NUMBUTTONS] = {0,0,0,0};

//define action table. Each action has 2 components: switch to mode (3 bytes) and perform procedure (5 bytes)

byte action[4][4][8] = {
  { // current mode M_STARFINDER
    { // button CONFIRM
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed UP
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed DOWN
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed JOINT
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, (M_SETUP << 5), 0xFF, 0xFF, 0xFF, 0xFF
    }
  },
  { // current mode M_TAKE_SIGHT
    { // button CONFIRM
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed UP
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed DOWN
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed JOINT
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, (M_SETUP << 5), 0xFF, 0xFF, 0xFF, 0xFF
    }
  },
  { // current mode M_POS_FIX
    { // button CONFIRM
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed UP
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed DOWN
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed JOINT
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, (M_SETUP << 5), 0xFF, 0xFF, 0xFF, 0xFF
    }
  },
  { // current mode M_SETUP
    { // button CONFIRM
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed UP
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed DOWN
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    { // button pressed JOINT
      // VOID, END_PRESS, END_SHORT, END_LONG, VOID, VOID, START_SHORT, START_LONG
        0xFF, 0xFF, 0xFF, (M_STARFINDER << 5), 0xFF, 0xFF, 0xFF, 0xFF
    }
  }
};

void setup() {
  byte i;
  Wire.begin(); //start i2c (required for connection)
  DS3231_init(DS3231_INTCN); //register the ds3231 (DS3231_INTCN is the default address of ds3231, this is set by macro for no performance loss)
  Serial.begin(9600); //set up serial port
  // Make input & enable pull-up resistors on switch pins
  for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
    pinMode(BUZZPIN, OUTPUT); // Set buzzer - pin 9 as an output
  }
}

void loop() {
  byte event;
  byte button;
  byte act;
  int freq = BUZZHIGH;
  event = checkEvent();
  if (event!=0xFF) {
    Serial.print("Event : ");
    Serial.println(event,BIN);    
    DS3231_get(&t); //get the value and pass to the function the pointer to t, that make an lower memory fingerprint and faster execution than use return
  #ifdef CONFIG_UNIXTIME
    Serial.print("Timestamp : ");
    Serial.println(t.unixtime);
  #endif
    button = (event >> 4);
    if (button == BUTT_JOINT) {
      freq = BUZZLOW;
    }
    Serial.print("Button : ");
    Serial.println(button);    
    act = action[(byte)mode][(byte)event>>4][((byte)event & 0x07)];  
    Serial.print ("Mode:");
    Serial.print(mode);
    Serial.print (" Button:");
    Serial.print (event>>4,HEX);
    Serial.print (" Event:");
    Serial.print ((event&0x07),HEX);
    Serial.print (" Action:");
    Serial.print (act,BIN);
    Serial.print (" New mode:");
    Serial.println (act>>5,HEX);
    if (act!=0xFF) {    mode = act>>5; }

/*    switch (mode | button | action) {
      case (M_STARFINDER | BUTT_JOINT | EVENT_END_LONG) : {
        mode = M_SETUP;
        break;
      }
      case (M_TAKE_SIGHT | BUTT_JOINT | EVENT_END_LONG) : {
        mode = M_SETUP;
        break;
      }
      case (M_POS_FIX | BUTT_JOINT | EVENT_END_LONG) : {
        mode = M_SETUP;
        break;
      }
    }
    switch (action) {
      case EVENT_END_PRESS : {
        buzz (freq,100);
        break;
      }
      case EVENT_START_SHORT : {
        buzz (freq,100);
        break;
      }
      case EVENT_START_LONG : {
        buzz (freq,400);
        break;
      }
    }*/
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
        if  (buttStatus[i]!=ST_INHIBITED) {
          event = ((byte)i << 4) + ((byte)buttStatus[i]);
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
      event = ((byte)i << 4) + ((byte)buttStatus[i]) + 4;
//      event = 0x10 * i + EVENT_START_LONG;
    } else if ((millis()-pressedSince[i]>INHIBIT_JOINT) && (buttStatus[i]==ST_PRESSED)) {
      buttStatus[i]=ST_ACTIVE;
      event = ((byte)i << 4) + ((byte)buttStatus[i]) + 4;
//      event = 0x10 * i + EVENT_PRESS;
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
  return event;
}

void buzz (int freq, int len) {
  tone(BUZZPIN, freq);
  delay(len);
  noTone(BUZZPIN);
}
