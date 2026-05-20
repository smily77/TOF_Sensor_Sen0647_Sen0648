#include <Wire.h>
#include <TOFSenseF.h>
TOFSenseF tof;
void setup(){ Serial.begin(115200); Wire.begin(); tof.beginI2C(Wire,0x08); }
void loop(){ if(tof.readI2C()) { Serial.println(tof.distanceMm()); } delay(100); }
