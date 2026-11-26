// Brain.h — Pip's moods. Each mood has a fast "ring" (right now) and a slow "core" (his day),
// RDR2-style. Events push the ring; the ring drifts back to the core; the core slowly follows.
// To add an event: add an id + a row in EVFX (mood changes) + behaviors in Behaviors.h.
#pragma once
#include <time.h>
#include "config.h"
#include "EyeEngine.h"

enum Mood : uint8_t { M_HAPPY, M_SAD, M_ANGRY, M_ENERGY, M_BORED, M_CURIOUS, M_TRUST, M_COUNT };
// AUTO = free to do whatever | MANUAL = only the phone moves him | FOCUS = napping with a timer
enum Mode : uint8_t { MODE_AUTO, MODE_MANUAL, MODE_FOLLOW, MODE_WANDER, MODE_SLEEP, MODE_FOCUS, MODE_COUNT };

enum Event : uint8_t {
  EV_NONE, EV_IDLE, EV_DREAM, EV_PET, EV_TAP, EV_OVERPET, EV_HAND_APPEAR, EV_HAND_FAST, EV_HAND_LOST,
  EV_COVERED, EV_PEEKABOO, EV_FISTBUMP, EV_PICKED_UP, EV_SHAKEN, EV_UPSIDE, EV_KNOCK, EV_SETDOWN,
  EV_EDGE, EV_BUMP, EV_APP_CONNECT, EV_APP_DISCONNECT, EV_STICKS_IDLE, EV_MUSIC_START, EV_HIGHFIVE, EV_MORNING, EV_TIMER_DONE,
  EV_SLEEP, EV_WAKE, EV_COUNT
};

// Mood change per event: {happy, sad, angry, energy, bored, curious, trust}, fear spike, HUD log text
struct EvFx { int8_t d[M_COUNT]; uint8_t fear; const char* log; };
const EvFx EVFX[EV_COUNT] = {
  /* NONE        */ {{0, 0, 0, 0, 0, 0, 0}, 0, ""},
  /* IDLE        */ {{0, 0, 0, 0, 0, 0, 0}, 0, ""},
  /* DREAM       */ {{0, 0, 0, 0, 0, 0, 0}, 0, ""},
  /* PET         */ {{8, -6, -8, 0, -10, 0, 2}, 0, "petted"},
  /* TAP         */ {{1, 0, 0, 1, -8, 6, 0}, 0, "tapped"},
  /* OVERPET     */ {{-4, 0, 6, 0, 0, 0, 0}, 0, "too much petting"},
  /* HAND_APPEAR */ {{3, -3, 0, 2, -15, 15, 0}, 0, "saw a hand"},
  /* HAND_FAST   */ {{0, 0, 2, 5, -15, 0, -1}, 60, "startled"},
  /* HAND_LOST   */ {{-2, 4, 0, 0, 3, -5, 0}, 0, "hand left"},
  /* COVERED     */ {{0, 0, 0, 0, -10, 10, 0}, 10, "covered"},
  /* PEEKABOO    */ {{8, -4, 0, 4, -15, 5, 1}, 0, "peekaboo"},
  /* FISTBUMP    */ {{8, -3, -2, 3, -15, 0, 2}, 0, "fist bump"},
  /* PICKED_UP   */ {{0, 0, 2, 8, -20, 5, 0}, 50, "picked up"},
  /* SHAKEN      */ {{-15, 3, 42, 5, -20, 0, -8}, 40, "shaken"},
  /* UPSIDE      */ {{-8, 3, 12, 0, -20, 0, -4}, 70, "upside down"},
  /* KNOCK       */ {{0, 0, 2, 2, -10, 12, 0}, 20, "knock"},
  /* SETDOWN     */ {{3, 0, -3, 0, 0, 0, 1}, 0, "set down"},
  /* EDGE        */ {{0, 0, 0, 2, -5, 0, 0}, 50, "table edge"},
  /* BUMP        */ {{-2, 0, 3, 0, -5, 0, 0}, 25, "bumped"},
  /* APP_CONNECT */ {{10, -10, 0, 5, -25, 8, 0}, 0, "you checked in"},
  /* APP_DISCON  */ {{-3, 6, 0, 0, 5, 0, 0}, 0, "you left"},
  /* STICKS_IDLE */ {{0, 0, 0, 0, 0, 0, 0}, 0, ""},
  /* MUSIC_START */ {{15, -10, -5, 15, -40, 0, 0}, 0, "music"},
  /* HIGHFIVE    */ {{10, -4, -3, 4, -15, 0, 3}, 0, "high five"},
  /* MORNING     */ {{8, -5, -5, 10, -20, 5, 0}, 0, "good morning"},
  /* TIMER_DONE  */ {{6, 0, 0, 0, -20, 0, 0}, 0, "focus time done"},
  /* SLEEP       */ {{0, 0, 0, 0, 0, 0, 0}, 0, "fell asleep"},
  /* WAKE        */ {{0, 0, 0, 0, -20, 0, 0}, 0, "woke up"},
};

class Brain {
 public:
  //                      happy sad angry energy bored curious trust
  float ring[M_COUNT] = {55,   15,  5,    70,    20,   50,     60};
  float core[M_COUNT] = {55,   15,  5,    70,    20,   50,     60};
  float fear = 0;
  bool asleep = false;
  Mode mode = MODE_AUTO;
  const char* lastLog = "";
  uint32_t logSeq = 0;

  // Report something that happened. Safe to call from the main loop only.
  void event(Event e) {
    if (mode == MODE_FOCUS && e != EV_TIMER_DONE) return;     // nothing disturbs a focus nap
    uint32_t now = millis();
    lastEv[e] = now;
    if (e > EV_DREAM && e != EV_STICKS_IDLE && e != EV_SLEEP) lastActivity = now;
    if (asleep && e > EV_DREAM && e != EV_SLEEP && e != EV_WAKE && e != EV_APP_DISCONNECT) {   // anything wakes him
      asleep = false;
      if (e == EV_TAP || e == EV_KNOCK || e == EV_SHAKEN) ring[M_ANGRY] = clampf(ring[M_ANGRY] + 12, 0, 100);   // rude awakening
      ring[M_ENERGY] = ring[M_ENERGY] < 30 ? 30 : ring[M_ENERGY];   // enough to stay up for a bit
      e = EV_WAKE; lastEv[e] = now;
      if (mode == MODE_SLEEP) mode = MODE_AUTO;
    }
    const EvFx& fx = EVFX[e];
    float angerGain = 1.3f - ring[M_TRUST] / 200.0f;         // low trust = angers easier
    for (int i = 0; i < M_COUNT; i++) {
      float d = fx.d[i]; if (i == M_ANGRY && d > 0) d *= angerGain;
      ring[i] = clampf(ring[i] + d, 0, 100);
    }
    if (fx.fear > fear) fear = fx.fear;
    if (fx.log[0]) { lastLog = fx.log; logSeq++; }
    if (qn < 8) q[qn++] = e;
  }
  bool pop(Event& e) { if (!qn) return false; e = q[0]; for (int i = 1; i < qn; i++) q[i - 1] = q[i]; qn--; return true; }
  uint32_t since(Event e) const { return lastEv[e] ? millis() - lastEv[e] : 0xFFFFFFFFu; }
  uint32_t sinceActivity() const { return millis() - lastActivity; }

  void update(float dt) {
    // ring drifts to core; a happy core makes bad moods fade faster
    float rate = RING_RATE * (0.5f + core[M_HAPPY] / 100.0f);
    const uint8_t drift[] = {M_HAPPY, M_SAD, M_ANGRY, M_CURIOUS, M_TRUST};
    for (uint8_t i : drift) ring[i] += (core[i] - ring[i]) * rate * dt;
    for (int i = 0; i < M_COUNT; i++) core[i] += (ring[i] - core[i]) * CORE_RATE * dt;
    fear *= expf(-dt * 1.2f);

    float night = 1.0f;                                       // sleepier after midnight (needs NTP)
    time_t tnow = time(nullptr);
    if (tnow > 1700000000) { struct tm tmv; localtime_r(&tnow, &tmv); if (tmv.tm_hour < 7) night = 2.0f; }

    if (asleep) {
      ring[M_ENERGY] = clampf(ring[M_ENERGY] + ENERGY_SLEEP * dt, 0, 100);
      ring[M_BORED] = clampf(ring[M_BORED] - 2.0f * dt, 0, 100);
      if (ring[M_ENERGY] > 90 && mode != MODE_SLEEP && mode != MODE_FOCUS) { asleep = false; event(EV_WAKE); }
    } else {
      float tire = ring[M_BORED] > 80 ? 3.0f : 1.0f;          // being ignored makes him drowsy
      ring[M_ENERGY] = clampf(ring[M_ENERGY] - ENERGY_DRAIN * night * tire * dt, 0, 100);
      ring[M_BORED] = clampf(ring[M_BORED] + BORED_RATE * dt, 0, 100);
      if (ring[M_BORED] > 70) ring[M_SAD] = clampf(ring[M_SAD] + 0.3f * dt, 0, 100);   // ignored = lonely
      bool tired = ring[M_ENERGY] < 15;
      if (mode == MODE_SLEEP || mode == MODE_FOCUS || tired) { asleep = true; event(EV_SLEEP); }
    }
  }

  // Mood -> face/glow weights (0..1). Order = MoodW.
  void weights(float* w) const {
    w[W_HAPPY]   = clampf((ring[M_HAPPY] - 50) / 50.0f, 0, 1) * (1.0f - clampf(ring[M_ANGRY] / 60.0f, 0, 1));
    w[W_SAD]     = clampf((ring[M_SAD] - 35) / 65.0f, 0, 1);
    w[W_ANGRY]   = clampf((ring[M_ANGRY] - 25) / 60.0f, 0, 1);
    w[W_SLEEPY]  = clampf((35 - ring[M_ENERGY]) / 35.0f, 0, 1);
    w[W_CURIOUS] = clampf((ring[M_CURIOUS] - 60) / 40.0f, 0, 1) * 0.7f;
    w[W_SCARED]  = clampf(fear / 100.0f, 0, 1);
  }

  // One word for the phone HUD.
  const char* moodWord() const {
    if (asleep) return "asleep";
    if (fear > 40) return "startled";
    if (ring[M_ANGRY] > 50) return "grumpy";
    if (ring[M_SAD] > 55) return "lonely";
    if (ring[M_ENERGY] < 25) return "sleepy";
    if (ring[M_BORED] > 70) return "bored";
    if (ring[M_CURIOUS] > 65) return "curious";
    if (ring[M_HAPPY] > 75) return "happy";
    return "content";
  }

 private:
  uint32_t lastEv[EV_COUNT] = {0}, lastActivity = 0;
  Event q[8]; uint8_t qn = 0;
};

Brain brain;
