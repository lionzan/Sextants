#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#define BUZZPIN 9       // buzzer pin

/* Constructor */
//U8G2_UC1701_DOGS102_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_MIRROR, 5, 4);

/* u8g2.begin() is required and will sent the setup/init sequence to the display */
void setup(void) {
  u8g2.begin();
  tone(BUZZPIN, 880);
  delay(200);
  noTone(BUZZPIN);
}

/* draw something on the display with the `firstPage()`/`nextPage()` loop*/
void loop(void) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setFontDirection(1);
    u8g2.drawStr(0,20,"Hello World!");
  } while ( u8g2.nextPage() );
  delay(1000);
}
