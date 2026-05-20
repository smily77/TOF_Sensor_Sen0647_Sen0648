#pragma once

#include <Arduino.h>
#include <Wire.h>

class TOFSenseF {
public:
  enum class InterfaceMode : uint8_t { UART = 0, CAN = 1, IO = 2, I2C = 3 };
  enum class OutputMode : uint8_t { Active = 0, Query = 1 };
  enum class RangeMode : uint8_t { Short = 0, Medium = 1, Long = 2 };
  enum class Model : uint8_t { Unknown, SEN0647_25M, SEN0648_50M };
  enum class Error : uint8_t {
    OK, NoStream, Timeout, BadHeader, BadFunction, BadChecksum, I2CNack,
    I2CShortRead, InvalidParameter, UnsupportedBaudrate, UnsupportedRefreshRate
  };

  struct Measurement {
    uint8_t id = 0;
    uint32_t systemTimeMs = 0;
    int32_t distanceMm = 0;
    float distanceM = 0.0f;
    uint16_t distanceStatus = 0;
    uint16_t signalStrength = 0;
    uint8_t rangePrecisionCm = 0;
    bool validChecksum = false;
    bool validFrame = false;
    uint32_t timestampMs = 0;
  };

  TOFSenseF();
  bool beginUART(Stream &serial, uint8_t id = 0);
#if defined(ARDUINO_ARCH_ESP32)
  bool beginUART(HardwareSerial &serial, uint32_t baud, int8_t rxPin, int8_t txPin, uint8_t id = 0);
#endif
  void setUARTStream(Stream &serial);
  bool read();
  bool available();
  const Measurement& measurement() const;
  int32_t distanceMm() const;
  float distanceM() const;
  uint8_t id() const;
  uint16_t status() const;
  uint16_t signalStrength() const;
  uint8_t rangePrecisionCm() const;

  bool requestUART(uint8_t id);
  bool requestAndReadUART(uint8_t id, uint32_t timeoutMs = 100);

  bool beginI2C(TwoWire &wire = Wire, uint8_t address = 0x08);
  bool readI2C();
  bool readI2C(uint8_t address);
  bool isConnectedI2C();

  bool readRegister8(uint8_t reg, uint8_t &value);
  bool readRegister16(uint8_t reg, uint16_t &value);
  bool readRegister24(uint8_t reg, uint32_t &value);
  bool readRegister32(uint8_t reg, uint32_t &value);
  bool writeRegister8(uint8_t reg, uint8_t value);
  bool writeRegister16(uint8_t reg, uint16_t value);
  bool writeRegister32(uint8_t reg, uint32_t value);

  bool readProductVersion(uint16_t &productVersion);
  bool readHardwareVersion(uint16_t &hardwareVersion);
  bool readBootloaderVersion(uint16_t &bootloaderVersion);
  bool readFirmwareVersion(uint32_t &firmwareVersion);
  bool readDeviceConfig(uint32_t &rawConfig);
  bool readDeviceId(uint8_t &id);
  bool readInterfaceMode(InterfaceMode &mode);
  bool readOutputMode(OutputMode &mode);
  bool readUARTBaudrate(uint32_t &baud);
  bool readRefreshRate(uint16_t &hz);
  bool readFilterFactor(uint8_t &factor);
  bool readBandI2C(uint16_t &bandStartMm, uint16_t &bandWidthMm);
  bool readDistanceMmI2C(uint32_t &distanceMm);
  bool readStatusAndSignal(uint16_t &status, uint16_t &signal);
  bool readPrecisionRefreshFilter(uint8_t &precisionCm, uint16_t &refreshHz, uint8_t &filterFactor);

  bool setDeviceIdI2C(uint8_t id);
  bool setInterfaceModeI2C(InterfaceMode mode);
  bool setUARTBaudrateI2C(uint32_t baud);
  bool setBandI2C(uint16_t bandStartMm, uint16_t bandWidthMm);
  bool setRefreshRateAndFilterI2C(uint16_t refreshHz, uint8_t filterFactor);

  bool sendUARTSettingFrame(uint8_t id, InterfaceMode interfaceMode, OutputMode outputMode, RangeMode rangeMode,
                            uint32_t baud, uint16_t refreshHz, uint8_t filterFactor,
                            uint16_t bandStartMm = 1000, uint16_t bandWidthMm = 0, bool readRequest = false);
  bool setUARTModeUART(uint8_t id, InterfaceMode interfaceMode, OutputMode outputMode, RangeMode rangeMode,
                       uint32_t baud, uint16_t refreshHz, uint8_t filterFactor);
  bool setToUARTActiveViaUART(uint8_t id = 0);
  bool setToUARTQueryViaUART(uint8_t id = 0);
  bool setToI2CViaUART(uint8_t id = 0);
  bool setToIOViaUART(uint8_t id = 0, uint16_t bandStartMm = 1000, uint16_t bandWidthMm = 0);

  Error lastError() const;
  const char* lastErrorString() const;

  static uint8_t checksum(const uint8_t *data, size_t lengthWithoutChecksum);
  static bool verifyChecksum(const uint8_t *data, size_t fullLength);
  static int32_t decodeInt24LE(const uint8_t *p);
  static uint16_t decodeUInt16LE(const uint8_t *p);
  static uint32_t decodeUInt24LE(const uint8_t *p);
  static uint32_t decodeUInt32LE(const uint8_t *p);
  static void encodeUInt16LE(uint8_t *p, uint16_t v);
  static void encodeUInt24LE(uint8_t *p, uint32_t v);
  static void encodeUInt32LE(uint8_t *p, uint32_t v);

private:
  static constexpr uint8_t kUartHeader = 0x57;
  static constexpr uint8_t kUartOutputFunction = 0x00;
  static constexpr uint8_t kUartQueryFunction = 0x10;
  static constexpr uint8_t kSettingHeader = 0x54;
  static constexpr uint8_t kSettingFunction = 0x20;
  static constexpr uint8_t kUartFrameLen = 16;

  Stream *_serial;
  TwoWire *_wire;
  uint8_t _id;
  uint8_t _i2cAddress;
  Error _lastError;
  Measurement _measurement;
  bool _hasNewMeasurement;
  uint8_t _frameBuffer[kUartFrameLen];
  uint8_t _frameIndex;

  void setError(Error e);
  bool parseFrame(const uint8_t *frame);
  bool readBytesI2C(uint8_t reg, uint8_t *buf, size_t len);
  bool writeBytesI2C(uint8_t reg, const uint8_t *buf, size_t len);
  static bool isSupportedBaud(uint32_t baud);
  static bool isSupportedRefresh(uint16_t hz);
  static uint8_t buildMode(InterfaceMode interfaceMode, OutputMode outputMode, RangeMode rangeMode);
};
