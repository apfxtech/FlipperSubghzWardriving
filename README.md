SubGhz Wardriving application for Unleashed firmware. Application can capture and save (autosave) SubGhz signals with GPS position and save it to .sub files.

## GPS
The GPS source is selected in Radio Settings -> GPS:

- **OFF** - no GPS.
- **NMEA** - external GPS module over UART (NMEA sentences). Set the module speed in GPS Baudrate.
- **UBX** - external u-blox module over UART (UBX binary, NAV-PVT). Set the module speed in GPS Baudrate.
- **RPC** - location streamed from a connected companion app (phone) over USB/BLE. No external module needed; GPS Baudrate is ignored.

NMEA and Ubox use the USART pins (13 TX, 14 RX): connect the module TX to pin 14, module RX to pin 13, plus 3V3 and GND. NMEA/Ubox load a small plugin; RPC talks to the firmware GPS service directly. Only one source is active at a time.

The selected source is applied on app start, so after changing it in Radio Settings reopen the app.

Based on MMX (https://github.com/xMasterX) idea and SubGhz code from Momentum firmware by WillyJL (https://github.com/WillyJL).