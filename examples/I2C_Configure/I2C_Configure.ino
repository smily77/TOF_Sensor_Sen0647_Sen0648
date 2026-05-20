#include <Wire.h>
#include <TOFSenseF.h>
TOFSenseF tof;
void setup(){
  Serial.begin(115200); Wire.begin(); tof.beginI2C(Wire,0x08);
  uint16_t pv,hv,bv; uint32_t fw,cfg;
  tof.readProductVersion(pv); tof.readHardwareVersion(hv); tof.readBootloaderVersion(bv); tof.readFirmwareVersion(fw); tof.readDeviceConfig(cfg);
  Serial.printf("PV:%u HV:%u BL:%u FW:%lu CFG:0x%08lX\n",pv,hv,bv,fw,cfg);
  tof.setRefreshRateAndFilterI2C(50,5);
}
void loop(){}
