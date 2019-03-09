//determine how big the array up above is, by checking the size
#define NUMBUTTONS 4 //S, L, H, L+H
#define DEBOUNCE 5  // how many ms to debounce, 5+ ms is usually plenty
 
//define the buttons that we'll use.
byte buttons[] = {4, 5, 6}; 
char bNames[] ="SLHJ";
char bEvents[] ="pslr";
 
//track if a button is just pressed, just released, or 'currently pressed' for S, L, H, L+H
byte pressed[NUMBUTTONS], oldpressed[NUMBUTTONS];
byte justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];
byte longpress[NUMBUTTONS], oldlongpress[NUMBUTTONS];
byte previous_keystate[NUMBUTTONS], current_keystate[NUMBUTTONS];

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
  }
}


void loop() {
  char event[2] = "";
  if (checkEvent(event)) {
    Serial.print(event[0]);
    Serial.println(event[1]);
  }
}
 
void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  static long statesince[NUMBUTTONS];
  byte index;
  if (millis() < lasttime) {
    // we wrapped around, lets just try again
    lasttime = millis();
  }
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  for (index = 0; index < NUMBUTTONS; index++) {
    justpressed[index] = 0;       //when we start, we clear out the "just" indicators
    justreleased[index] = 0;
    if (index == 3) {
      currentstate[index] = digitalRead(buttons[1]) || digitalRead(buttons[2]);
    } else {
      currentstate[index] = digitalRead(buttons[index]);   //read the button
    }
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) { //Watch out here LOW has opposite meanings
        // just pressed
        justpressed[index] = 1;
        statesince[index] = millis();
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        justreleased[index] = 1; // just released
        statesince[index] = millis();
      }
      pressed[index] = !currentstate[index];  //remember, digital HIGH means NOT pressed
      if (millis() - statesince[index] > 500) {
        longpress[index] = pressed[index];
      }else{
        longpress[index] = 0;
      }
    }
    previousstate[index] = currentstate[index]; //keep a running tally of the buttons
  }
/*  if ((pressed[1]==1) && (pressed[2]==1)) {
    if (longpressed[1]==0) && (longpressed[2]==0)) {
      if ((justpressed[1]==1) || (justpressed[2]==1)) { //if second button pressed before first turned long, they fuse in L+H
        justpressed[1]=0;
        justpressed[2]=0;
        justpressed[3]=1;
        statesince[3] = millis();
      }
    } else if ((justreleased[1]==1) || (justreleased[2]==1)) {
       justreleased[3]=1;
    }
  } */
}
 
byte thisSwitch_justReleased() {
  byte thisSwitch = 255;
  check_switches();  //check the switches &amp; get the current state
  for (byte i = 0; i < NUMBUTTONS; i++) {
    current_keystate[i]=justreleased[i];
    if (current_keystate[i] != previous_keystate[i]) {
      if (current_keystate[i] == 1) thisSwitch=i;
    }
    previous_keystate[i]=current_keystate[i];
  }  
  return thisSwitch;
}

/* byte thisSwitch_justlong() {
  byte thisSwitch = 255;
  check_switches();  //check the switches &amp; get the current state
  for (byte i = 0; i < NUMBUTTONS; i++) {
    current_keylen[i]=longpress[i];
    if (current_keylen[i] != previous_keylen[i]) {
      if (current_keylen[i]) thisSwitch=i;
    }
    previous_keylen[i]=current_keylen[i];
  }  
  return thisSwitch;
} */

boolean checkEvent(char event[2]) {
  boolean isEvent = false;
  check_switches();
  for (int i=3; i>=0; i--) { 
    if (longpress[i]!=oldlongpress[i]) {
      if (longpress[i]==1) {// start long
        event[0]=bEvents[2];
        event[1]=bNames[i];
        isEvent = true;
        break;
      }
      if (longpress[i]==0) {// released long
        event[0]=bEvents[3];
        event[1]=bNames[i];
        isEvent = true;
        break;
      }
    }
    if (pressed[i]!=oldpressed[i]) {
      if (pressed[i]==1) {// pressed
        event[0]=bEvents[0];
        event[1]=bNames[i];
        isEvent = true;
        break;
      }
      if (pressed[i]==0) {// released
        event[0]=bEvents[1];
        event[1]=bNames[i];
        isEvent = true;
        break;
      }
    }
  }
  for (int i=0; i<4; i++) {
    oldpressed[i]=pressed[i];
    oldlongpress[i]=longpress[i];
  }
  return isEvent;
}
