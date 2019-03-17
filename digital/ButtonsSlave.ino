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

// button events
#define EVENT_PRESS         0x01
#define EVENT_RELEASE_SHORT 0x02
#define EVENT_START_LONG    0x03
#define EVENT_RELEASE_LONG  0x04

// buttons
#define BUTT_CONFIRM 0x01
#define BUTT_DOWN    0x02
#define BUTT_UP      0x03
#define BUTT_JOINT   0x04

#define ACTIVESTART 200     //delay to become a genuine short press, not possible to do joint anymore
#define LONGSTART  500     //delay to become a long press
#define MAXINT 4294967295 // the max value of an int after which millis rolls over

//define the buttons
byte buttons[] = {4, 5, 6}; // button pins
boolean noButtons = true; // no buttons pressed
boolean afterJointRelease = false; //the last event was a Joint Release, so next release of UP or DOWN are not reported
boolean activeUDJ = false; //one button of UDJ is active, to avoid activating the others
 
//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte longPress[NUMBUTTONS], oldLongPress[NUMBUTTONS];
byte activePress[NUMBUTTONS], oldActivePress[NUMBUTTONS];

// current mode
// char modeNames[] = "FMPS"; // FIND, MEASURE, POSITION, SETUP . SF, TS, PF, SU
// byte mode = 1; //start from mode STARFINDER

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
  byte event = checkEvent();
  if (event != 255) {
    Serial.println(event, HEX);
  }
}
 
void checkSwitches() {
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
    return; 
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
      }
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) {
        justReleased[index] = 1; // just released
        stateSince[index] = millis();
      }
      pressed[index] = !currentState[index];  //remember, digital HIGH means NOT pressed
      if (millis() - stateSince[index] > LONGSTART) {
        longPress[index] = pressed[index];
      }else{
        longPress[index] = 0;
      }
      if (millis() - stateSince[index] > ACTIVESTART) {
        activePress[index] = pressed[index];
      }else{
        activePress[index] = 0;
      }
    }
    previousState[index] = currentState[index]; //keep a running tally of the buttons
  }
}

byte checkEvent() {
  byte event = 0xFF; // if no event return 255
  checkSwitches();
  for (byte i=4; i>0; i--) { 
    if (longPress[i-1]!=oldLongPress[i-1]) {
      if (longPress[i-1]==1) { // start long
        event = EVENT_START_LONG + 0x10 * i;
        break;
      }
      if (longPress[i-1]==0) { // released long
        if ((afterJointRelease == true) && ((i == BUTT_UP) || (i == BUTT_DOWN))) { // it's the release fo the second joint long button
          afterJointRelease = false; // reset flag and do not return anything
          Serial.println("afterJointRelease = FALSE");
          break;
        } else {
          event = EVENT_RELEASE_LONG + 0x10 * i;
          if (i > 0) {
            activeUDJ = false;
          }
          if (i == BUTT_JOINT) {
            afterJointRelease = true;
            Serial.println("afterJointRelease = TRUE");
          }
          break;
        }
      }
    }
    if (activePress[i-1]!=oldActivePress[i-1]) {
      if (activePress[i-1] == 1) {// pressed
        if ((i > 0) && (activeUDJ == true)) {
          break;
        }
        if (i > 0) {
          activeUDJ = true;
        }
        event = EVENT_PRESS + 0x10 * i;
        break;
      }
      if (activePress[i-1] == 0) {// released
        if (i > 0) {
          activeUDJ = false;
        }
        event = EVENT_RELEASE_SHORT + 0x10 * i;
        break;
      }
    }
  }
  for (int i=0; i<4; i++) {
    oldLongPress[i]=longPress[i];
    oldActivePress[i]=activePress[i];
  }
  noButtons = ((pressed[0]==0) && (pressed[1]==0) && (pressed[2]==0) && (pressed[3]==0));
  return event;
}

void buzz (int freq, int len) {
  tone(BUZZPIN, freq);
  delay(len);
  noTone(BUZZPIN);
}
