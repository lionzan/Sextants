/* MPU9250 Basic Example Code
 by: Kris Winer
 date: April 1, 2014
 license: Beerware - Use this code however you'd like. If you
 find it useful you can buy me a beer some time.
 Modified by Brent Wilkins July 19, 2016

 Demonstrate basic MPU-9250 functionality including parameterizing the register
 addresses, initializing the sensor, getting properly scaled accelerometer,
 gyroscope, and magnetometer data out. Added display functions to allow display
 to on breadboard monitor. Addition of 9 DoF sensor fusion using open source
 Madgwick and Mahony filter algorithms. Sketch runs on the 3.3 V 8 MHz Pro Mini
 and the Teensy 3.1.

 SDA and SCL should have external pull-up resistors (to 3.3V).
 10k resistors are on the EMSENSR-9250 breakout board.

 Hardware setup:
 MPU9250 Breakout --------- Arduino
 VDD ---------------------- 3.3V
 VDDI --------------------- 3.3V
 SDA ----------------------- A4
 SCL ----------------------- A5
 GND ---------------------- GND
 */

#include <Wire.h>
#include <U8glib.h> // U8glib - Version: 1.19.1

// SPI OLED board connection setup: SPI Com: SCK = 13, MOSI = 11, CS = NONE, A0 = 9, RST = 8
U8GLIB_SSD1309_128X64 u8g(13,11,255,9,8);

// Pin definitions
int intPin = 12;  // These can be changed, 2 and 3 are the Arduinos ext int pins
int myLed  = 13;  // Set up pin 13 led for toggling

volatile byte* yawFloatPtr;
volatile byte* pitchFloatPtr;
volatile byte* rollFloatPtr;

String txt;

#define I2Cclock 400000
#define I2Cport Wire
#define  MPU_SLAVE_ADDRESS  0x29  //slave address,any number from 0x01 to 0x7F

void drawYPR(float yaw, float pitch, float roll){
  char show[8];
  int xr = 32 * cos(roll);
  int yr = -32 * sin(roll);
  int zr = -160 * sin(pitch); // arbitrary number to be calibrated
  int xy = 32 * sin(yaw);
  int yy = -32 * cos(yaw);
  int xa = 32;
  int xb = 96;
  int y = 32;
// states to be implemented
// setup--------00
// startfinder--10 showing az/decl star name
// reading------20 showing crosshatch az/decl
// results------30 showing updated position
// 
  u8g.drawLine(xa,y,xa+xy,y+yy);
  u8g.drawLine(xb-xr,y-yr+zr,xb+xr,y+yr+zr);
  dtostrf(yaw*RAD_TO_DEG, 6, 2, show); // Leave room for too large numbers!
  u8g.drawStr( 20, 40, show);
}

//void drawChars(char chars[6]) {
//  u8g.drawStr( 10, 10, chars);
//}

void drawTxt(String str) {
  char show[6];
  str.toCharArray(show, 6);
  u8g.drawStr( 20, 20, show);
}

/* void drawLine(float angle){
  
} */

void setup()
{
  Wire.begin(); 
  Serial.begin(38400);
  Serial.println("Serial.begin");

/*  u8g.firstPage();  
  do {
    drawTxt("Initialising");
  } while( u8g.nextPage() );
*/
//// assign default color value and font
  u8g.setColorIndex(1);         // pixel on
  u8g.setFont(u8g_font_unifont);

  pinMode(8, OUTPUT);

//// end color mode 

  while(!Serial){};

  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(intPin, INPUT);
  digitalWrite(intPin, LOW);
  pinMode(myLed, OUTPUT);
  digitalWrite(myLed, HIGH);

}

void loop()
{
  Wire.requestFrom(4, 6);    // request 6 bytes from slave device #4
  while(Wire.available())    // slave may send less than requested
  { 
    char c = Wire.read(); // receive a byte as character
    txt=txt+c;
  }
  Serial.print(txt);

//// picture loop
  u8g.firstPage();  
  do {
    drawTxt( txt);
  } while( u8g.nextPage() );
  txt="";
  
  delay(200);
}

/*
void loop()
{
  float yaw;
  float pitch;
  float roll;
  char Data[6];
  int i=0;
  delay(500);
//  Serial.print("Wire.request time ");
//  Serial.println(millis());
  Wire.requestFrom(MPU_SLAVE_ADDRESS, 6);    // request 6 bytes from slave device #8
  while (Wire.available()) { // slave may send less than requested
    Data[i] = Wire.read(); // receive a byte as character
    Serial.print("iteration ");
    Serial.print(i);
    Serial.print(" - data ");
    Serial.println(Data[i]);
    i++;
  }
  Serial.println(Data);
/*  yawFloatPtr = (byte*) &yaw;
  pitchFloatPtr = (byte*) &pitch;
  rollFloatPtr = (byte*) &roll;
  yawFloatPtr[0]=Data[0];
  yawFloatPtr[1]=Data[1];
  yawFloatPtr[2]=Data[2];
  yawFloatPtr[3]=Data[3];
  pitchFloatPtr[0]=Data[4];
  pitchFloatPtr[1]=Data[5];
  pitchFloatPtr[2]=Data[6];
  pitchFloatPtr[3]=Data[7];
  rollFloatPtr[0]=Data[8];
  rollFloatPtr[1]=Data[9];
  rollFloatPtr[2]=Data[10];
  rollFloatPtr[3]=Data[11];
  Serial.println("data read DATA ");
  Serial.print(Data[0]);
  Serial.print(" ");
  Serial.print(Data[1]);
  Serial.print(" ");
  Serial.print(Data[2]);
  Serial.print(" ");
  Serial.println(Data[3]);
  Serial.print(Data[4]);
  Serial.print(" ");
  Serial.print(Data[5]);
  Serial.print(" ");
  Serial.print(Data[6]);
  Serial.print(" ");
  Serial.println(Data[7]);
  Serial.print(Data[8]);
  Serial.print(" ");
  Serial.print(Data[9]);
  Serial.print(" ");
  Serial.print(Data[10]);
  Serial.print(" ");
  Serial.println(Data[11]);
  Serial.print("data read YPR ");
  Serial.print(yaw);
  Serial.print(" ");
  Serial.print(pitch);
  Serial.print(" ");
  Serial.println(roll);

      // Declination of location
      // - http://www.ngdc.noaa.gov/geomag-web/#declination
//      myIMU.yaw  -= .5;
//      myIMU.roll *= RAD_TO_DEG;

//// picture loop
  u8g.firstPage();  
  do {
    drawYPR( yaw, pitch, roll);
  } while( u8g.nextPage() );
  
}
*/
