#include "MPU9250.h"

MPU9250 mpu;

void setup()
{
    Serial.begin(115200);

    Wire.begin();

    delay(2000);
    mpu.setup();

    mpu.calibrateAccelGyro();
    mpu.calibrateMag();

    mpu.printCalibration();
}

void loop()
{
    static uint32_t prev_ms = millis();
    if ((millis() - prev_ms) > 100)
    {
        mpu.update();
        mpu.print("A");

/*        Serial.print("Pitch ");
        Serial.print(mpu.getPitch());
        Serial.print("  -  Roll ");
        Serial.print(mpu.getRoll());
        Serial.print("  -  Yaw ");
        Serial.println(mpu.getYaw());
*/
        prev_ms = millis();
    }
}
