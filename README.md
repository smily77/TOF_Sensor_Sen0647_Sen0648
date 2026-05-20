# DFRobot_TOFSenseF

Eine vollständige Arduino-Library für DFRobot/Nooploop TOF-Sensoren der TOFSense-F Familie – insbesondere:

- **SEN0647** (25 m)
- **SEN0648** (50 m)
- voraussichtlich kompatibel mit weiteren NLink_TOFSense-basierten Varianten (z. B. SEN0646)

Die Sensoren nutzen dasselbe Grundprotokoll. Unterschiede liegen primär bei Optik/Reichweite, nicht bei UART/I2C-Protokoll.

---

## Inhaltsverzeichnis

1. [Features](#features)
2. [Installation](#installation)
3. [Elektrischer Anschluss](#elektrischer-anschluss)
4. [Schnellstart](#schnellstart)
5. [Betriebsmodi](#betriebsmodi)
6. [Vollständige API-Dokumentation](#vollständige-api-dokumentation)
7. [UART-Protokollübersicht](#uart-protokollübersicht)
8. [I2C-Registerübersicht](#i2c-registerübersicht)
9. [Fehlerbehandlung](#fehlerbehandlung)
10. [Hinweise zu Genauigkeit & Outdoor](#hinweise-zu-genauigkeit--outdoor)
11. [Beispiele](#beispiele)
12. [Sicherheits-/Konfigurationshinweise](#sicherheits-konfigurationshinweise)

---

## Features

- Nicht-blockierender UART-Parser (16-Byte Messframes)
- UART Active Mode und UART Query Mode
- Vollständiger I2C Registerzugriff (lesen/schreiben)
- High-Level APIs für Distanz, Status, Signalstärke, Präzision
- Konfigurationsfunktionen für ID, Interface, Baudrate, Refresh/Filter, IO-Band
- Hilfsfunktionen für Checksum und Little-Endian Kodierung
- Fehlerstatus mit `lastError()` und `lastErrorString()`
- Plattformen: ESP32/ESP32-S3, AVR (UNO soweit hardwareseitig möglich), SAMD u. a.

---

## Installation

1. Dieses Repository als ZIP laden.
2. In Arduino IDE: **Sketch → Bibliothek einbinden → .ZIP-Bibliothek hinzufügen**.
3. Alternativ als Ordner `DFRobot_TOFSenseF` in den Arduino `libraries` Pfad kopieren.

---

## Elektrischer Anschluss

### UART

- Sensor **TX** → MCU **RX**
- Sensor **RX** → MCU **TX**
- **GND gemeinsam**
- Logikpegel beachten (typisch 3.3 V)
- Default: **921600 Baud, 8N1**

### I2C

- Sensor **SDA** ↔ MCU **SDA**
- Sensor **SCL** ↔ MCU **SCL**
- Pullups auf SDA/SCL erforderlich
- 7-bit Basisadresse: **0x08**
- Effektive Adresse: **0x08 + ID**
- In Arduino `Wire` immer 7-bit Adresse verwenden (also `0x08`, nicht `0x10/0x11`)

---

## Schnellstart

### UART Active

```cpp
#include <TOFSenseF.h>

TOFSenseF tof;

void setup() {
  Serial.begin(115200);
  Serial2.begin(921600, SERIAL_8N1, 16, 17); // ESP32 Beispielpins
  tof.beginUART(Serial2, 0);
}

void loop() {
  if (tof.read()) {
    Serial.println(tof.distanceMm());
  }
}
```

### I2C

```cpp
#include <Wire.h>
#include <TOFSenseF.h>

TOFSenseF tof;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  tof.beginI2C(Wire, 0x08);
}

void loop() {
  if (tof.readI2C()) {
    Serial.println(tof.distanceM(), 3);
  }
}
```

---

## Betriebsmodi

- **UART Active**: Sensor sendet kontinuierlich Messframes.
- **UART Query**: Sensor antwortet nur auf Anfrageframe.
- **I2C**: Mess- und Konfigurationsdaten über Register.
- **IO**: Threshold-/Hysterese-Ausgänge, keine Distanzstream-Ausgabe.

---

## Vollständige API-Dokumentation

## Klasse `TOFSenseF`

### Enums

- `InterfaceMode { UART=0, CAN=1, IO=2, I2C=3 }`
- `OutputMode { Active=0, Query=1 }`
- `RangeMode { Short=0, Medium=1, Long=2 }`
- `Model { Unknown, SEN0647_25M, SEN0648_50M }`
- `Error { OK, NoStream, Timeout, BadHeader, BadFunction, BadChecksum, I2CNack, I2CShortRead, InvalidParameter, UnsupportedBaudrate, UnsupportedRefreshRate }`

### Struct `Measurement`

- `uint8_t id` – Modul-ID
- `uint32_t systemTimeMs` – Sensor-Systemzeit in ms
- `int32_t distanceMm` – Distanz in mm
- `float distanceM` – Distanz in m
- `uint16_t distanceStatus` – Distanzstatus
- `uint16_t signalStrength` – Signalstärke
- `uint8_t rangePrecisionCm` – Präzision in cm
- `bool validChecksum` – UART Frame-Checksum gültig
- `bool validFrame` – Frame formal gültig
- `uint32_t timestampMs` – MCU-Zeitpunkt (`millis()`) der letzten Messung

### Konstruktor

- `TOFSenseF();`

### UART API

- `bool beginUART(Stream &serial, uint8_t id = 0);`
  - Initialisiert die Library mit beliebigem Arduino-`Stream`.
- `bool beginUART(HardwareSerial &serial, uint32_t baud, int8_t rxPin, int8_t txPin, uint8_t id = 0);` *(ESP32 only)*
  - Komfortfunktion für `HardwareSerial.begin(...)` mit Pinangabe.
- `void setUARTStream(Stream &serial);`
  - UART-Stream nachträglich setzen/wechseln.
- `bool read();`
  - Nicht-blockierendes Parsen eingehender UART-Bytes.
  - `true`, sobald ein kompletter gültiger Messframe geparst wurde.
- `bool available();`
  - `true`, wenn eine neue Messung vorhanden ist.
- `const Measurement& measurement() const;`
  - Zugriff auf letzte Messung.
- `int32_t distanceMm() const;`
- `float distanceM() const;`
- `uint8_t id() const;`
- `uint16_t status() const;`
- `uint16_t signalStrength() const;`
- `uint8_t rangePrecisionCm() const;`

### UART Query

- `bool requestUART(uint8_t id);`
  - Sendet den 8-Byte Query-Frame (`0x57 0x10 ... checksum`).
- `bool requestAndReadUART(uint8_t id, uint32_t timeoutMs = 100);`
  - Sendet Query und wartet bis Timeout auf eine gültige Antwort.

### I2C Basis

- `bool beginI2C(TwoWire &wire = Wire, uint8_t address = 0x08);`
- `bool readI2C();`
- `bool readI2C(uint8_t address);`
- `bool isConnectedI2C();`

`readI2C()` liest blockweise `0x20..0x2F` (16 Bytes):

- `0x20`: Systemzeit
- `0x24`: Distanz mm
- `0x28`: Status + Signal
- `0x2C`: Präzision + Refresh + Filter

### I2C Registerzugriff

- `bool readRegister8(uint8_t reg, uint8_t &value);`
- `bool readRegister16(uint8_t reg, uint16_t &value);`
- `bool readRegister24(uint8_t reg, uint32_t &value);`
- `bool readRegister32(uint8_t reg, uint32_t &value);`
- `bool writeRegister8(uint8_t reg, uint8_t value);`
- `bool writeRegister16(uint8_t reg, uint16_t value);`
- `bool writeRegister32(uint8_t reg, uint32_t value);`

### I2C Informationsfunktionen

- `bool readProductVersion(uint16_t &productVersion);`
- `bool readHardwareVersion(uint16_t &hardwareVersion);`
- `bool readBootloaderVersion(uint16_t &bootloaderVersion);`
- `bool readFirmwareVersion(uint32_t &firmwareVersion);`
- `bool readDeviceConfig(uint32_t &rawConfig);`
- `bool readDeviceId(uint8_t &id);`
- `bool readInterfaceMode(InterfaceMode &mode);`
- `bool readOutputMode(OutputMode &mode);`
- `bool readUARTBaudrate(uint32_t &baud);`
- `bool readRefreshRate(uint16_t &hz);`
- `bool readFilterFactor(uint8_t &factor);`
- `bool readBandI2C(uint16_t &bandStartMm, uint16_t &bandWidthMm);`
- `bool readDistanceMmI2C(uint32_t &distanceMm);`
- `bool readStatusAndSignal(uint16_t &status, uint16_t &signal);`
- `bool readPrecisionRefreshFilter(uint8_t &precisionCm, uint16_t &refreshHz, uint8_t &filterFactor);`

### I2C Konfiguration

> Änderungen können erst nach Power-Cycle wirksam werden.

- `bool setDeviceIdI2C(uint8_t id);` (`0..111`)
- `bool setInterfaceModeI2C(InterfaceMode mode);`
- `bool setUARTBaudrateI2C(uint32_t baud);`
- `bool setBandI2C(uint16_t bandStartMm, uint16_t bandWidthMm);`
- `bool setRefreshRateAndFilterI2C(uint16_t refreshHz, uint8_t filterFactor);`

### UART Konfigurationsframe (Setting Frame)

- `bool sendUARTSettingFrame(...)`
  - Low-Level API für Setting-Frame (`0x54`, `0x20`).
- `bool setUARTModeUART(...)`
  - Allgemeine High-Level Funktion.
- `bool setToUARTActiveViaUART(uint8_t id = 0);`
- `bool setToUARTQueryViaUART(uint8_t id = 0);`
- `bool setToI2CViaUART(uint8_t id = 0);`
- `bool setToIOViaUART(uint8_t id = 0, uint16_t bandStartMm = 1000, uint16_t bandWidthMm = 0);`

> **Achtung:** Das Umschalten auf I2C kann den Sensor über UART/NAssistant „unsichtbar“ machen.

### Fehlerbehandlung

- `Error lastError() const;`
- `const char* lastErrorString() const;`

Empfohlener Ablauf:

```cpp
if (!tof.readI2C()) {
  Serial.println(tof.lastErrorString());
}
```

### Statische Helper

- `static uint8_t checksum(const uint8_t *data, size_t lengthWithoutChecksum);`
- `static bool verifyChecksum(const uint8_t *data, size_t fullLength);`
- `static int32_t decodeInt24LE(const uint8_t *p);`
- `static uint16_t decodeUInt16LE(const uint8_t *p);`
- `static uint32_t decodeUInt24LE(const uint8_t *p);`
- `static uint32_t decodeUInt32LE(const uint8_t *p);`
- `static void encodeUInt16LE(uint8_t *p, uint16_t v);`
- `static void encodeUInt24LE(uint8_t *p, uint32_t v);`
- `static void encodeUInt32LE(uint8_t *p, uint32_t v);`

---

## UART-Protokollübersicht

### Messframe (Active/Antwort auf Query)

- Länge: 16 Bytes
- Byte 0: `0x57`
- Byte 1: `0x00`
- Byte 8..10: Distanz uint24 little-endian (praktisch mm)
- Byte 15: Checksum (`sum(frame[0..14]) & 0xFF`)

### Queryframe

- Länge: 8 Bytes
- Header `0x57`, Function `0x10`
- Checksum auf Byte 0..6

---

## I2C-Registerübersicht

- `0x00` Product Version (RO)
- `0x04` Hardware/Bootloader Version (RO)
- `0x08` Firmware Version (RO)
- `0x0C` Device Config (RW)
  - Bits 0..2 Interface
  - Bit 3 Output Mode
  - Bit 6 Low Power
  - Bits 8..15 Device ID
- `0x10` UART Baudrate (RW)
- `0x1C` Band Start / Band Width (RW)
- `0x20` Systime (RO)
- `0x24` Distance mm (RO)
- `0x28` Status + Signal (RO)
- `0x2C` Precision + Refresh + Filter (RW)

---

## Fehlerbehandlung

Mögliche `Error` Werte:

- `OK`
- `NoStream`
- `Timeout`
- `BadHeader`
- `BadFunction`
- `BadChecksum`
- `I2CNack`
- `I2CShortRead`
- `InvalidParameter`
- `UnsupportedBaudrate`
- `UnsupportedRefreshRate`

---

## Hinweise zu Genauigkeit & Outdoor

- Starkes Sonnenlicht kann Reichweite und Genauigkeit verschlechtern.
- Unter schwierigen Lichtbedingungen steigt das Rauschen/Schwanken.
- Der Sensor liefert immer **eine Distanz** (kein Point-Cloud/LiDAR-Scanbild).

---

## Beispiele

- `examples/UART_Active/UART_Active.ino`
- `examples/UART_Query/UART_Query.ino`
- `examples/I2C_Read/I2C_Read.ino`
- `examples/I2C_Configure/I2C_Configure.ino`
- `examples/UART_SetMode/UART_SetMode.ino`

---

## Sicherheits-/Konfigurationshinweise

1. Vor dem Mode-Switch immer aktuelle Konfiguration dokumentieren.
2. Wechsel auf I2C nur durchführen, wenn I2C-Zugriff gesichert ist.
3. Nach ID- oder Interface-Wechsel ggf. Power-Cycle durchführen.
4. Nicht jede Refresh-Rate ist auf jedem Modell sinnvoll/stabil.
5. Für produktive Systeme konservative Defaults verwenden (`921600`, `50 Hz`, Filter `5`).

---

## Lizenz

Nutzung auf eigenes Risiko. Bitte Datenblatt/Manual des jeweiligen Sensors beachten.
