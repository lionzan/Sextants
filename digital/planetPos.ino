void planetPos (float elem[12], float time) {
/******************************
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

const TIME_J2000 = 946728000.0; // (seconds) double check value!!!
const JULIAN_CENTURY = 36525.0; // (days)
const OBLIQUITY_J2000 = 23.43928 * DEG2RAD; // (rad)

// compute elements at time and convert angles from DEG to RAD
float dT = ( time - TIME_J2000 ) / JULIAN_CENTURY;
float eSeMaAxis = elem[0] + dt * elem[6];
float eEccentr  = elem[1] + dt * elem[7];
float	eInclin   = ( elem[2] + dt * elem[8] ) * DEG2RAD;
float eMeanLon  = ( elem[3] + dt * elem[9] ) * DEG2RAD;
float eLonPeri  = ( elem[4] + dt * elem[10] ) * DEG2RAD;
float eLonNode  = ( elem[5] + dt * elem[11] ) * DEG2RAD;

// compute Perihelion
float peri = eLonPeri - eLonNode;

// compute Mean Anomaly (originally required to be between -180..+180 deg
float meanAnom = eMeanLon - eLonPeri;

// compute Eccentric Anomaly so that eccAnom = meanAnom + eEccentr * sin ( eccAnom )
float eccAnom = meanAnom + eEccentr * sin (meanAnom);
float dMA;
float dEA = 1.0;
float i = 0;
while ( (dEA > 1e-6) && ( i<10 ) ) {
  dMA = pMeanAnom - ( pEccAnom - eEccentr * sin (eccAnom) );
  dEA = dMA / ( 1 - e * cos (eccAnom) );
  eccAnom += dEA;
  i++;
}

// compute planet position on its heliocentric orbit
float xPlanet = eSeMaAxis * ( cos (eccAnom) - eEccentr );
float yPlanet = eSeMaAxis * sin (eccAnom) * sqrt ( 1 - eEccentr * e Eccentr );
float zPlanet = 0;

// transform position to J2000 ecliptic plane
float xEcliptic = ( cos(eLonPeri) * cos(eLonNode) - sin(eLonPeri) * sin(eLonNode) * cos(eInclin) ) * xPlanet;
xEcliptic -= ( sin(eLonPeri) * cos(eLonNode) + cos(eLonPeri) * sin(eLonNode) * cos(eInclin) ) * yPlanet;

float yEcliptic = ( cos(eLonPeri) * sin(eLonNode) + sin(eLonPeri) * cos(eLonNode) * cos(eInclin) ) * xPlanet;
yEcliptic -= ( sin(eLonPeri) * sin(eLonNode) - cos(eLonPeri) * cos(eLonNode) * cos(eInclin) ) * yPlanet;

float zEcliptic = ( sin(eLonPeri) * sin(eInclin) ) * xPlanet;
zEcliptic += ( cos(eLonPeri) * sin(eInclin) ) * yPlanet;

// transform position to equatorial J2000 frame (ICRF frame)
float xEquat = xEcliptic;
float yEquat = cos(OBLIQUITY_J2000) * yEcliptic - sin(OBLIQUITY_J2000) * zEcliptic;
float zEquat = sin(OBLIQUITY_J2000) * yEcliptic + cos(OBLIQUITY_J2000) * zEcliptic;
}
