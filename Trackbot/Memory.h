// Memory.h — Pip remembers across power-offs (ESP32 flash, "Preferences").
// Saved: mood cores (how he's been treated, incl. trust), days alive, last calendar day.
// Daily routine: a new calendar day bumps the day counter; first wake in the morning = greeting.
#pragma once
#include <Preferences.h>
#include <time.h>
#include "config.h"
#include "Brain.h"

class Memory {
 public:
  uint16_t days = 1;          // "days alive"
  uint32_t boots = 0;
  bool morningDue = false;    // Trackbot.ino fires EV_MORNING when he's free

  void begin() {
    p.begin("pip", false);
    boots = p.getUInt("boots", 0) + 1; p.putUInt("boots", boots);
    days = p.getUShort("days", 1); lastDay = p.getInt("lastDay", -1);
    if (p.getBytesLength("core") == sizeof(brain.core)) {
      p.getBytes("core", brain.core, sizeof(brain.core));
      for (int i = 0; i < M_COUNT; i++) brain.ring[i] = brain.core[i];     // wakes up feeling like his recent days
      brain.ring[M_BORED] = 20; if (brain.ring[M_ENERGY] < 60) brain.ring[M_ENERGY] = 60;
    }
    lastSave = millis();
  }

  void save() {
    p.putBytes("core", brain.core, sizeof(brain.core));
    p.putUShort("days", days); p.putInt("lastDay", lastDay);
    lastSave = millis();
  }

  void update() {
    if (millis() - lastSave > SAVE_EVERY_MS) save();
    if (millis() - lastClock < 5000) return;
    lastClock = millis();
    time_t now = time(nullptr);
    if (now < 1700000000) return;                       // clock not set yet (needs WiFi/NTP)
    struct tm t; localtime_r(&now, &t);
    int id = t.tm_year * 400 + t.tm_yday;
    if (lastDay < 0) { lastDay = id; save(); }
    else if (id != lastDay) { lastDay = id; days++; save(); newDay = true; }
    if ((newDay || firstCheck) && t.tm_hour >= 5 && t.tm_hour < 11) { morningDue = true; newDay = false; }   // morning greeting
    firstCheck = false;
  }

  void reset() {                                         // "factory reset" his personality
    p.clear(); days = 1; lastDay = -1;
    const float def[M_COUNT] = {55, 15, 5, 70, 20, 50, 60};
    for (int i = 0; i < M_COUNT; i++) { brain.ring[i] = def[i]; brain.core[i] = def[i]; }
  }

 private:
  Preferences p;
  int lastDay = -1;
  uint32_t lastSave = 0, lastClock = 0;
  bool newDay = false, firstCheck = true;
};

Memory memory;
