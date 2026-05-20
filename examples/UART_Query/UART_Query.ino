#include <TOFSenseF.h>
TOFSenseF tof;
void setup(){ Serial.begin(115200); Serial2.begin(921600, SERIAL_8N1, 16, 17); tof.beginUART(Serial2,0); }
void loop(){ if(tof.requestAndReadUART(0,100)){ Serial.println(tof.distanceMm()); } delay(100); }
