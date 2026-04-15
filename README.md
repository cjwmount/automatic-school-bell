# RTC & Schedule Module - School Bell System

## My Role
Time management, schedule storage, bell event logic, and EEPROM persistence.

## Files I Created
- `src/core/rtc_manager.cpp/h` - DS3231 RTC integration
- `src/core/schedule.cpp/h` - Bell schedule CRUD operations
- `src/core/storage.cpp/h` - EEPROM save/load with magic number
- `include/structures.h` - Shared data structures (BellEvent)

## Features Implemented
- RTC initialization with lost power detection
- Auto-sync time from compiler (`__DATE__`, `__TIME__`)
- Schedule: Add, Delete, List, Find next bell
- EEPROM persistence (20 events max)
- Default school schedule (6 events)

## Schedule Structure
```cpp
struct BellEvent {
  uint8_t hour;    // 0-23
  uint8_t minute;  // 0-59
  bool active;     // enabled/disabled
  uint8_t duration; // ringing seconds (1-10)
};
