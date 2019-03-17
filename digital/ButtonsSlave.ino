/**********************************************
 *
 * Buttons Slave module of Sextant
 * 
 * 17 March 2019
 * 
 * @lionzan
 * 
 **********************************************/

//U8GLIB_SSD1309_128X64 u8g(13,11,255,9,8); //SPI OLED board connection setup: SPI Com: SCK = 13, MOSI = 11, CS = NONE, A0 = 9, RST = 8
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);	// I2C / TWI 

#define NUMBUTTONS 4     // 3 physical buttons + 1 combination of 2
#define DEBOUNCE 5       // how many ms to debounce, 5+ ms is usually plenty
#define BUZZFREQ 1000    // buzzer frequency
#define BUZZLEN 50       // buzzer duration
#DEFINE BUZZPIN 9        // buzzer pin

// button events
#define EVENT_PRESS         0x01
#define EVENT_RELEASE_SHORT 0x02
#define EVENT_START_LONG    0x03
#define EVENT_RELEASE_LONG  0x04

// buttons
#define BUTT_CONFIRM 0x10
#define BUTT_DOWN    0x20
#define BUTT_UP      0x30
#define BUTT_JOINT   0x40

#define LONGSTART 500     //delay to become a long press

//define the buttons
byte buttons[] = {4, 5, 6}; // button pins
// char bNames[] ="CUDJ"; // buttons names CONFIRM, UP,  DOWN, JOINT UP+DOWN
// char bEvents[] ="pslr"; // event names PRESS, RELEASE AFTER SHORT, BEGIN LONG, RELEASE AFTER LONG
boolean noButtons = true; // no buttons pressed
 
//track if a button is just pressed, just released, or 'currently pressed'
byte pressed[NUMBUTTONS], oldPressed[NUMBUTTONS];
byte justPressed[NUMBUTTONS], justReleased[NUMBUTTONS];
byte longPress[NUMBUTTONS], oldLongPress[NUMBUTTONS];

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
    Serial.print(event, HEX);
    Serial.print(" ");
    Serial.print(modeNames[mode-1]);
    Serial.print(" ");
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
    // we wrapped around, move lastTime back by a full cycle
    lastTime -= 4294967295;
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
      if (millis() - stateSince[index] > LONGSTART) {
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
  tone(BUZZPIN, BUZZFREQ); // Send 1KHz sound signal...
  delay(BUZZLEN);        // ...for 1 sec
  noTone(BUZZPIN);     // Stop sound...
}

void buzz (int freq, int len) {
  tone(BUZZPIN, freq); //
  delay(len);        //
  noTone(BUZZPIN);     //
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

