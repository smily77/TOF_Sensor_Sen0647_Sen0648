# DFRobot_TOFSenseF

Arduino Library für DFRobot/Nooploop TOF Serie (SEN0647 25m, SEN0648 50m; voraussichtlich kompatibel zu SEN0646), basierend auf dem NLink_TOFSense-Protokoll.

## Features
- UART Active Mode (nicht blockierender 16-Byte Parser)
- UART Query Mode (`requestUART`, `requestAndReadUART`)
- I2C Registerzugriff inkl. Block-Read 0x20..0x2F
- Konfigurationshilfen für Interface/ID/Baud/Refresh/Filter
- Optionale IO-Threshold-Konfiguration

## Wichtige Defaults
- UART: **921600**, 8N1, little-endian Frames
- UART Messframe: Header `0x57`, Function `0x00`, Länge 16
- UART Query Frame: Function `0x10`, Länge 8
- I2C 7-bit Basisadresse: `0x08`, effektiv `0x08 + ID`

## Anschluss
### UART
- Sensor TX -> MCU RX
- Sensor RX -> MCU TX
- GND gemeinsam
- Logikpegel 3.3V beachten

### I2C
- SDA/SCL mit Pullups
- In Arduino `Wire` immer 7-bit Adresse nutzen (z. B. `0x08`), **nicht** `0x10/0x11`.

## Distanzdaten
- UART: uint24 little-endian, effektiv mm
- I2C Register 0x24: uint32 mm
- `distanceM = distanceMm / 1000.0`

## Modi
- UART Active: Sensor streamt Messframes
- UART Query: Sensor antwortet nur auf Query
- I2C: Controller liest Register
- IO: Threshold/Hysterese Ausgänge statt Distanzdaten

## Warnung zu Mode-Wechsel
Wenn auf **I2C** umgestellt wird, ist der Sensor ggf. über UART/NAssistant nicht mehr erreichbar. Rückweg dann über I2C oder Recovery-Sequenz. Interfacewechsel vorsichtig einsetzen; ggf. Power-Cycle nötig.

## IO Mode Kurzinfo
- Single Threshold: BandStart=1000, BandWidth=0
- Hysterese: low=BandStart, high=BandStart+BandWidth
- Ausgänge sind komplementär, low-current (Treiber/Relaisstufe für Lasten)

## Outdoor-Hinweis
Starkes Sonnenlicht reduziert Reichweite/Genauigkeit und erhöht Messschwankungen.

## Beispiele
- `examples/UART_Active`
- `examples/UART_Query`
- `examples/I2C_Read`
- `examples/I2C_Configure`
- `examples/UART_SetMode`
