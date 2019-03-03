// constants for planets positions
const TIME_J2000 = 946728000.0; // (seconds) J2000 == 1/1/2000 at 12:00 in the C++ reference, T0 == 1/1/1970 double check value!!!
const JD_J2000 = 2450680.5; // (days) J2000 == 1/1/2000 at 12:00 in the Julian Calendar
const JULIAN_CENTURY = 36525.0; // (days)
const JULIAN_DAY = 86400.0; // (seconds)
const JULIAN_CENTURY_SECONDS = JULIAN_DAY * JULIAN_CENTURY; 
const OBLIQUITY_J2000 = 23.43928 * DEG2RAD; // (rad) at J2000, virtually constant (oscillates 2.1 deg with 41,000 yrs cycle)
  
void getPlanetElelments (byte planet, *float elements[12]) {
/******************************
*
* TO BE COMPLETED!!!
*
* Retrieving Planet's elements from EEPROM
*
* elements (from NASA tables https://ssd.jpl.nasa.gov/txt/aprx_pos_planets.pdf)
* e0SeMaAxis = Semi Mayor Axis (au)
* e0Ecc     = Eccentricity (1)
*	e0Incl    = Inclination (deg)
* e0MeanLon = Mean Longitude (deg)
* e0LonPeri = Longitude of Perihelion (deg)
* edLonNode = Longitude of Ascending Node (deg)
*
* rates of change per Century
* edSeMaAxis
* edEcc
*	edIncl
* edMeanLon
* edLonPeri
* edLonNode
*
******************************/
  
// access EEPROM tables via I2C
  
}

void planetHeliocentricPosition (byte planet, float time, *float cHelioEquatCart[3]) {
/******************************
*
* Computing Planet's position in the Heliocentric Equatorial Cartesian System
*
* For slow planets it can be done only once at setup or every day or so
* For fast moving planets it should be done at the time of observation

* byte planet is the Planet's index (skipping 0 for clarity)
* 1 Mercury
* 2 Venus
* 3 Earth+Moon
* etc...
*
******************************/

// retrieve elements
float elements[12];
getPlanetElements (planet, elements);

// compute elements at time and convert angles from DEG to RAD
float dT = ( time - TIME_J2000 ) / JULIAN_CENTURY_SECONDS; //time from J2000 expressed in centuries for elements calculation
float eSeMaAxis = elem[0] + dT * elem[6];
float eEccentr  = elem[1] + dT * elem[7];
float	eInclin   = ( elem[2] + dT * elem[8] ) * DEG2RAD;
float eMeanLon  = ( elem[3] + dT * elem[9] ) * DEG2RAD;
float eLonPeri  = ( elem[4] + dT * elem[10] ) * DEG2RAD;
float eLonNode  = ( elem[5] + dT * elem[11] ) * DEG2RAD;

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
  eccAnom += dEA;
  i++;
}

// compute planet position on its heliocentric orbit
float xPlanet = eSeMaAxis * ( cos(pEccAnom) - eEccentr );
float yPlanet = eSeMaAxis * sin(pEccAnom) * sqrt( 1 - eEccentr * e Eccentr );
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

void planetGeocentricPosition ( byte planet, float time, * float cGeoEquatCart) {
/*****************************
*
* TO BE COMPLETED!!!
*
*******************************/
  
  float pHEC[3]; // Planet's Heliocentric Equatorial Cartesian coordiantes
  float eHEC[3]; // Earth's Heliocentric Equatorial Cartesian coordiantes
  
  // compute planet position
  planetHeliocentricPosition (elem[12], time, pHEC);
  
  // compute Earth position ( NOTE!!! approximate since not considering Earth vs. Earth + Moon 
  planetHeliocentricPosition (elem[12], time, eHEC);
  
}
