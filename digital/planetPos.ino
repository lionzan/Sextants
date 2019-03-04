/***************************
*
* Verified on create.arduino on 2019-03-04
*
****************************/

#include <Wire.h>

// constants for planets positions
const float TIME_J2000 = 946728000.0; // (seconds) J2000 == 1/1/2000 at 12:00 in the C++ reference, T0 == 1/1/1970 double check value!!!
const float JD_J2000 = 2450680.5;     // (days) J2000 == 1/1/2000 at 12:00 in the Julian Calendar
const float JULIAN_CENTURY = 36525.0; // (days)
const float JULIAN_DAY = 86400.0;     // (seconds)
const float JULIAN_CENTURY_SECONDS = JULIAN_DAY * JULIAN_CENTURY; 
const float OBLIQUITY_J2000 = 23.43928 * DEG_TO_RAD; // (rad) at J2000, virtually constant (oscillates 2.1 deg with 41,000 yrs cycle)

// constants for EEPROM access
const unsigned int EEPROM_ADDR         = 0X0050; // hard set through pins 1-3 between 0x50-0x58
const unsigned int EEPROM_SIZE         = 0X8000; // 32 kbytes = 256 kbits
const unsigned int PLANETS_BASE_ADDR   = 0X0000; // 9 planets at 12 float each = 12*4 = 48 = 0X30 bytes each == 0x360 bytes total
const unsigned int PLANET_DATA_SIZE    = 0x60;
const unsigned int PLANET_E0_DATA_SIZE = 0x18;   // 6 float elements (+ 2 spare at the end)
const unsigned int PLANET_ED_DATA_SIZE = 0x18;   // 6 float elements (+ 2 spare at the end)
const unsigned int PLANET_INFO_SIZE    = 0x20;   // 32 bytes available for additional info on plane
const unsigned int SUN_MOON_BASE_ADDR  = 0X0360; // Free space for Sun and Moon or other uses = 0xA0 total
const unsigned int STARS_DATA_ADDR     = 0X0400; // up to 96 stars at 32 bytes each 
const unsigned int STAR_DATA_SIZE      = 0x20;
const unsigned int MAG_DECL_ADDR       = 0X1000; // grid 4 deg X 4 deg for Lon[180W,180E], Lat[72N,64S], 35*90=3,150 at 8 byte per point = 0x6270
const unsigned int MAG_DECL_BASE_SIZE  = 0x08;
const unsigned int FREE_SPACE_ADDR     = 0X7270; // 0x0D90 = 3472 bytes available

void setup() {
    
}

void loop() {
    
}

void i2c_eeprom_read_buffer( int deviceaddress, unsigned int eeaddress, byte *buffer, int length ) {
// read max 31 bytes
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress,length);
    int c = 0;
    for ( c = 0; c < length; c++ )
        if (Wire.available()) buffer[c] = Wire.read();
}

  
void getPlanetElements (byte planet, float elem[12]) {
/******************************
*
* Return Planet Elements from EEPROM
*
* TO BE TESTED!!!
*
* Elements table (from NASA tables https://ssd.jpl.nasa.gov/txt/aprx_pos_planets.pdf)
* e0... (elements at T0 = J2000)
* SeMaAxis = Semi Mayor Axis (au)
* Eccentr  = Eccentricity (1)
* Inclin   = Inclination (deg)
* MeanLon = Mean Longitude (deg)
* LonPeri = Longitude of Perihelion (deg)
* LonNode = Longitude of Ascending Node (deg)
*
* rates of change per Century
* ed... (elements d/dt)
* SeMaAxis
* Eccentr
* Inclin
* MeanLon
* LonPeri
* LonNode
*
******************************/
  
// access EEPROM tables via I2C and return values
int pAddr = PLANETS_BASE_ADDR + PLANET_DATA_SIZE * ( planet - 1 );
byte *e0 = (byte)elem;               // start from the beginning
byte *ed = e0 + PLANET_E0_DATA_SIZE; // start after end of e0 elements
i2c_eeprom_read_buffer( EEPROM_ADDR, pAddr, (byte *)e0, PLANET_E0_DATA_SIZE );
i2c_eeprom_read_buffer( EEPROM_ADDR, pAddr, (byte *)ed, PLANET_ED_DATA_SIZE );
  
}

void planetHeliocentricPosition (byte planet, float time, float cHelioEquatCart[3]) {
/******************************
*
* return Planet position in Heliocentric Equatorial Cartesian coordinates
*
* See notes in planetGeocentricPosition function
*
* Algorithm from from NASA https://ssd.jpl.nasa.gov/txt/aprx_pos_planets.pdf
*
******************************/

// retrieve elements
float elem[12];
getPlanetElements (planet, elem);

// compute elements at time and convert angles from DEG to RAD
float dT = ( time - TIME_J2000 ) / JULIAN_CENTURY_SECONDS; //time from J2000 expressed in centuries for elements calculation
float eSeMaAxis = elem[0] + dT * elem[6];
float eEccentr  = elem[1] + dT * elem[7];
float eInclin   = ( elem[2] + dT * elem[8] ) * DEG_TO_RAD;
float eMeanLon  = ( elem[3] + dT * elem[9] ) * DEG_TO_RAD;
float eLonPeri  = ( elem[4] + dT * elem[10] ) * DEG_TO_RAD;
float eLonNode  = ( elem[5] + dT * elem[11] ) * DEG_TO_RAD;

// compute Perihelion
float pPeri = eLonPeri - eLonNode;

// compute Mean Anomaly
float pMeanAnom = eMeanLon - eLonPeri;

// compute Eccentric Anomaly so that eccAnom = meanAnom + eEccentr * sin ( eccAnom )
float pEccAnom = pMeanAnom + eEccentr * sin(pMeanAnom);
float dMA;
float dEA = 1.0; //set at a value higher than tolerance tolEA (below)
float tolEA = 1e-6; // tolerance for Eccentric Anomaly calculation
int maxIter = 10;   // max number of iterations for EA calculation
float i = 0;
while ( (abs(dEA) >= tolEA) && ( i < maxIter ) ) {
  dMA = pMeanAnom - ( pEccAnom - eEccentr * sin(pEccAnom) );
  dEA = dMA / ( 1 - eEccentr * cos(pEccAnom) );
  pEccAnom += dEA;
  i++;
}

// compute planet position on its heliocentric orbit
float xPlanet = eSeMaAxis * ( cos(pEccAnom) - eEccentr );
float yPlanet = eSeMaAxis * sin(pEccAnom) * sqrt( 1 - eEccentr * eEccentr );
float zPlanet = 0;

// transform position to J2000 ecliptic plane
float xEcliptic = ( cos(pPeri) * cos(eLonNode) - sin(pPeri) * sin(eLonNode) * cos(eInclin) ) * xPlanet;
xEcliptic -= ( sin(pPeri) * cos(eLonNode) + cos(pPeri) * sin(eLonNode) * cos(eInclin) ) * yPlanet;

float yEcliptic = ( cos(pPeri) * sin(eLonNode) + sin(pPeri) * cos(eLonNode) * cos(eInclin) ) * xPlanet;
yEcliptic -= ( sin(pPeri) * sin(eLonNode) - cos(pPeri) * cos(eLonNode) * cos(eInclin) ) * yPlanet;

float zEcliptic = ( sin(pPeri) * sin(eInclin) ) * xPlanet;
zEcliptic += ( cos(pPeri) * sin(eInclin) ) * yPlanet;

// transform position to equatorial J2000 frame (ICRF frame) REMEMBER! The coordinates are stlii Heliocentric, not Geocentric!!
float xEquat = xEcliptic;
float yEquat = cos(OBLIQUITY_J2000) * yEcliptic - sin(OBLIQUITY_J2000) * zEcliptic;
float zEquat = sin(OBLIQUITY_J2000) * yEcliptic + cos(OBLIQUITY_J2000) * zEcliptic;
  
// return cHelioEquatCart
cHelioEquatCart [0] = xEquat;
cHelioEquatCart [1] = yEquat;
cHelioEquatCart [2] = zEquat;
  
}

void planetGeocentricPosition ( byte planet, float time, float cGeoEquatCart[3]) {
/*****************************
*
* return Planet position at time in Geocentric Equatorial Cartesian coordinates
*
* For slow planets it can be done just once at setup or every day or so
* For fast moving planets it should be done at the time of observation
* byte planet is the Planet's index (index 0 is used for the Sun position)
* 0 Sun
* 1 Mercury
* 2 Venus
* 3 Earth+Moon
* etc...
*
*******************************/
  
  float pHEC[3] = {0.0, 0.0, 0.0}; // Planet's Heliocentric Equatorial Cartesian coordiantes (default to the Sun)
  float eHEC[3];                   // Earth's Heliocentric Equatorial Cartesian coordiantes
  
  if (planet!=0) {
    // compute planet position, otherwise it is already set to be the Sun by default
    planetHeliocentricPosition (planet, time, pHEC);
  }
  
  // compute Earth position. Approximate since not considering Earth vs. Earth + Moon
  // but apparently good enough, with minimal parallax even with close planets [VERIFY!]
  planetHeliocentricPosition (3, time, eHEC);
  
  // compute and return Geocentric coordinates
  cGeoEquatCart[0] = pHEC[0] - eHEC[0];
  cGeoEquatCart[1] = pHEC[1] - eHEC[1];
  cGeoEquatCart[2] = pHEC[2] - eHEC[2];
  
}

void planetGeocentricSpherical ( byte planet, float time, float cGeoEquatSpherical[2], float *pDistance) {
/*****************************
*
* return planet position at time in Geocentric Equatorial Spherical coordinates (Lon, Lat, distance)
*
* TO BE COMPLETED!!!
*
*******************************/

  float pGES[2];                   // Planet's's Geocentric Equatorial Spherical coordiantes

  // compute planet position in Geocentric Equatorial Spherical coordinates
  planetGeocentricPosition (planet, time, pGES);
  
  // compute and return planet position [UPDATE THE FORMULAS WITH THE CORRECT TRANSFORMATION OF pGES!!!!]
  cGeoEquatSpherical[0] = 0.0;
  cGeoEquatSpherical[1] = 0.0;
  *pDistance = 0.0;

}

void starGeocentricSpherical (byte star, float cGeoEquatSpherical[2]) {
/*****************************
*
* Retrieving Star's position from EEPROM
*
* TO BE COMPLETED!!!
*
*******************************/

  // retrieve and return values
  cGeoEquatSpherical[0] = 0.0;
  cGeoEquatSpherical[1] = 0.0;
  
}

void cGEStoLocal ( float time, float cGeoEquatSpherical[2], float cLocal[2] ) {
/*****************************
*
* Convert Star or Planet position from Geocentric Equatorial Spherical to Local at time 
*
* TO BE COMPLETED!!!
*
*******************************/

    
}
