#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <Arduino.h>

#define MAX_SCHEDULE_ITEMS 20
#define EEPROM_SIZE 512
#define MAGIC_NUMBER 0xBE11

struct BellEvent {
  uint8_t hour;
  uint8_t minute;
  bool active;
  uint8_t duration;
};

// Global extern declarations
extern BellEvent schedule[MAX_SCHEDULE_ITEMS];
extern int eventCount;
extern bool isRinging;

#endif
