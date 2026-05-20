#include "TOFSenseF.h"

TOFSenseF::TOFSenseF() : _serial(nullptr), _wire(nullptr), _id(0), _i2cAddress(0x08), _lastError(Error::OK), _hasNewMeasurement(false), _frameIndex(0) {}

bool TOFSenseF::beginUART(Stream &serial, uint8_t id) { _serial = &serial; _id = id; _frameIndex = 0; _hasNewMeasurement = false; setError(Error::OK); return true; }
#if defined(ARDUINO_ARCH_ESP32)
bool TOFSenseF::beginUART(HardwareSerial &serial, uint32_t baud, int8_t rxPin, int8_t txPin, uint8_t id) { serial.begin(baud, SERIAL_8N1, rxPin, txPin); return beginUART(static_cast<Stream&>(serial), id); }
#endif
void TOFSenseF::setUARTStream(Stream &serial) { _serial = &serial; }

bool TOFSenseF::read() {
  if (!_serial) { setError(Error::NoStream); return false; }
  while (_serial->available() > 0) {
    uint8_t b = static_cast<uint8_t>(_serial->read());
    if (_frameIndex == 0 && b != kUartHeader) { setError(Error::BadHeader); continue; }
    _frameBuffer[_frameIndex++] = b;
    if (_frameIndex == kUartFrameLen) {
      _frameIndex = 0;
      if (parseFrame(_frameBuffer)) return true;
      if (b == kUartHeader) { _frameBuffer[0] = b; _frameIndex = 1; }
    }
  }
  return false;
}

bool TOFSenseF::parseFrame(const uint8_t *frame) {
  if (frame[0] != kUartHeader) { setError(Error::BadHeader); return false; }
  if (frame[1] != kUartOutputFunction) { setError(Error::BadFunction); return false; }
  if (!verifyChecksum(frame, kUartFrameLen)) { setError(Error::BadChecksum); return false; }
  _measurement.id = frame[3];
  _measurement.systemTimeMs = decodeUInt32LE(frame + 4);
  uint32_t rawDist = decodeUInt24LE(frame + 8);
  _measurement.distanceMm = static_cast<int32_t>(rawDist);
  _measurement.distanceM = rawDist / 1000.0f;
  _measurement.distanceStatus = frame[11];
  _measurement.signalStrength = decodeUInt16LE(frame + 12);
  _measurement.rangePrecisionCm = frame[14];
  _measurement.validChecksum = true;
  _measurement.validFrame = true;
  _measurement.timestampMs = millis();
  _hasNewMeasurement = true;
  setError(Error::OK);
  return true;
}

bool TOFSenseF::available() { return _hasNewMeasurement; }
const TOFSenseF::Measurement& TOFSenseF::measurement() const { return _measurement; }
int32_t TOFSenseF::distanceMm() const { return _measurement.distanceMm; }
float TOFSenseF::distanceM() const { return _measurement.distanceM; }
uint8_t TOFSenseF::id() const { return _measurement.id; }
uint16_t TOFSenseF::status() const { return _measurement.distanceStatus; }
uint16_t TOFSenseF::signalStrength() const { return _measurement.signalStrength; }
uint8_t TOFSenseF::rangePrecisionCm() const { return _measurement.rangePrecisionCm; }

bool TOFSenseF::requestUART(uint8_t id) {
  if (!_serial) { setError(Error::NoStream); return false; }
  uint8_t frame[8] = {kUartHeader, kUartQueryFunction, 0xFF, 0xFF, id, 0xFF, 0xFF, 0};
  frame[7] = checksum(frame, 7);
  size_t written = _serial->write(frame, sizeof(frame));
  return written == sizeof(frame);
}
bool TOFSenseF::requestAndReadUART(uint8_t id, uint32_t timeoutMs) {
  if (!requestUART(id)) return false;
  uint32_t start = millis();
  while (millis() - start < timeoutMs) if (read()) return true;
  setError(Error::Timeout);
  return false;
}

bool TOFSenseF::beginI2C(TwoWire &wire, uint8_t address) { _wire = &wire; _i2cAddress = address; setError(Error::OK); return true; }
bool TOFSenseF::readI2C() { return readI2C(_i2cAddress); }
bool TOFSenseF::readI2C(uint8_t address) {
  _i2cAddress = address;
  uint8_t buf[16];
  if (!readBytesI2C(0x20, buf, sizeof(buf))) return false;
  _measurement.systemTimeMs = decodeUInt32LE(buf + 0);
  _measurement.distanceMm = static_cast<int32_t>(decodeUInt32LE(buf + 4));
  _measurement.distanceM = _measurement.distanceMm / 1000.0f;
  uint32_t st = decodeUInt32LE(buf + 8);
  _measurement.distanceStatus = static_cast<uint16_t>(st & 0xFFFF);
  _measurement.signalStrength = static_cast<uint16_t>((st >> 16) & 0xFFFF);
  uint32_t prf = decodeUInt32LE(buf + 12);
  _measurement.rangePrecisionCm = static_cast<uint8_t>(prf & 0xFF);
  _measurement.id = (address >= 0x08) ? static_cast<uint8_t>(address - 0x08) : 0xFF;
  _measurement.validFrame = true; _measurement.validChecksum = true; _measurement.timestampMs = millis(); _hasNewMeasurement = true; setError(Error::OK); return true;
}
bool TOFSenseF::isConnectedI2C() { if (!_wire) { setError(Error::NoStream); return false; } _wire->beginTransmission(_i2cAddress); return _wire->endTransmission() == 0; }

bool TOFSenseF::readBytesI2C(uint8_t reg, uint8_t *buf, size_t len) {
  if (!_wire) { setError(Error::NoStream); return false; }
  _wire->beginTransmission(_i2cAddress); _wire->write(reg);
  if (_wire->endTransmission(false) != 0) { setError(Error::I2CNack); return false; }
  size_t got = _wire->requestFrom(static_cast<int>(_i2cAddress), static_cast<int>(len));
  if (got != len) { setError(Error::I2CShortRead); return false; }
  for (size_t i = 0; i < len; ++i) buf[i] = static_cast<uint8_t>(_wire->read());
  return true;
}
bool TOFSenseF::writeBytesI2C(uint8_t reg, const uint8_t *buf, size_t len) {
  if (!_wire) { setError(Error::NoStream); return false; }
  _wire->beginTransmission(_i2cAddress); _wire->write(reg); _wire->write(buf, len);
  if (_wire->endTransmission() != 0) { setError(Error::I2CNack); return false; }
  setError(Error::OK); return true;
}
#define READN(n,t) bool TOFSenseF::readRegister##n(uint8_t reg, t &v){uint8_t b[n/8]; if(!readBytesI2C(reg,b,sizeof(b))) return false; v=(t)decodeUInt##n##LE(b); setError(Error::OK); return true;}
READN(16,uint16_t) READN(24,uint32_t) READN(32,uint32_t)
#undef READN
bool TOFSenseF::readRegister8(uint8_t reg, uint8_t &value){ return readBytesI2C(reg,&value,1);} 
bool TOFSenseF::writeRegister8(uint8_t reg, uint8_t value){ return writeBytesI2C(reg,&value,1);} 
bool TOFSenseF::writeRegister16(uint8_t reg, uint16_t value){ uint8_t b[2]; encodeUInt16LE(b,value); return writeBytesI2C(reg,b,2);} 
bool TOFSenseF::writeRegister32(uint8_t reg, uint32_t value){ uint8_t b[4]; encodeUInt32LE(b,value); return writeBytesI2C(reg,b,4);} 

bool TOFSenseF::readProductVersion(uint16_t &v){ uint32_t raw; if(!readRegister32(0x00,raw)) return false; v=raw&0xFFFF; return true;}
bool TOFSenseF::readHardwareVersion(uint16_t &v){ uint32_t raw; if(!readRegister32(0x04,raw)) return false; v=raw&0xFFFF; return true;}
bool TOFSenseF::readBootloaderVersion(uint16_t &v){ uint32_t raw; if(!readRegister32(0x04,raw)) return false; v=(raw>>16)&0xFFFF; return true;}
bool TOFSenseF::readFirmwareVersion(uint32_t &v){ return readRegister32(0x08,v);} 
bool TOFSenseF::readDeviceConfig(uint32_t &v){ return readRegister32(0x0C,v);} 
bool TOFSenseF::readDeviceId(uint8_t &v){ uint32_t raw; if(!readDeviceConfig(raw)) return false; v=(raw>>8)&0xFF; return true;}
bool TOFSenseF::readInterfaceMode(InterfaceMode &m){ uint32_t raw; if(!readDeviceConfig(raw)) return false; m=(InterfaceMode)(raw&0x07); return true;}
bool TOFSenseF::readOutputMode(OutputMode &m){ uint32_t raw; if(!readDeviceConfig(raw)) return false; m=(OutputMode)((raw>>3)&0x01); return true;}
bool TOFSenseF::readUARTBaudrate(uint32_t &b){ return readRegister32(0x10,b);} 
bool TOFSenseF::readRefreshRate(uint16_t &hz){ uint32_t raw; if(!readRegister32(0x2C,raw)) return false; hz=(raw>>8)&0xFFFF; return true;}
bool TOFSenseF::readFilterFactor(uint8_t &f){ uint32_t raw; if(!readRegister32(0x2C,raw)) return false; f=(raw>>24)&0xFF; return true;}
bool TOFSenseF::readBandI2C(uint16_t &s,uint16_t &w){ uint32_t raw; if(!readRegister32(0x1C,raw)) return false; s=raw&0xFFFF; w=(raw>>16)&0xFFFF; return true;}
bool TOFSenseF::readDistanceMmI2C(uint32_t &d){ return readRegister32(0x24,d);} 
bool TOFSenseF::readStatusAndSignal(uint16_t &st,uint16_t &sg){ uint32_t raw; if(!readRegister32(0x28,raw)) return false; st=raw&0xFFFF; sg=(raw>>16)&0xFFFF; return true;}
bool TOFSenseF::readPrecisionRefreshFilter(uint8_t &p,uint16_t &r,uint8_t &f){ uint32_t raw; if(!readRegister32(0x2C,raw)) return false; p=raw&0xFF; r=(raw>>8)&0xFFFF; f=(raw>>24)&0xFF; return true;}

bool TOFSenseF::setDeviceIdI2C(uint8_t id){ if(id>111){setError(Error::InvalidParameter);return false;} uint32_t raw; if(!readDeviceConfig(raw)) return false; raw=(raw&~(0xFFu<<8))|((uint32_t)id<<8); return writeRegister32(0x0C,raw);} 
bool TOFSenseF::setInterfaceModeI2C(InterfaceMode mode){ uint32_t raw; if(!readDeviceConfig(raw)) return false; raw=(raw&~0x07u)|(static_cast<uint32_t>(mode)&0x07u); return writeRegister32(0x0C,raw);} 
bool TOFSenseF::setUARTBaudrateI2C(uint32_t baud){ if(!isSupportedBaud(baud)){setError(Error::UnsupportedBaudrate);return false;} return writeRegister32(0x10,baud);} 
bool TOFSenseF::setBandI2C(uint16_t s,uint16_t w){ return writeRegister32(0x1C,((uint32_t)w<<16)|s);} 
bool TOFSenseF::setRefreshRateAndFilterI2C(uint16_t hz,uint8_t factor){ if(!isSupportedRefresh(hz)){setError(Error::UnsupportedRefreshRate);return false;} uint32_t raw; if(!readRegister32(0x2C,raw)) return false; raw=(raw&0x000000FFu)|((uint32_t)hz<<8)|((uint32_t)factor<<24); return writeRegister32(0x2C,raw);} 

bool TOFSenseF::sendUARTSettingFrame(uint8_t id, InterfaceMode im, OutputMode om, RangeMode rm, uint32_t baud, uint16_t refresh, uint8_t filter, uint16_t bandStart, uint16_t bandWidth, bool readRequest){
  if(!_serial){setError(Error::NoStream);return false;} if(!isSupportedBaud(baud)){setError(Error::UnsupportedBaudrate);return false;} if(!isSupportedRefresh(refresh)){setError(Error::UnsupportedRefreshRate);return false;}
  uint8_t f[32]; for(uint8_t &x:f) x=0xFF; f[0]=kSettingHeader; f[1]=kSettingFunction; f[2]=readRequest?0x01:0x00; f[3]=0xFF; f[4]=id; encodeUInt32LE(f+5,millis()); f[9]=buildMode(im,om,rm); encodeUInt24LE(f+12,baud); encodeUInt16LE(f+19,bandStart); encodeUInt16LE(f+21,bandWidth); encodeUInt16LE(f+24,refresh); f[26]=filter; f[31]=checksum(f,31);
  return _serial->write(f,sizeof(f))==sizeof(f);
}
bool TOFSenseF::setUARTModeUART(uint8_t id, InterfaceMode im, OutputMode om, RangeMode rm, uint32_t baud, uint16_t hz, uint8_t ff){ return sendUARTSettingFrame(id,im,om,rm,baud,hz,ff); }
bool TOFSenseF::setToUARTActiveViaUART(uint8_t id){ return setUARTModeUART(id,InterfaceMode::UART,OutputMode::Active,RangeMode::Long,921600,50,5);} 
bool TOFSenseF::setToUARTQueryViaUART(uint8_t id){ return setUARTModeUART(id,InterfaceMode::UART,OutputMode::Query,RangeMode::Long,921600,50,5);} 
bool TOFSenseF::setToI2CViaUART(uint8_t id){ return setUARTModeUART(id,InterfaceMode::I2C,OutputMode::Active,RangeMode::Long,921600,50,5);} 
bool TOFSenseF::setToIOViaUART(uint8_t id,uint16_t s,uint16_t w){ return sendUARTSettingFrame(id,InterfaceMode::IO,OutputMode::Active,RangeMode::Long,921600,50,5,s,w,false);} 

TOFSenseF::Error TOFSenseF::lastError() const { return _lastError; }
const char* TOFSenseF::lastErrorString() const { switch(_lastError){case Error::OK:return"OK";case Error::NoStream:return"NoStream";case Error::Timeout:return"Timeout";case Error::BadHeader:return"BadHeader";case Error::BadFunction:return"BadFunction";case Error::BadChecksum:return"BadChecksum";case Error::I2CNack:return"I2CNack";case Error::I2CShortRead:return"I2CShortRead";case Error::InvalidParameter:return"InvalidParameter";case Error::UnsupportedBaudrate:return"UnsupportedBaudrate";case Error::UnsupportedRefreshRate:return"UnsupportedRefreshRate";} return "Unknown"; }
void TOFSenseF::setError(Error e){ _lastError=e; }

uint8_t TOFSenseF::checksum(const uint8_t *data, size_t n){ uint8_t s=0; for(size_t i=0;i<n;++i) s+=data[i]; return s; }
bool TOFSenseF::verifyChecksum(const uint8_t *data, size_t n){ return n>1 && checksum(data,n-1)==data[n-1]; }
int32_t TOFSenseF::decodeInt24LE(const uint8_t *p){ uint32_t raw=decodeUInt24LE(p); if(raw&0x800000) raw|=0xFF000000; return (int32_t)raw; }
uint16_t TOFSenseF::decodeUInt16LE(const uint8_t *p){ return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }
uint32_t TOFSenseF::decodeUInt24LE(const uint8_t *p){ return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16); }
uint32_t TOFSenseF::decodeUInt32LE(const uint8_t *p){ return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
void TOFSenseF::encodeUInt16LE(uint8_t *p,uint16_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
void TOFSenseF::encodeUInt24LE(uint8_t *p,uint32_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; }
void TOFSenseF::encodeUInt32LE(uint8_t *p,uint32_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }

bool TOFSenseF::isSupportedBaud(uint32_t b){ static const uint32_t v[]={4800,9600,14400,19200,38400,43000,57600,76800,115200,230400,460800,921600,1000000,1200000,1500000,2000000,3000000}; for(uint32_t x:v) if(x==b) return true; return false; }
bool TOFSenseF::isSupportedRefresh(uint16_t hz){ static const uint16_t v[]={1,2,5,10,25,50,100,200,350}; for(uint16_t x:v) if(x==hz) return true; return false; }
uint8_t TOFSenseF::buildMode(InterfaceMode im, OutputMode om, RangeMode rm){ return (static_cast<uint8_t>(im)&0x07) | ((static_cast<uint8_t>(om)&0x01)<<3) | ((static_cast<uint8_t>(rm)&0x03)<<4); }
