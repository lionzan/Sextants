/**********************************************
 *
 * Buttons Slave module of Sextant
 * 
 * 17 March 2019
 * 
 * @lionzan
 * 
 **********************************************/

#define NUMBUTTONS 4     // 3 physical buttons + 1 combination of 2
#define DEBOUNCE 5       // how many ms to debounce, 5+ ms is usually plenty
#define BUZZFREQ 1000    // buzzer frequency
#define BUZZLEN 50       // buzzer duration
#define BUZZPIN 9        // buzzer pin

// buttons
#define BUTT_CONFIRM 0x00
#define BUTT_DOWN    0x01
#define BUTT_UP      0x02
#define BUTT_JOINT   0x03

// buttons status
#define ST_RELEASED   0x00
#define ST_PRESSED    0x01
#define ST_ACTIVE     0x02
#define ST_LONGPRESS  0x03
#define ST_INHIBITED  0xFF

// button events
#define EVENT_PRESS         0x00
#define EVENT_RELEASE_SHORT 0x01
#define EVENT_START_LONG    0x02
#define EVENT_RELEASE_LONG  0x03


#define INHIBIT_JOINT 200     //delay to become a genuine short press, not possible to do joint anymore
#define LONG_START    500     //delay to become a long press
#define MAXINT 4294967295 // the max value of an int after which millis rolls over

//define the buttons
byte buttons[] = {4, 5, 6}; // button pins

//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte buttStatus[NUMBUTTONS] = {ST_RELEASED,ST_RELEASED,ST_RELEASED,ST_RELEASED};
unsigned long pressedSince[NUMBUTTONS] = {0,0,0,0};


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
    pinMode(BUZZPIN, OUTPUT); // Set buzzer - pin 9 as an output
  }
}

void loop() {
  byte event;
  byte button;
  byte action;
  event = checkEvent();
  if (event!=0xFF) {
    Serial.println(event,HEX);
    button = (event & 0xF0) >> 4;
    action = event & 0x0F;
    Serial.print(button);
    Serial.println(action);
    switch (action) {
      case EVENT_RELEASE_SHORT : {
        buzz (1000,100);
        break;
      }
      case EVENT_START_LONG : {
        buzz (1000,500);
        break;
      }
    }
  }
}
 
bool checkSwitches() {
  bool event = false;
  static byte previousState[NUMBUTTONS];
  static byte currentState[NUMBUTTONS];
  static unsigned long lastTime;
  static long stateSince[NUMBUTTONS];
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
        stateSince[index] = millis();
        event=true;
      }
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) {
        justReleased[index] = 1; // just released
        stateSince[index] = millis();
        event=true;
      }
      pressed[index] = !currentState[index];  //remember, digital HIGH means NOT pressed
    } 
    previousState[index] = currentState[index]; //keep a running tally of the buttons
  }
  return event;
}

byte checkEvent() {  
  byte event = 0xFF;
  if (checkSwitches()==true) {
    for (int i=BUTT_JOINT; i>=BUTT_CONFIRM; i--) {
      if ((justPressed[i]==1) && (buttStatus[i]!=ST_INHIBITED)) {
        buttStatus[i]=ST_PRESSED;
        pressedSince[i] = millis();
        if (i== BUTT_JOINT) {
          buttStatus[BUTT_UP] = ST_INHIBITED;
          buttStatus[BUTT_DOWN] = ST_INHIBITED;
          buttStatus[BUTT_JOINT] = ST_ACTIVE;
          Serial.println("PRESS 3 ");
          // return EVENT PRESS JOINT
          event = 0x10 * i + EVENT_PRESS;
        }
      } else if (justReleased[i]==1) {
        switch (buttStatus[i]) {
          case ST_INHIBITED :{
            break;
          }
          case ST_LONGPRESS :{
            Serial.print("RELEASE LONG ");
            Serial.println(i);
            // return EVENT RELEASE LONG
            event = 0x10 * i + EVENT_RELEASE_LONG;
            break;
          }
          case ST_ACTIVE :{
            Serial.print("RELEASE SHORT ");
            Serial.println(i);
            // return EVENT RELEASE SHORT
            event = 0x10 * i + EVENT_RELEASE_SHORT;
            break;
          }
          case ST_PRESSED :{
            Serial.print("RELEASE SHORT ");
            Serial.println(i);
            // return EVENT RELEASE SHORT
            event = 0x10 * i + EVENT_RELEASE_SHORT;
            break;
          }
        }
        buttStatus[i]=ST_RELEASED;
      }
    }
  }  
  // check status function of time
  for (int i=BUTT_JOINT; i>=BUTT_CONFIRM; i--) {
    if ((millis()-pressedSince[i]>LONG_START) && buttStatus[i]==ST_ACTIVE) {
      buttStatus[i]=ST_LONGPRESS;
            Serial.print("START LONG ");
            Serial.println(i);
      // return EVENT START LONG
      event = 0x10 * i + EVENT_START_LONG;
    } else if ((millis()-pressedSince[i]>INHIBIT_JOINT) && (buttStatus[i]==ST_PRESSED)) {
      buttStatus[i]=ST_ACTIVE;
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
