// Arduino master for digital sextant
/****************************************************************************************
 * YPR I2C Slave Module (Enhanced MPU9250)
 * Slave address 0x29
 * Registers
 * ---------------------
 * Addr Description
 * ---------------------
 * 0x00 Status register
 * 0x01 yaw MSB
 * 0x02 yaw
 * 0x03 yaw
 * 0x04 yaw LSB
 * 0x05 pitch MSB
 * 0x06 pitch
 * 0x07 pitch
 * 0x08 pitch LSB
 * 0x09 roll MSB
 * 0x0A roll
 * 0x0B roll
 * 0x0C roll LSB
 * 0x09 temp MSB
 * 0x0A temp
 * 0x0B temp
 * 0x0C temp LSB
 * 0x0D Mode register
 * 0x0E Configuration register
 * 0x0F Identification register
 *
 ***************************************************************************************/


#include <Wire.h>

#define  YPR_SLAVE_ADDRESS  0x29  //YPR slave address
#define  YPR_DATA_SIZE        16  //YPR data size
#define  SERIAL_SPEED     115200  //

// check where to put the following
// TWBR = 12;  // 400 kHz (maximum)
// TWBR = 32;  // 200 kHz 
// TWBR = 72;  // 100 kHz (default)
// TWBR = 152;  // 50 kHz 

byte yprData[YPR_DATA_SIZE];
byte test = 0;
byte yprStatus = 0;
float yaw = 0;
float pitch = 0;
float roll = 0;
float temp = 0;
byte modeRegister = 0;
byte configRegister = 0;

void setup()
{
  Wire.begin();
  Serial.begin(SERIAL_SPEED);
}

void loop()
{
  getYPR();
  delay(1000);
  Wire.beginTransmission(YPR_SLAVE_ADDRESS);
  Wire.write(0x0B);
  Wire.write(test);
  Wire.write(test+1);
  Wire.endTransmission();
  test++;
}

void getYPR()
{
  Wire.beginTransmission(YPR_SLAVE_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(YPR_SLAVE_ADDRESS, YPR_DATA_SIZE);
  for (int i = 0; i < YPR_DATA_SIZE; i++)
  {
    yprData[i]=Wire.read();
//    Serial.print(yprData[i]);
//    Serial.print(" ");
  }
  
  byte * bytePointer;      //we declare a pointer as type byte
  byte arrayIndex = 1;     //we need to keep track of where we are retrieving data in the array
  yprStatus = yprData[0];  //no need to use a pointer for (byte)yprStatus
  
  bytePointer = (byte*)&yaw; //yaw is 4 bytes
  for (int i = 3; i > -1; i--)
  {
    bytePointer[i] = yprData[arrayIndex];  //increment pointer to store each byte
    arrayIndex++;
  }
//  Serial.print("Yaw=");
//  Serial.print(yaw);
  
  bytePointer = (byte*)&pitch; //pitch is 4 bytes
  for (int i = 3; i > -1; i--)
  {
    bytePointer[i] = yprData[arrayIndex];  //increment pointer to store each byte
    arrayIndex++;
  }
//  Serial.print(" Pitch=");
//  Serial.print(pitch);
  
  bytePointer = (byte*)&roll; //roll is 4 bytes
  for (int i = 3; i > -1; i--)
  {
    bytePointer[i] = yprData[arrayIndex];  //increment pointer to store each byte
    arrayIndex++;
  }
//  Serial.print(" Roll=");
//  Serial.print(roll);

  bytePointer = (byte*)&temp; //temp is 4 bytes
  for (int i = 3; i > -1; i--)
  {
    bytePointer[i] = yprData[arrayIndex];  //increment pointer to store each byte
    arrayIndex++;
  }
//  Serial.print(" Temp=");
//  Serial.print(temp);
  
  modeRegister = yprData[arrayIndex];
  arrayIndex++;
//  Serial.print("mod=");
//  Serial.print(modeRegister);
  
  configRegister =yprData[arrayIndex];
//  Serial.print("cfg=");
//  Serial.println(configRegister);

}
