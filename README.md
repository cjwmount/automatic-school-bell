# Hardware & Display Module - School Bell System

## My Role
Hardware initialization, display driver, LED indicators, and buzzer control.

## Files I Created
- `src/hardware/display.cpp/h` - ST7735S screen management
- `src/hardware/leds.cpp/h` - RGB LED control (Green/Red/Blue/White)
- `src/hardware/buzzer.cpp/h` - Tone generation for bell ringing
- `include/config.h` - Pin definitions for all components

## Pin Connections
| Component | Pins |
|-----------|------|
| TFT Display | CS:5, RST:16, DC:17, MOSI:23, SCLK:18 |
| LEDs | Green:26, Red:27, Blue:14, White:12 |
| Buzzer | GPIO 25 |

## Functions Implemented
- `initDisplay()` - Boot screen and display setup
- `updateDisplay()` - Real-time clock, date, WiFi status
- `ringBell(duration)` - Buzzer with LED flash pattern
- `heartbeatLED()` - System alive indicator (blinks every 5 sec)

## Testing
✅ Display shows time and schedule info  
✅ LEDs respond to boot/ringing/ready states  
✅ Buzzer produces 2KHz tone during bell events  

## Dependencies
- Adafruit_GFX
- Adafruit_ST7735
- SPI library

## Merge Status
⏳ Waiting for Person 2 (RTC module) and Person 3 (Web module)
