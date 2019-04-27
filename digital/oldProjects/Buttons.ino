/**********************************************
 *
 * Control module of Sextant
 * 
 * 9 March 2019
 * 
 * @lionzan
 * 
 **********************************************/

//determine how big the array up above is, by checking the size
#define NUMBUTTONS 4 // 3 physical buttons + 1 virtual from the combination of 2
#define DEBOUNCE 5  // how many ms to debounce, 5+ ms is usually plenty
#define BUZZFREQ 1000
#define BUZZLEN 50

const byte buzzer = 9; //buzzer to arduino pin 9

 
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

  }
}

void loop() {
  byte event = checkEvent();
  if (event != 255) {
    Serial.print(event, HEX);
    Serial.print(" ");
    Serial.print(modeNames[mode-1]);
    Serial.print(" ");
    switch (mode) {
      case 1: {  // STARFINDER
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
      case 2: {  // TAKE SIGHT OR MEASURE
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
      case 3: {  // POSITION FIXING
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
      case 4: {  // SETUP - TAKE HORIZON
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
