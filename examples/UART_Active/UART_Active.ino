#include <TOFSenseF.h>
TOFSenseF tof;
void setup(){ Serial.begin(115200); Serial2.begin(921600, SERIAL_8N1, 16, 17); tof.beginUART(Serial2,0); }
void loop(){ if(tof.read()){ Serial.print("mm="); Serial.print(tof.distanceMm()); Serial.print(" status="); Serial.print(tof.status()); Serial.print(" signal="); Serial.print(tof.signalStrength()); Serial.print(" prec(cm)="); Serial.println(tof.rangePrecisionCm()); }}
