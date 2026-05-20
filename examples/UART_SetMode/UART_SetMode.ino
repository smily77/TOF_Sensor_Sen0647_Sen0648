#include <TOFSenseF.h>
TOFSenseF tof;
void setup(){
  Serial.begin(115200); Serial2.begin(921600, SERIAL_8N1, 16, 17); tof.beginUART(Serial2,0);
  Serial.println("WARNUNG: setToI2CViaUART kann UART/NAssistant Zugang unterbrechen!");
  tof.setToUARTActiveViaUART(0);
  // tof.setToUARTQueryViaUART(0);
  // tof.setToI2CViaUART(0); // mit Vorsicht!
}
void loop(){}
